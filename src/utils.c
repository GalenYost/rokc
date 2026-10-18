#include "utils.h"
#include "arena.h"
#include "ast.h"

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

static const char *words[] = {
    // DECLARATIONS
    [NODE_VAR_DECL] = "NODE_VAR_DECL",
    [NODE_FUNC_DECL] = "NODE_FUNC_DECL",
    [NODE_STRUCT_DECL] = "NODE_STRUCT_DECL",
    [NODE_ENUM_DECL] = "NODE_ENUM_DECL",
    [NODE_TYPE_DECL] = "NODE_TYPE_DECL",
    // STATEMENTS
    [NODE_BLOCK] = "NODE_BLOCK",
    [NODE_IF] = "NODE_IF",
    [NODE_WHILE] = "NODE_WHILE",
    [NODE_FOR] = "NODE_FOR",
    [NODE_RETURN] = "NODE_RETURN",
    [NODE_BREAK] = "NODE_BREAK",
    [NODE_CONTINUE] = "NODE_CONTINUE",
    [NODE_DEFER] = "NODE_DEFER",
    [NODE_EXPR_STMT] = "NODE_EXPR_STMT",
    [NODE_IMPORT] = "NODE_IMPORT",
    // EXPRESSIONS
    [NODE_LITERAL] = "NODE_LITERAL",
    [NODE_IDENTIFIER] = "NODE_IDENTIFIER",
    [NODE_BINOP] = "NODE_BINOP",
    [NODE_UNARY] = "NODE_UNARY",
    [NODE_CALL] = "NODE_CALL",
    [NODE_MEMBER_ACCESS] = "NODE_MEMBER_ACCESS",
    [NODE_INDEX] = "NODE_INDEX",
    [NODE_STRUCT_INIT] = "NODE_STRUCT_INIT",
    [NODE_ARRAY_INIT] = "NODE_ARRAY_INIT",
    [NODE_SWITCH] = "NODE_SWITCH",
    [NODE_CASE] = "NODE_CASE",
};

static char *vfmt(Arena *a, const char *fmt, ...);

typedef struct {
    char *data;
    size_t len;
    size_t cap;
    bool failed;
} FormatBuffer;

static bool format_buffer_reserve(FormatBuffer *b, size_t extra) {
    if (extra <= b->cap - b->len) return true;

    size_t required = b->len + extra;
    size_t cap = b->cap ? b->cap : 256;
    while (cap < required) {
        if (cap > SIZE_MAX / 2) {
            cap = required;
            break;
        }
        cap *= 2;
    }

    char *data = (char *)realloc(b->data, cap);
    if (!data) {
        b->failed = true;
        return false;
    }

    b->data = data;
    b->cap = cap;
    return true;
}

static void format_buffer_append_n(FormatBuffer *b, const char *data, size_t len) {
    if (b->failed || !data || len == 0) return;
    if (!format_buffer_reserve(b, len)) return;
    memcpy(b->data + b->len, data, len);
    b->len += len;
}

static void format_buffer_append(FormatBuffer *b, const char *data) {
    if (data) format_buffer_append_n(b, data, strlen(data));
}

static void format_buffer_append_fmt(FormatBuffer *b, const char *fmt, ...) {
    if (b->failed) return;

    va_list args;
    va_start(args, fmt);
    va_list size_args;
    va_copy(size_args, args);
    int size = vsnprintf(NULL, 0, fmt, size_args);
    va_end(size_args);

    if (size < 0 || !format_buffer_reserve(b, (size_t)size + 1)) {
        b->failed = true;
        va_end(args);
        return;
    }

    vsnprintf(b->data + b->len, (size_t)size + 1, fmt, args);
    b->len += (size_t)size;
    va_end(args);
}

static void format_buffer_indent(FormatBuffer *b, size_t indent) {
    for (size_t i = 0; i < indent; i++) format_buffer_append(b, "  ");
}

static void format_buffer_free(FormatBuffer *b) {
    free(b->data);
    b->data = NULL;
    b->len = 0;
    b->cap = 0;
}

static char *vfmt(Arena *a, const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);

    va_list size_args;
    va_copy(size_args, args);
    int size = vsnprintf(NULL, 0, fmt, size_args);
    va_end(size_args);

    if (size < 0) {
        va_end(args);
        return NULL;
    }

    char *result = (char *)arena_alloc(a, (size_t)size + 1);
    if (result) vsnprintf(result, (size_t)size + 1, fmt, args);

    va_end(args);
    return result;
}

static void format_token_literal_bytes(FormatBuffer *b, StringStream view,
                                       bool escape_quotes, bool escape_backslashes) {
    for (size_t i = 0; view.data && i < view.len; i++) {
        unsigned char c = (unsigned char)view.data[i];
        const char *escape = NULL;

        switch (c) {
            case '\n': escape = "\\n"; break;
            case '\r': escape = "\\r"; break;
            case '\t': escape = "\\t"; break;
            case '\0': escape = "\\0"; break;
            case '\\':
                if (escape_backslashes) escape = "\\\\";
                break;
            case '"':
                if (escape_quotes) escape = "\\\"";
                break;
        }

        if (escape) {
            format_buffer_append(b, escape);
        } else if (c < 0x20 || c == 0x7f) {
            format_buffer_append_fmt(b, "\\x%02x", c);
        } else {
            char byte = (char)c;
            format_buffer_append_n(b, &byte, 1);
        }
    }
}

static char *format_literal_token(Arena *a, Token t, const char *name) {
    FormatBuffer b = {0};
    format_buffer_append_fmt(&b, "%zu:%zu : %s(", t.line, t.col, name);

    if (t.type == TOKEN_LIT_STR) {
        format_buffer_append(&b, "\"");
        format_token_literal_bytes(&b, t.view, true, true);
        format_buffer_append(&b, "\"");
    } else {
        // Character token views still contain their source quotes and escape spelling.
        // Escape literal control bytes for display, while preserving source backslashes.
        format_token_literal_bytes(&b, t.view, false, false);
    }
    format_buffer_append(&b, ")");

    char *result = NULL;
    if (!b.failed) {
        result = (char *)arena_alloc(a, b.len + 1);
        if (result) {
            memcpy(result, b.data, b.len);
            result[b.len] = '\0';
        }
    }
    format_buffer_free(&b);
    return result;
}

StringStream sstream_new(char *str) {
    return (StringStream) {
        .data = str,
        .len = strlen(str),
    };
}

StringStream sstream_remove(Arena *a, StringStream ss, char c) {
    if (!a || (!ss.data && ss.len != 0) || ss.len == SIZE_MAX) {
        return (StringStream){0};
    }

    char *view = (char *)arena_alloc(a, ss.len + 1);
    if (!view) return (StringStream){0};

    size_t i = 0;
    size_t next_i = 0;
    while (i < ss.len) {
        char cur = ss.data[i];
        if (cur != c) {
            view[next_i] = cur;
            next_i++;
        }
        i++;
    }

    view[next_i] = '\0';
    return (StringStream){
        .data = view,
        .len = next_i,
    };
}

StringStream sstream_dump(Arena *a, StringStream ss) {
    char *dest = (char *)arena_alloc(a, ss.len);
    memcpy(dest, ss.data, ss.len);
    
    return (StringStream){
        .data = dest,
        .len = ss.len,
    };
}

char *sstream_to_cstr(Arena *a, StringStream ss) {
    char *cstr = (char *)arena_alloc(a, ss.len + 1);
    memcpy(cstr, ss.data, ss.len);
    cstr[ss.len] = '\0';

    return cstr;
}

StringStream read_file(const char *path) {
    StringStream ss = {0};

    FILE *f = fopen(path, "rb");
    if (!f) {
        LOG(ERROR, "Could not open file for reading: %s", path);
        return ss;
    }

    fseek(f, 0, SEEK_END);
    long fsize = ftell(f);
    fseek(f, 0, SEEK_SET);

    if (fsize < 0) {
        LOG(ERROR, "Could not read file size: %s", path);
        fclose(f);
        return ss;
    }

    char *buf = (char*)malloc(fsize + 1);

    if (!buf) {
        LOG(FATAL, "Could not allocate %ld bytes to read file %s", fsize, path);
        fclose(f);
        exit(1);
    }

    size_t bytes_read = fread(buf, 1, fsize, f);

    if (bytes_read < (size_t)fsize) {
        LOG(ERROR, "Failed to read entire file: %s", path);
        free(buf);
        fclose(f);
        return ss;
    }

    buf[bytes_read] = '\0';
    fclose(f);

    ss.data = buf;
    ss.len = strlen(buf);

    return ss;
}

#define CONSOLE_PREFIX_FMT "[%s%s%s] "
#define FILE_PREFIX_FMT "[%s] "

#define CYAN "\033[36m"
#define RED "\033[91m"
#define GREEN "\033[92m"

#define DARK_RED "\033[31m"
#define DARK_YELLOW "\033[33m"

#define RESET_COLOR "\033[0m"

static const char *LOG_PREFIXES[] = {
    "DEBUG", "INFO",
    "WARN", "ERROR",
    "FATAL",
};

static const char *LOG_PREFIX_COLORS[] = {
    CYAN,
    GREEN,
    DARK_YELLOW,
    RED, DARK_RED,
};

void log_to_console(LogLevel level, const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);

    printf(CONSOLE_PREFIX_FMT, LOG_PREFIX_COLORS[level], LOG_PREFIXES[level], RESET_COLOR);
    vprintf(fmt, args);
    printf("\n");

    va_end(args);
}

void log_to_file(LogLevel level, const char *path, const char *fmt, ...) {
    FILE *f = fopen(path, "a");
    if (!f) return;

    va_list args;
    va_start(args, fmt);

    fprintf(f, FILE_PREFIX_FMT, LOG_PREFIXES[level]);
    vfprintf(f, fmt, args);
    fprintf(f, "\n");

    va_end(args);
    fclose(f);
}

char *format_token(Arena *a, Token t) {
    static const char *words[] = {
        // SYMBOLS
        [TOKEN_PLUS] = "TOKEN_PLUS",
        [TOKEN_MINUS] = "TOKEN_MINUS",
        [TOKEN_STAR] = "TOKEN_STAR",
        [TOKEN_SLASH] = "TOKEN_SLASH",
        [TOKEN_EQ] = "TOKEN_EQ",
        [TOKEN_PAREN_LEFT] = "TOKEN_PAREN_LEFT",
        [TOKEN_PAREN_RIGHT] = "TOKEN_PARENT_RIGHT",
        [TOKEN_BRACE_LEFT] = "TOKEN_BRACE_LEFT",
        [TOKEN_BRACE_RIGHT] = "TOKEN_BRACE_RIGHT",
        [TOKEN_CURLY_LEFT] = "TOKEN_CURLY_LEFT",
        [TOKEN_CURLY_RIGHT] = "TOKEN_CURLY_RIGHT",
        [TOKEN_BANG] = "TOKEN_BANG",
        [TOKEN_AT] = "TOKEN_AT",
        [TOKEN_HASH] = "TOKEN_HASH",
        [TOKEN_DOLLAR] = "TOKEN_DOLLAR",
        [TOKEN_PERCENT] = "TOKEN_PERCENT",
        [TOKEN_QUOTE] = "TOKEN_QUOTE",
        [TOKEN_DOUBLE_QUOTE] = "TOKEN_DOUBLE_QUOTE",
        [TOKEN_QUESTION] = "TOKEN_QUESTION",
        [TOKEN_COMMA] = "TOKEN_COMMA",
        [TOKEN_SEMI] = "TOKEN_SEMI",
        [TOKEN_COLON] = "TOKEN_COLON",
        [TOKEN_DOT] = "TOKEN_DOT",
        // OPERATORS
        [TOKEN_PLUS_PLUS] = "TOKEN_PLUS_PLUS",
        [TOKEN_PLUS_EQ] = "TOKEN_PLUS_EQ",
        [TOKEN_MINUS_MINUS] = "TOKEN_MINUS_MINUS",
        [TOKEN_MINUS_EQ] = "TOKEN_MINUS_EQ",
        [TOKEN_MULTI_EQ] = "TOKEN_MULTI_EQ",
        [TOKEN_DIV_EQ] = "TOKEN_DIV_EQ",
        [TOKEN_EQ_EQ] = "TOKEN_EQ_EQ",
        [TOKEN_NOT_EQ] = "TOKEN_NOT_EQ",
        [TOKEN_LT] = "TOKEN_LT",
        [TOKEN_LT_EQ] = "TOKEN_LT_EQ",
        [TOKEN_GT] = "TOKEN_GT",
        [TOKEN_GT_EQ] = "TOKEN_GT_EQ",
        [TOKEN_POW] = "TOKEN_POW",
        [TOKEN_XOR] = "TOKEN_XOR",
        [TOKEN_LOGICAL_AND] = "TOKEN_LOGICAL_AND",
        [TOKEN_LOGICAL_OR] = "TOKEN_LOGICAL_OR",
        [TOKEN_BIT_OR] = "TOKEN_BIT_OR",
        [TOKEN_BIT_OR_EQ] = "TOKEN_BIT_OR_EQ",
        [TOKEN_BIT_AND] = "TOKEN_BIT_AND",
        [TOKEN_BIT_AND_EQ] = "TOKEN_BIT_AND_EQ",
        [TOKEN_SHIFT_LEFT] = "TOKEN_SHIFT_LEFT",
        [TOKEN_SHIFT_LEFT_EQ] = "TOKEN_SHIFT_LEFT_EQ",
        [TOKEN_SHIFT_RIGHT] = "TOKEN_SHIFT_RIGHT",
        [TOKEN_SHIFT_RIGHT_EQ] = "TOKEN_SHIFT_RIGHT_EQ",
        [TOKEN_BIT_NOT] = "TOKEN_BIT_NOT",
        [TOKEN_XOR_EQ] = "TOKEN_XOR_EQ",
        // KEYWORDS
        [TOKEN_WHILE] = "TOKEN_WHILE",
        [TOKEN_FOR] = "TOKEN_FOR",
        [TOKEN_BREAK] = "TOKEN_BREAK",
        [TOKEN_RETURN] = "TOKEN_RETURN",
        [TOKEN_CONTINUE] = "TOKEN_CONTINUE",
        [TOKEN_IF] = "TOKEN_IF",
        [TOKEN_ELSE] = "TOKEN_ELSE",
        [TOKEN_IMPORT] = "TOKEN_IMPORT",
        [TOKEN_STATIC] = "TOKEN_STATIC",
        [TOKEN_DEFER] = "TOKEN_DEFER",
        [TOKEN_COMPTIME] = "TOKEN_COMPTIME",
        [TOKEN_STRUCT] = "TOKEN_STRUCT",
        [TOKEN_TYPE] = "TOKEN_TYPE",
        [TOKEN_CONST] = "TOKEN_CONST",
        [TOKEN_ENUM] = "TOKEN_ENUM",
        [TOKEN_TRUE] = "TOKEN_TRUE",
        [TOKEN_FALSE] = "TOKEN_FALSE",
        [TOKEN_SWITCH] = "TOKEN_SWITCH",
        [TOKEN_CASE] = "TOKEN_CASE",
        [TOKEN_DEFAULT] = "TOKEN_DEFAULT",
        // TYPES
        [TOKEN_CHAR] = "TOKEN_CHAR",
        [TOKEN_INT] = "TOKEN_INT",
        [TOKEN_FLOAT] = "TOKEN_FLOAT",
        [TOKEN_BOOL] = "TOKEN_BOOL",
        [TOKEN_VOID] = "TOKEN_VOID",
        [TOKEN_NULL] = "TOKEN_NULL",
        // LITERALS
        [TOKEN_LIT_STR] = "TOKEN_LIT_STR",
        [TOKEN_LIT_CHAR] = "TOKEN_LIT_CHAR",
        [TOKEN_LIT_INT] = "TOKEN_LIT_INT",
        [TOKEN_LIT_FLOAT] = "TOKEN_LIT_FLOAT",
        [TOKEN_IDENT] = "TOKEN_IDENT",
        // OTHER
        [TOKEN_EOF] = "TOKEN_EOF",
        [TOKEN_UNKNOWN] = "TOKEN_UNKNOWN",
    };

    if (t.type >= TOKEN_PLUS && t.type < TOKEN_COUNT && words[t.type]) {
        if (t.type == TOKEN_LIT_STR || t.type == TOKEN_LIT_CHAR) {
            return format_literal_token(a, t, words[t.type]);
        }

        return vfmt(a, "%zu:%zu : %s(%.*s)",
                    t.line, t.col, words[t.type], (int)t.view.len,
                    t.view.data ? t.view.data : "");
    }

    return vfmt(a, "<invalid token: %d>", t.type);
}

void print_token(Token t) {
    Arena a = arena_new(1024);
    char *formatted = format_token(&a, t);
    if (formatted) printf("%s\n", formatted);
    arena_free(&a);
}

void print_node(Node *n) {
    Arena a = arena_new(1024);
    char *formatted = format_node(&a, n);
    if (formatted) printf("%s\n", formatted);
    arena_free(&a);
}

static void format_node_one(FormatBuffer *b, Arena *scratch, Node *n, size_t indent);

static void format_node_chain(FormatBuffer *b, Arena *scratch, Node *head, size_t indent) {
    if (!head) {
        format_buffer_indent(b, indent);
        format_buffer_append(b, "<empty>\n");
        return;
    }

    for (Node *node = head; node; node = node->next) {
        format_node_one(b, scratch, node, indent);
    }
}

static void format_token_field(FormatBuffer *b, Arena *scratch, size_t indent,
                               const char *label, Token token) {
    format_buffer_indent(b, indent);
    format_buffer_append_fmt(b, "%s: ", label);
    arena_clear(scratch);
    char *formatted = format_token(scratch, token);
    format_buffer_append(b, formatted ? formatted : "<invalid token>");
    format_buffer_append(b, "\n");
}

static void format_type_field(FormatBuffer *b, size_t indent, const char *label, const DataType *type) {
    static const char *kind_names[] = {
        [TYPE_VOID] = "void",
        [TYPE_NULL] = "null",
        [TYPE_INT] = "int",
        [TYPE_FLOAT] = "float",
        [TYPE_CHAR] = "char",
        [TYPE_BOOL] = "bool",
        [TYPE_CUSTOM] = "custom",
    };

    format_buffer_indent(b, indent);
    format_buffer_append_fmt(b, "%s: ", label);
    bool valid_kind = type->kind >= 0 &&
                      type->kind < (int)(sizeof(kind_names) / sizeof(kind_names[0]));
    if (!valid_kind) {
        format_buffer_append(b, "<invalid type>");
    } else if (type->kind == TYPE_CUSTOM && type->name.data) {
        format_buffer_append_n(b, type->name.data, type->name.len);
    } else {
        format_buffer_append(b, kind_names[type->kind]);
    }
    for (int i = 0; i < type->pointer_depth; i++) format_buffer_append(b, "*");
    format_buffer_append_fmt(b, " (kind=%s, pointer_depth=%d)\n", valid_kind ? kind_names[type->kind] : "invalid", type->pointer_depth);
}

static void format_node_field(FormatBuffer *b, Arena *scratch, size_t indent,
                              const char *label, Node *node) {
    format_buffer_indent(b, indent);
    format_buffer_append_fmt(b, "%s:\n", label);
    if (node) format_node_one(b, scratch, node, indent + 1);
    else {
        format_buffer_indent(b, indent + 1);
        format_buffer_append(b, "<null>\n");
    }
}

static void format_node_array_field(FormatBuffer *b, Arena *scratch, size_t indent,
                                    const char *label, Node **nodes, size_t count) {
    format_buffer_indent(b, indent);
    format_buffer_append_fmt(b, "%s (count=%zu):\n", label, count);
    if (!nodes || count == 0) {
        format_buffer_indent(b, indent + 1);
        format_buffer_append(b, "<empty>\n");
        return;
    }

    for (size_t i = 0; i < count; i++) {
        format_buffer_indent(b, indent + 1);
        format_buffer_append_fmt(b, "[%zu]:\n", i);
        if (nodes[i]) format_node_one(b, scratch, nodes[i], indent + 2);
        else {
            format_buffer_indent(b, indent + 2);
            format_buffer_append(b, "<null>\n");
        }
    }
}

static void format_node_one(FormatBuffer *b, Arena *scratch, Node *n, size_t indent) {
    if (!n) {
        format_buffer_indent(b, indent);
        format_buffer_append(b, "<null>\n");
        return;
    }

    format_buffer_indent(b, indent);
    if (n->kind >= NODE_COUNT || !words[n->kind]) {
        format_buffer_append_fmt(b, "<invalid node kind=%d>\n", n->kind);
        return;
    }

    format_buffer_append_fmt(b, "%s (line=%zu, col=%zu, path=%s, export=%s, comptime=%s)\n",
                             words[n->kind], n->line, n->col,
                             n->path ? n->path : "<unknown>",
                             format_bool(n->_export), format_bool(n->_comptime));

    switch (n->kind) {
        case NODE_VAR_DECL:
            format_token_field(b, scratch, indent + 1, "name", n->as.var_decl.name);
            format_type_field(b, indent + 1, "type", &n->as.var_decl.type);
            format_buffer_indent(b, indent + 1);
            format_buffer_append_fmt(b, "is_const: %s\n", format_bool(n->as.var_decl.is_const));
            format_buffer_indent(b, indent + 1);
            format_buffer_append_fmt(b, "is_static: %s\n", format_bool(n->as.var_decl.is_static));
            format_node_field(b, scratch, indent + 1, "initializer", n->as.var_decl.initializer);
            break;

        case NODE_FUNC_DECL:
            format_token_field(b, scratch, indent + 1, "name", n->as.func_decl.name);
            format_type_field(b, indent + 1, "return_type", &n->as.func_decl.return_type);
            format_buffer_indent(b, indent + 1);
            format_buffer_append_fmt(b, "param_count: %zu\n", n->as.func_decl.param_count);
            format_buffer_indent(b, indent + 1);
            format_buffer_append(b, "params:\n");
            format_node_chain(b, scratch, n->as.func_decl.params, indent + 2);
            format_node_field(b, scratch, indent + 1, "body", n->as.func_decl.body);
            format_buffer_indent(b, indent + 1);
            break;

        case NODE_STRUCT_DECL:
            format_token_field(b, scratch, indent + 1, "name", n->as.struct_decl.name);
            format_buffer_indent(b, indent + 1);
            format_buffer_append_fmt(b, "field_count: %zu\n", n->as.struct_decl.field_count);
            format_buffer_indent(b, indent + 1);
            format_buffer_append(b, "fields:\n");
            format_node_chain(b, scratch, n->as.struct_decl.fields, indent + 2);
            break;

        case NODE_ENUM_DECL:
            format_token_field(b, scratch, indent + 1, "name", n->as.enum_decl.name);
            format_buffer_indent(b, indent + 1);
            format_buffer_append_fmt(b, "variant_count: %zu\n", n->as.enum_decl.variant_count);
            format_buffer_indent(b, indent + 1);
            format_buffer_append(b, "variants:\n");
            format_node_chain(b, scratch, n->as.enum_decl.variants, indent + 2);
            break;

        case NODE_TYPE_DECL:
            format_token_field(b, scratch, indent + 1, "alias_name", n->as.type_decl.alias_name);
            format_type_field(b, indent + 1, "target_type", &n->as.type_decl.target_type);
            break;

        case NODE_BLOCK:
            format_node_array_field(b, scratch, indent + 1, "statements",
                                    n->as.block.statements, n->as.block.count);
            break;
        case NODE_IF:
            format_node_field(b, scratch, indent + 1, "condition", n->as.if_stmt.condition);
            format_node_field(b, scratch, indent + 1, "then_branch", n->as.if_stmt.then_branch);
            format_node_field(b, scratch, indent + 1, "else_branch", n->as.if_stmt.else_branch);
            break;
        case NODE_WHILE:
            format_node_field(b, scratch, indent + 1, "condition", n->as.while_stmt.condition);
            format_node_field(b, scratch, indent + 1, "body", n->as.while_stmt.body);
            break;
        case NODE_FOR:
            format_node_field(b, scratch, indent + 1, "init", n->as.for_stmt.init);
            format_node_field(b, scratch, indent + 1, "condition", n->as.for_stmt.condition);
            format_node_field(b, scratch, indent + 1, "increment", n->as.for_stmt.increment);
            format_node_field(b, scratch, indent + 1, "body", n->as.for_stmt.body);
            break;
        case NODE_RETURN:
            format_node_field(b, scratch, indent + 1, "expr", n->as.return_stmt.expr);
            break;
        case NODE_DEFER:
            format_node_field(b, scratch, indent + 1, "stmt", n->as.defer_stmt.stmt);
            break;
        case NODE_EXPR_STMT:
            format_node_field(b, scratch, indent + 1, "expr", n->as.expr_stmt.expr);
            break;
        case NODE_BREAK:
        case NODE_CONTINUE:
            break;
        case NODE_IMPORT:
            format_buffer_indent(b, indent + 1);
            format_buffer_append(b, "path: ");
            format_buffer_append_n(b, n->as.import_stmt.path.data, n->as.import_stmt.path.len);
            format_buffer_append(b, "\n");
            break;
        case NODE_SWITCH:
            format_node_field(b, scratch, indent + 1, "expr", n->as.switch_stmt.expr);
            format_buffer_indent(b, indent + 1);
            format_buffer_append_fmt(b, "case_count: %zu\n", n->as.switch_stmt.case_count);
            format_buffer_indent(b, indent + 1);
            format_buffer_append(b, "cases:\n");
            format_node_chain(b, scratch, n->as.switch_stmt.cases, indent + 2);
            break;
        case NODE_CASE:
            format_node_field(b, scratch, indent + 1, "value", n->as.case_stmt.value);
            format_node_field(b, scratch, indent + 1, "body", n->as.case_stmt.body);
            break;

        case NODE_LITERAL:
            format_token_field(b, scratch, indent + 1, "token", n->as.literal.token);
            break;
        case NODE_IDENTIFIER:
            format_token_field(b, scratch, indent + 1, "name", n->as.identifier.name);
            break;
        case NODE_BINOP: {
            Token op = {.type = n->as.binop.op};
            format_token_field(b, scratch, indent + 1, "operator", op);
            format_node_field(b, scratch, indent + 1, "left", n->as.binop.left);
            format_node_field(b, scratch, indent + 1, "right", n->as.binop.right);
            break;
        }
        case NODE_UNARY: {
            Token op = {.type = n->as.unary.op};
            format_token_field(b, scratch, indent + 1, "operator", op);
            format_node_field(b, scratch, indent + 1, "operand", n->as.unary.operand);
            break;
        }
        case NODE_CALL:
            format_node_field(b, scratch, indent + 1, "callee", n->as.call.callee);
            format_node_array_field(b, scratch, indent + 1, "args",
                                    n->as.call.args, n->as.call.arg_count);
            break;
        case NODE_MEMBER_ACCESS:
            format_node_field(b, scratch, indent + 1, "object", n->as.member_access.object);
            format_token_field(b, scratch, indent + 1, "property", n->as.member_access.property);
            break;
        case NODE_INDEX:
            format_node_field(b, scratch, indent + 1, "array", n->as.index.array);
            format_node_field(b, scratch, indent + 1, "index", n->as.index.index);
            break;
        case NODE_STRUCT_INIT:
            format_token_field(b, scratch, indent + 1, "type_name", n->as.struct_init.type_name);
            format_buffer_indent(b, indent + 1);
            format_buffer_append_fmt(b, "fields (count=%zu):\n", n->as.struct_init.field_count);
            for (size_t i = 0; i < n->as.struct_init.field_count; i++) {
                if (n->as.struct_init.field_names) {
                    format_token_field(b, scratch, indent + 2, "name", n->as.struct_init.field_names[i]);
                } else {
                    format_buffer_indent(b, indent + 2);
                    format_buffer_append(b, "name: <missing>\n");
                }
                format_node_field(b, scratch, indent + 2, "value",
                                  n->as.struct_init.field_values
                                      ? n->as.struct_init.field_values[i] : NULL);
            }
            break;
        case NODE_ARRAY_INIT:
            format_node_array_field(b, scratch, indent + 1, "elements",
                                    n->as.array_init.elements, n->as.array_init.element_count);
            break;
        case NODE_COUNT:
            format_buffer_indent(b, indent + 1);
            format_buffer_append(b, "<sentinel>\n");
            break;
    }
}

char *format_node(Arena *a, Node *n) {
    FormatBuffer b = {0};
    Arena scratch = arena_new(1024);

    if (n) format_node_chain(&b, &scratch, n, 0);
    else format_buffer_append(&b, "<null node>\n");

    arena_free(&scratch);
    if (b.failed) {
        format_buffer_free(&b);
        return NULL;
    }

    /* Formatting returns text, not a line-printing side effect. */
    if (b.len > 0 && b.data[b.len - 1] == '\n') b.len--;

    char *result = (char *)arena_alloc(a, b.len + 1);
    if (!result) {
        format_buffer_free(&b);
        return NULL;
    }

    memcpy(result, b.data, b.len);
    result[b.len] = '\0';
    format_buffer_free(&b);
    return result;
}

const char *format_bool(bool value) {
    return value ? "true" : "false";
}

char *format_data_type(Arena *a, DataType *type) {
    static const char *kind_names[] = {
        [TYPE_VOID] = "void",
        [TYPE_NULL] = "null",
        [TYPE_INT] = "int",
        [TYPE_FLOAT] = "float",
        [TYPE_CHAR] = "char",
        [TYPE_BOOL] = "bool",
        [TYPE_CUSTOM] = "custom",
    };
    bool valid_kind = type && type->kind >= 0 &&
                      type->kind < (int)(sizeof(kind_names) / sizeof(kind_names[0]));
    const char *name = valid_kind && type->kind != TYPE_CUSTOM
                           ? kind_names[type->kind] : NULL;
    size_t name_len = name ? strlen(name) : (type ? type->name.len : 0);
    int pointer_depth = type ? type->pointer_depth : 0;

    char *buf = (char *)arena_alloc(a, name_len + pointer_depth + 1);
    if (!buf) return NULL;

    if (name) memcpy(buf, name, name_len);
    else if (type && type->name.data) memcpy(buf, type->name.data, name_len);
    for (int i = 0; i < pointer_depth; i++) {
        buf[name_len + i] = '*';
    }
    buf[name_len + pointer_depth] = '\0';

    return buf;
}
