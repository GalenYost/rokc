#include "parser.h"

#include <stdlib.h>
#include <string.h>

// handle errors with "panic mode"
static void parser_error(Parser *p, const char *err);
static void parser_synchronize(Parser *p);

// shift tokens
static void parser_advance(Parser *p);
// strict expect
static Token parser_consume(Parser *p, TokenType type, const char *err);
// optional expect
static bool parser_match(Parser *p, TokenType type);
static unsigned parse_numeric_type_width(Token type_token);

// parser_next helper functions
static Node *parse_import_decl(Parser *p);
static Node *parse_struct_decl(Parser *p);
static Node *parse_enum_decl(Parser *p);
static Node *parse_type_decl(Parser *p);
static Node *parse_var_or_func_decl(Parser *p);
static Node *parse_expr(Parser *p);
static Node *parse_binary_expr(Parser *p, int min_precedence);
static Node *parse_unary_expr(Parser *p);
static Node *parse_primary_expr(Parser *p);
static int binary_precedence(TokenType type);
static bool is_right_associative(TokenType type);

// parse types e.g int, int**, char, void* etc
static DataType parse_type(Parser *p);

static void parser_error(Parser *p, const char *err) {
    if (p->panic_mode) return;

    p->errored = true;
    p->panic_mode = true;

    LOG(ERROR, "%s:%zu:%zu: %s", p->src_path, p->curr.line, p->curr.col, err);
}

static void parser_synchronize(Parser *p) {
    p->panic_mode = false;

    while (p->curr.type != TOKEN_EOF) {
        if (p->prev.type == TOKEN_SEMI) return;

        switch (p->curr.type) {
            case TOKEN_STRUCT:
            case TOKEN_STATIC:
            case TOKEN_IMPORT:
            case TOKEN_IF:
            case TOKEN_WHILE:
            case TOKEN_FOR:
            case TOKEN_RETURN:
            case TOKEN_DEFER:
            case TOKEN_COMPTIME:
                return;
            default:
                break;
        }

        parser_advance(p);
    }
}

static void parser_advance(Parser *p) {
    do {
        p->prev = p->curr;
        p->curr = lexer_next(p->lex);
    } while (p->curr.type == TOKEN_UNKNOWN);
}

static Token parser_consume(Parser *p, TokenType type, const char *err) {
    if (p->curr.type == type) {
        parser_advance(p);
        return p->prev;
    }

    parser_error(p, err);
    return p->curr;
}

static bool parser_match(Parser *p, TokenType type) {
    if (p->curr.type == type) {
        parser_advance(p);
        return true;
    }
    return false;
}

static Node *parse_import_decl(Parser *p) {
    Token kw = p->prev;
    Token ident = parser_consume(p, TOKEN_IDENT, "Expected import path");
    if (p->errored) return NULL;

    const char *start_ptr = ident.view.data;
    const char *end_ptr = start_ptr + ident.view.len;

    while (parser_match(p, TOKEN_DOT)) {
        Token next = parser_consume(p, TOKEN_IDENT, "Expected submodule name after '.'");
        if (p->errored) return NULL;

        end_ptr = next.view.data + next.view.len;
    }

    parser_consume(p, TOKEN_SEMI, "Expected ';' after import path");

    Node *n = node_new(p->a, NODE_IMPORT, kw.line, kw.col, p->src_path);
    n->as.import_stmt.path.data = (char *)start_ptr;
    n->as.import_stmt.path.len = (size_t)(end_ptr - start_ptr);
    return n;
}

static Node *parse_struct_decl(Parser *p) {
    Token kw = p->prev;
    Token struct_name = parser_consume(p, TOKEN_IDENT, "Expected struct name");
    if (p->errored) return NULL;

    Node *n = node_new(p->a, NODE_STRUCT_DECL, kw.line, kw.col, p->src_path);
    n->as.struct_decl.name = struct_name;

    parser_consume(p, TOKEN_CURLY_LEFT, "Expected '{' before struct declaration");

    Node *head = NULL;
    Node *tail = NULL;
    size_t count = 0;

    while (p->curr.type != TOKEN_CURLY_RIGHT && p->curr.type != TOKEN_EOF) {
        Node *member = parse_var_or_func_decl(p);
        if (p->panic_mode) {
            parser_synchronize(p);
            continue; // try next members
        }
        if (!member) continue;

        member->next = NULL;
        if (!head) {
            head = member;
            tail = member;
        } else {
            tail->next = member;
            tail = member;
        }
        count++;
    }

    parser_consume(p, TOKEN_CURLY_RIGHT, "Expected '}' after struct declaration");

    n->as.struct_decl.fields = head;
    n->as.struct_decl.field_count = count;

    return n;
}

static Node *parse_enum_decl(Parser *p) {
    Token kw = p->prev;
    Token enum_name = parser_consume(p, TOKEN_IDENT, "Expected enum identifier");
    if (p->errored) return NULL;

    Node *n = node_new(p->a, NODE_ENUM_DECL, kw.line, kw.col, p->src_path);
    n->as.enum_decl.name = enum_name;

    parser_consume(p, TOKEN_CURLY_LEFT, "Expected '{' before enum declaration");

    Node *head = NULL;
    Node *tail = NULL;
    size_t count = 0;

    while (p->curr.type != TOKEN_CURLY_RIGHT && p->curr.type != TOKEN_EOF) {
        Node *member = parse_var_or_func_decl(p);
        if (p->panic_mode) {
            parser_synchronize(p);
            continue; // try next members
        }
        if (!member) continue;

        member->next = NULL;
        if (!head) {
            head = member;
            tail = member;
        } else {
            tail->next = member;
            tail = member;
        }
        count++;
    }

    parser_consume(p, TOKEN_CURLY_RIGHT, "Expected '}' after enum declaration");

    n->as.enum_decl.variants = head;
    n->as.enum_decl.variant_count = count;

    return n;
}

static DataType parse_type(Parser *p) {
    DataType dt = {0};

    if (p->curr.type == TOKEN_SIGNED_INT) {
        Token token = p->curr;
        parser_advance(p);
        dt.kind = TYPE_INT;
        if (token.view.len == 3 && memcmp(token.view.data, "int", 3) == 0) {
            // Target-native signed integer; no explicit width.
        } else if (token.view.len == 5 && memcmp(token.view.data, "isize", 5) == 0) {
            dt.is_target_sized = true;
        } else {
            dt.bit_width = parse_numeric_type_width(token);
        }
    } else if (p->curr.type == TOKEN_UNSIGNED_INT) {
        Token token = p->curr;
        parser_advance(p);
        dt.kind = TYPE_UINT;
        if (token.view.len == 5 && memcmp(token.view.data, "usize", 5) == 0) {
            dt.is_target_sized = true;
        } else {
            dt.bit_width = parse_numeric_type_width(token);
        }
    } else if (parser_match(p, TOKEN_CHAR)) {
        dt.kind = TYPE_CHAR;
    } else if (p->curr.type == TOKEN_FLOAT) {
        Token token = p->curr;
        parser_advance(p);
        dt.kind = TYPE_FLOAT;
        dt.bit_width = token.view.len == 3 && token.view.data[1] >= '0' &&
                       token.view.data[1] <= '9'
                           ? parse_numeric_type_width(token)
                           : 32;
    } else if (parser_match(p, TOKEN_BOOL)) {
        dt.kind = TYPE_BOOL;
    } else if (parser_match(p, TOKEN_VOID)) {
        dt.kind = TYPE_VOID;
    } else {
        Token ident = parser_consume(p, TOKEN_IDENT, "Expected type name");
        dt.kind = TYPE_CUSTOM;
        dt.name = ident.view;
    }

    while (parser_match(p, TOKEN_STAR)) {
        dt.pointer_depth++;
    }

    if (parser_match(p, TOKEN_BRACE_LEFT)) {
        parser_consume(p, TOKEN_BRACE_RIGHT, "Expected ']' after '['");
    }

    return dt;
}

static unsigned parse_numeric_type_width(Token type_token) {
    unsigned width = 0;
    for (size_t i = 1; i < type_token.view.len; i++) {
        width = width * 10 + (unsigned)(type_token.view.data[i] - '0');
    }
    return width;
}

static Node *parse_type_decl(Parser *p) {
    Token kw = p->prev;

    Token alias_name = parser_consume(p, TOKEN_IDENT, "Expected alias identifier name");
    if (p->errored) return NULL;

    parser_consume(p, TOKEN_EQ, "Expected '=' after alias name");
    if (p->errored) return NULL;

    DataType target = parse_type(p);
    if (p->errored) return NULL;

    parser_consume(p, TOKEN_SEMI, "Expected ';' after type alias declaration");

    Node *n = node_new(p->a, NODE_TYPE_DECL, kw.line, kw.col, p->src_path);
    n->as.type_decl.alias_name = alias_name;
    n->as.type_decl.target_type = target;
    return n;
}

static Node *parse_var_or_func_decl(Parser *p) {
    // the order matters here since
    // `comptime static const` is possible
    const bool is_comptime = parser_match(p, TOKEN_COMPTIME);
    const bool is_static = parser_match(p, TOKEN_STATIC);
    const bool is_const = parser_match(p, TOKEN_CONST);

    const DataType type = parse_type(p);
    const Token name = parser_consume(p, TOKEN_IDENT, "Expected identifier after type name");

    // assume variable declaration without initializer by default
    NodeKind kind = NODE_VAR_DECL;
    Node *initializer = NULL;

    if (parser_match(p, TOKEN_EQ)) {
        kind = NODE_VAR_DECL;
        initializer = parse_expr(p);
    } else if (parser_match(p, TOKEN_BRACE_LEFT)) {
        kind = NODE_FUNC_DECL;
        // TODO
        // func args
        // expect right brace
        // try left curly
        // parse body
        // expect right curly
    }

    Node *n = node_new(p->a, NODE_VAR_DECL, p->curr.line, p->curr.col, p->src_path);
    n->_comptime = is_comptime;

    switch (kind) {
        case NODE_VAR_DECL: {
            n->as.var_decl.initializer = initializer;

            n->as.var_decl.type = type;
            n->as.var_decl.name = name;

            n->as.var_decl.is_static = is_static;
            n->as.var_decl.is_const = is_const;

            break;
        };
        case NODE_FUNC_DECL: {
            n->kind = NODE_FUNC_DECL;

            n->as.func_decl.is_static = is_static;
            n->as.func_decl.is_const = is_const;

            n->as.func_decl.return_type = type;
            n->as.func_decl.name = name;
            // TODO: add body and arguments

            break;
        }
        default: break; // TODO: might want to add error handling here
    };

    parser_consume(p, TOKEN_SEMI, "Expected ';' after variable or function declaration");

    return n;
}

static Node *parse_expr(Parser *p) {
    return parse_binary_expr(p, 1);
}

static Node *parse_binary_expr(Parser *p, int min_precedence) {
    Node *left = parse_unary_expr(p);
    if (!left) return NULL;

    while (!p->panic_mode) {
        int precedence = binary_precedence(p->curr.type);
        if (precedence < min_precedence) break;

        Token op = p->curr;
        parser_advance(p);

        int rhs_min_precedence = is_right_associative(op.type)
                                     ? precedence
                                     : precedence + 1;
        Node *right = parse_binary_expr(p, rhs_min_precedence);
        if (!right) return NULL;

        Node *binary = node_new(p->a, NODE_BINOP, op.line, op.col, p->src_path);
        binary->as.binop.left = left;
        binary->as.binop.right = right;
        binary->as.binop.op = op.type;
        left = binary;
    }

    return left;
}

static Node *parse_unary_expr(Parser *p) {
    TokenType op = p->curr.type;
    switch (op) {
        case TOKEN_BANG:
        case TOKEN_BIT_NOT:
        case TOKEN_PLUS:
        case TOKEN_MINUS:
        case TOKEN_PLUS_PLUS:
        case TOKEN_MINUS_MINUS:
        case TOKEN_STAR:
        case TOKEN_BIT_AND: {
            Token operator_token = p->curr;
            parser_advance(p);

            Node *operand = parse_unary_expr(p);
            if (!operand) return NULL;

            Node *unary = node_new(p->a, NODE_UNARY, operator_token.line,
                                   operator_token.col, p->src_path);
            unary->as.unary.operand = operand;
            unary->as.unary.op = op;
            return unary;
        }
        default:
            return parse_primary_expr(p);
    }
}

static Node *parse_primary_expr(Parser *p) {
    Token token = p->curr;

    switch (token.type) {
        case TOKEN_LIT_INT:
        case TOKEN_LIT_FLOAT:
        case TOKEN_LIT_STR:
        case TOKEN_LIT_CHAR:
        case TOKEN_TRUE:
        case TOKEN_FALSE:
        case TOKEN_NULL: {
            parser_advance(p);
            Node *literal = node_new(p->a, NODE_LITERAL, token.line, token.col, p->src_path);
            literal->as.literal.token = token;
            return literal;
        }
        case TOKEN_IDENT: {
            parser_advance(p);
            Node *identifier = node_new(p->a, NODE_IDENTIFIER, token.line, token.col, p->src_path);
            identifier->as.identifier.name = token;
            return identifier;
        }
        case TOKEN_PAREN_LEFT: {
            parser_advance(p);
            Node *expr = parse_expr(p);
            if (!expr) return NULL;
            parser_consume(p, TOKEN_PAREN_RIGHT, "Expected ')' after expression");
            return p->panic_mode ? NULL : expr;
        }
        default:
            parser_error(p, "Expected expression");
            return NULL;
    }
}

static int binary_precedence(TokenType type) {
    switch (type) {
        case TOKEN_COMMA: return 1;

        case TOKEN_EQ:
        case TOKEN_PLUS_EQ:
        case TOKEN_MINUS_EQ:
        case TOKEN_MULTI_EQ:
        case TOKEN_DIV_EQ:
        case TOKEN_SHIFT_LEFT_EQ:
        case TOKEN_SHIFT_RIGHT_EQ:
        case TOKEN_BIT_AND_EQ:
        case TOKEN_XOR_EQ:
        case TOKEN_BIT_OR_EQ:
            return 2;

        case TOKEN_LOGICAL_OR: return 3;
        case TOKEN_LOGICAL_AND: return 4;
        case TOKEN_BIT_OR: return 5;
        case TOKEN_XOR: return 6;
        case TOKEN_BIT_AND: return 7;
        case TOKEN_EQ_EQ:
        case TOKEN_NOT_EQ: return 8;
        case TOKEN_LT:
        case TOKEN_LT_EQ:
        case TOKEN_GT:
        case TOKEN_GT_EQ: return 9;
        case TOKEN_SHIFT_LEFT:
        case TOKEN_SHIFT_RIGHT: return 10;
        case TOKEN_PLUS:
        case TOKEN_MINUS: return 11;
        case TOKEN_STAR:
        case TOKEN_SLASH:
        case TOKEN_PERCENT: return 12;
        default: return 0;
    }
}

static bool is_right_associative(TokenType type) {
    switch (type) {
        case TOKEN_EQ:
        case TOKEN_PLUS_EQ:
        case TOKEN_MINUS_EQ:
        case TOKEN_MULTI_EQ:
        case TOKEN_DIV_EQ:
        case TOKEN_SHIFT_LEFT_EQ:
        case TOKEN_SHIFT_RIGHT_EQ:
        case TOKEN_BIT_AND_EQ:
        case TOKEN_XOR_EQ:
        case TOKEN_BIT_OR_EQ:
            return true;
        default:
            return false;
    }
}

Parser parser_new(Lexer *lex, Arena *a, const char *path) {
    Parser p = (Parser) {
        .lex = lex,
        .a = a,
        .src_path = path,
        .errored = false,
        .panic_mode = false,
    };

    p.curr.type = TOKEN_UNKNOWN;
    parser_advance(&p);

    return p;
}

Node *parser_next(Parser *p) {
    if (p->curr.type == TOKEN_EOF) return NULL;

    if (parser_match(p, TOKEN_IMPORT)) return parse_import_decl(p);
    if (parser_match(p, TOKEN_STRUCT)) return parse_struct_decl(p);
    if (parser_match(p, TOKEN_ENUM)) return parse_enum_decl(p);
    if (parser_match(p, TOKEN_TYPE)) return parse_type_decl(p);

    Node *decl = parse_var_or_func_decl(p);

    if (p->panic_mode) {
        parser_synchronize(p);
        return NULL;
    }

    return decl;
}
