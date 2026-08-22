#include "parser.h"

#include <stdlib.h>

// handle errors with "panic mode"
static void parser_error(Parser *p, const char *err);
static void parser_synchronize(Parser *p);

// shift tokens
static void parser_advance(Parser *p);
// strict expect
static Token parser_consume(Parser *p, TokenType type, const char *err);
// optional expect
static bool parser_match(Parser *p, TokenType type);

// parser_next helper functions
static Node *parse_import_decl(Parser *p);
static Node *parse_struct_decl(Parser *p);
static Node *parse_enum_decl(Parser *p);
static Node *parse_type_decl(Parser *p);
static Node *parse_var_or_func_decl(Parser *p);

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

    if (parser_match(p, TOKEN_INT)) {
        dt.kind = TYPE_INT;
    } else if (parser_match(p, TOKEN_CHAR)) {
        dt.kind = TYPE_CHAR;
    } else if (parser_match(p, TOKEN_FLOAT)) {
        dt.kind = TYPE_FLOAT;
    } else if (parser_match(p, TOKEN_BOOL)) {
        dt.kind = TYPE_BOOL;
    } else if (parser_match(p, TOKEN_VOID)) {
        dt.kind = TYPE_VOID;
    } else if (parser_match(p, TOKEN_NULL)) {
        dt.kind = TYPE_NULL;
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
    // TODO
    // parse type
    // parse name
    // expect '=' or '('
    // if '=' - parse literal and expect ';'
    // --------
    // else if '(' - parse args and expect ')'
    // 
    // try to:
    // either get ';' to simply declare function
    // or
    // get '{' and parse body + expect '}'
    return NULL;
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
