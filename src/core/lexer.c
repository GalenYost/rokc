#include "lexer.h"
#include "../arena.h"
#include "../utils.h"

#include <stdbool.h>
#include <string.h>

static bool is_at_end(Lexer *lex);
static bool lexer_match(Lexer *lex, char expected);

static void skip_whitespace(Lexer *lex);

static bool is_digit(char c);
static bool decode_escape(char escape, char *decoded);

static char lexer_peek(Lexer *lex);
static char lexer_peek_next(Lexer *lex);
static char lexer_advance(Lexer *lex);

static Token lex_string(Lexer *lex);
static Token lex_char(Lexer *lex);

static Token identify_symbols_and_operators(Lexer *lex);
static Token identify_keyword(Lexer *lex);
static Token identify_number(Lexer *lex);

static TokenType identify_numeric_type(const char *name, size_t len);

static bool is_at_end(Lexer *lex) {
    return *lex->current == '\0';
}

static bool lexer_match(Lexer *lex, char expected) {
    if (is_at_end(lex)) return false;
    if (*lex->current != expected) return false;

    lex->current++;
    lex->col++;
    return true;
}

static void skip_whitespace(Lexer *lex) {
    while (true) {
        char c = lexer_peek(lex);
        switch (c) {
            case ' ':
            case '\r':
            case '\t':
            case '\n': {
                lexer_advance(lex);
                break;
            }
            case '/':
                if (lex->current[1] == '/') {
                    while (lexer_peek(lex) != '\n' && !is_at_end(lex)) {
                        lexer_advance(lex);
                    }
                } else return;
                break;
            default: return;
        }
    }
}

static bool is_digit(char c) {
    return c >= '0' && c <= '9';
}

static bool is_digit_for_base(char c, unsigned base) {
    if (c >= '0' && c <= '9') return (unsigned)(c - '0') < base;
    if (base == 16 && c >= 'a' && c <= 'f') return true;
    if (base == 16 && c >= 'A' && c <= 'F') return true;
    return false;
}

static size_t scan_digits(Lexer *lex, unsigned base, bool *valid) {
    size_t count = 0;
    while (is_digit_for_base(lexer_peek(lex), base) || lexer_peek(lex) == '_') {
        char c = lexer_peek(lex);
        if (c == '_') {
            char previous = lex->current > lex->start ? lex->current[-1] : '\0';
            char next = lexer_peek_next(lex);
            if (!is_digit_for_base(previous, base) || !is_digit_for_base(next, base)) {
                *valid = false;
            }
        } else {
            count++;
        }
        lexer_advance(lex);
    }
    return count;
}

static char lexer_peek(Lexer *lex) {
    if (is_at_end(lex)) {
        return '\0';
    }
    return *lex->current;
}

static char lexer_peek_next(Lexer *lex) {
    if (is_at_end(lex)) return '\0';
    if (lex->current[1] == '\0') return '\0';
    return lex->current[1];
}

static char lexer_advance(Lexer *lex) {
    if (is_at_end(lex)) {
        return '\0';
    }

    const char c = *lex->current++;

    if (c == '\n') {
        lex->line++;
        lex->col = 1;
    } else lex->col++;

    return c;
}

static Token lex_string(Lexer *lex) {
    lexer_advance(lex); // opening quote

    const char *content = lex->current;
    const char *scan = content;
    bool valid = true;
    bool has_interpolation = false;
    bool closed = false;

    while (*scan != '\0') {
        if (*scan == '"') {
            closed = true;
            break;
        }
        if (*scan == '\n') {
            valid = false;
            break;
        }
        if (*scan == '\\') {
            char ignored;
            if (scan[1] == '\n') {
                valid = false;
                scan++;
                break;
            }
            if (scan[1] == '\0' || !decode_escape(scan[1], &ignored)) {
                valid = false;
            }
            if (scan[1] == '\0') {
                scan++;
                break;
            }
            scan += 2;
            continue;
        }
        if (*scan == '$' && scan[1] == '{') has_interpolation = true;
        scan++;
    }

    // Advance the lexer to the end of the content without crossing an invalid
    // unescaped newline, so normal whitespace handling can recover afterward.
    while (lex->current < scan) lexer_advance(lex);

    if (!closed) valid = false;
    const char *content_end = scan;
    if (closed) lexer_advance(lex); // closing quote

    if (!valid) {
        return (Token) {
            .type = TOKEN_UNKNOWN,
            .view = {
                .data = (char *)lex->start,
                .len = (size_t)(lex->current - lex->start),
            },
            .line = lex->line,
            .col = lex->col,
        };
    }

    size_t source_len = (size_t)(content_end - content);
    char *dest = (char *)arena_alloc(lex->arena, source_len + 1);
    if (!dest) {
        return (Token) { .type = TOKEN_UNKNOWN, .line = lex->line, .col = lex->col };
    }

    size_t decoded_len = 0;
    for (const char *cursor = content; cursor < content_end;) {
        if (*cursor == '\\') {
            decode_escape(cursor[1], &dest[decoded_len++]);
            cursor += 2;
        } else {
            dest[decoded_len++] = *cursor++;
        }
    }
    dest[decoded_len] = '\0';

    return (Token) {
        .type = has_interpolation ? TOKEN_LIT_STR_INTERP : TOKEN_LIT_STR,
        .view = { .data = dest, .len = decoded_len },
        .line = lex->line,
        .col = lex->col,
    };
}

static Token lex_char(Lexer *lex) {
    lexer_advance(lex); // opening quote

    char decoded = '\0';
    bool valid = true;

    if (is_at_end(lex) || lexer_peek(lex) == '\n' || lexer_peek(lex) == '\'') {
        valid = false;
    } else if (lexer_peek(lex) == '\\') {
        lexer_advance(lex);
        if (is_at_end(lex) || lexer_peek(lex) == '\n' ||
            !decode_escape(lexer_peek(lex), &decoded)) {
            valid = false;
            if (!is_at_end(lex) && lexer_peek(lex) != '\n') lexer_advance(lex);
        } else {
            lexer_advance(lex);
        }
    } else {
        decoded = lexer_advance(lex);
    }

    if (!lexer_match(lex, '\'')) {
        valid = false;
        while (!is_at_end(lex) && lexer_peek(lex) != '\n' && lexer_peek(lex) != '\'') {
            lexer_advance(lex);
        }
        lexer_match(lex, '\'');
    }

    if (!valid) {
        return (Token) {
            .type = TOKEN_UNKNOWN,
            .view = {
                .data = (char *)lex->start,
                .len = (size_t)(lex->current - lex->start),
            },
            .line = lex->line,
            .col = lex->col,
        };
    }

    char *value = (char *)arena_alloc(lex->arena, 1);
    if (!value) return (Token) { .type = TOKEN_UNKNOWN, .line = lex->line, .col = lex->col };
    *value = decoded;

    return (Token) {
        .type = TOKEN_LIT_CHAR,
        .view = { .data = value, .len = 1 },
        .line = lex->line,
        .col = lex->col,
    };
}

static bool decode_escape(char escape, char *decoded) {
    switch (escape) {
        case 'n': *decoded = '\n'; return true;
        case 't': *decoded = '\t'; return true;
        case 'r': *decoded = '\r'; return true;
        case '\\': *decoded = '\\'; return true;
        case '"': *decoded = '"'; return true;
        case '\'': *decoded = '\''; return true;
        case '0': *decoded = '\0'; return true;
        default: return false;
    }
}

static Token identify_symbols_and_operators(Lexer *lex) {
    Token t = {
        .type = TOKEN_UNKNOWN,
        .line = lex->line,
        .col = lex->col,
    };

    char c = lexer_advance(lex);

    switch (c) {
        case '+': {
            if (lexer_match(lex, '+')) {
                t.type = TOKEN_PLUS_PLUS;
            } else if (lexer_match(lex, '=')) {
                t.type = TOKEN_PLUS_EQ;
            } else t.type = TOKEN_PLUS;
            break;
        }
        case '-': {
            if (lexer_match(lex, '-')) {
                t.type = TOKEN_MINUS_MINUS;
            } else if (lexer_match(lex, '=')) {
                t.type = TOKEN_MINUS_EQ;
            } else t.type = TOKEN_MINUS;
            break;
        }
        case '*': {
            if (lexer_match(lex, '*')) {
                t.type = TOKEN_POW;
            } else if (lexer_match(lex, '=')) {
                t.type = TOKEN_MULTI_EQ;
            } else t.type = TOKEN_STAR;
            break;
        }
        case '/': {
            if (lexer_match(lex, '=')) {
                t.type = TOKEN_DIV_EQ;
            } else t.type = TOKEN_SLASH;
            break;
        }
        case '(': {
            t.type = TOKEN_PAREN_LEFT;
            break;
        }
        case ')': {
            t.type = TOKEN_PAREN_RIGHT;
            break;
        }
        case '[': {
            t.type = TOKEN_BRACE_LEFT;
            break;
        }
        case ']': {
            t.type = TOKEN_BRACE_RIGHT;
            break;
        }
        case '{': {
            t.type = TOKEN_CURLY_LEFT;
            break;
        }
        case '}': {
            t.type = TOKEN_CURLY_RIGHT;
            break;
        }
        case '=': {
            if (lexer_match(lex, '=')) {
                t.type = TOKEN_EQ_EQ;
            } else t.type = TOKEN_EQ;
            break;
        }
        case '!': {
            if (lexer_match(lex, '=')) {
                t.type = TOKEN_NOT_EQ;
            } else t.type = TOKEN_BANG;
            break;
        }
        case '|': {
            if (lexer_match(lex, '|')) {
                t.type = TOKEN_LOGICAL_OR;
            } else if (lexer_match(lex, '=')) {
                t.type = TOKEN_BIT_OR_EQ;
            } else t.type = TOKEN_BIT_OR;
            break;
        }
        case '&': {
            if (lexer_match(lex, '&')) {
                t.type = TOKEN_LOGICAL_AND;
            } else if (lexer_match(lex, '=')) {
                t.type = TOKEN_BIT_AND_EQ;
            } else t.type = TOKEN_BIT_AND;
            break;
        }
        case '>': {
            if (lexer_match(lex, '>')) {
                if (lexer_match(lex, '=')) {
                    t.type = TOKEN_SHIFT_RIGHT_EQ;
                } else t.type = TOKEN_SHIFT_RIGHT;
            } else if (lexer_match(lex, '=')) {
                t.type = TOKEN_GT_EQ;
            } else t.type = TOKEN_GT;
            break;
        }
        case '<': {
            if (lexer_match(lex, '<')) {
                if (lexer_match(lex, '=')) {
                    t.type = TOKEN_SHIFT_LEFT_EQ;
                } else t.type = TOKEN_SHIFT_LEFT;
            } else if (lexer_match(lex, '=')) {
                t.type = TOKEN_LT_EQ;
            } else t.type = TOKEN_LT;
            break;
        }
        case '^': {
            if (lexer_match(lex, '=')) {
                t.type = TOKEN_XOR_EQ;
            } else t.type = TOKEN_XOR;
            break;
        }
        case '.': {
            t.type = TOKEN_DOT;
            break;
        }
        case '%': {
            t.type = TOKEN_PERCENT;
            break;
        }
        case '$': {
            t.type = TOKEN_DOLLAR;
            break;
        }
        case '#': {
            t.type = TOKEN_HASH;
            break;
        }
        case '\'': {
            t.type = TOKEN_QUOTE;
            break;
        }
        case '"': {
            t.type = TOKEN_DOUBLE_QUOTE;
            break;
        }
        case '?': {
            t.type = TOKEN_QUESTION;
            break;
        }
        case ',': {
            t.type = TOKEN_COMMA;
            break;
        }
        case ';': {
            t.type = TOKEN_SEMI;
            break;
        }
        case ':': {
            t.type = TOKEN_COLON;
            break;
        }
        case '@': {
            t.type = TOKEN_AT;
            break;
        }
    };

    t.view = (StringStream){
        .data = (char*)lex->start,
        .len = (size_t)(lex->current - lex->start),
    };

    return t;
}

static Token identify_keyword(Lexer *lex) {
    while (true) {
        char c = lexer_peek(lex);
        if ((c >= 'a' && c <= 'z') || 
            (c >= 'A' && c <= 'Z') || 
            (c >= '0' && c <= '9') || 
             c == '_') {
            lexer_advance(lex);
        } else {
            break;
        }
    }

    size_t len = (size_t)(lex->current - lex->start);

    TokenType type = TOKEN_IDENT; 

    switch (len) {
        case 2:
            if (strncmp(lex->start, "if", 2) == 0) type = TOKEN_IF;
            if (strncmp(lex->start, "as", 2) == 0) type = TOKEN_AS;
            if (strncmp(lex->start, "do", 2) == 0) type = TOKEN_DO;
            break;
        case 3:
            if (strncmp(lex->start, "for", 3) == 0) type = TOKEN_FOR;
            else if (strncmp(lex->start, "int", 3) == 0) type = TOKEN_SIGNED_INT;
            break;
        case 4:
            if (strncmp(lex->start, "else", 4) == 0) type = TOKEN_ELSE;
            else if (strncmp(lex->start, "true", 4) == 0) type = TOKEN_TRUE;
            else if (strncmp(lex->start, "void", 4) == 0) type = TOKEN_VOID;
            else if (strncmp(lex->start, "type", 4) == 0) type = TOKEN_TYPE;
            else if (strncmp(lex->start, "enum", 4) == 0) type = TOKEN_ENUM;
            else if (strncmp(lex->start, "char", 4) == 0) type = TOKEN_CHAR;
            else if (strncmp(lex->start, "null", 4) == 0) type = TOKEN_NULL;
            else if (strncmp(lex->start, "case", 4) == 0) type = TOKEN_CASE;
            else if (strncmp(lex->start, "bool", 4) == 0) type = TOKEN_BOOL;
            break;
        case 5:
            if (strncmp(lex->start, "while", 5) == 0) type = TOKEN_WHILE;
            else if (strncmp(lex->start, "break", 5) == 0) type = TOKEN_BREAK;
            else if (strncmp(lex->start, "defer", 5) == 0) type = TOKEN_DEFER;
            else if (strncmp(lex->start, "const", 5) == 0) type = TOKEN_CONST;
            else if (strncmp(lex->start, "float", 5) == 0) type = TOKEN_FLOAT;
            else if (strncmp(lex->start, "false", 5) == 0) type = TOKEN_FALSE;
            else if (strncmp(lex->start, "union", 5) == 0) type = TOKEN_UNION;
            break;
        case 6:
            if (strncmp(lex->start, "return", 6) == 0) type = TOKEN_RETURN;
            else if (strncmp(lex->start, "import", 6) == 0) type = TOKEN_IMPORT;
            else if (strncmp(lex->start, "static", 6) == 0) type = TOKEN_STATIC;
            else if (strncmp(lex->start, "struct", 6) == 0) type = TOKEN_STRUCT;
            else if (strncmp(lex->start, "switch", 6) == 0) type = TOKEN_SWITCH;
            else if (strncmp(lex->start, "sizeof", 6) == 0) type = TOKEN_SIZEOF;
            break;
        case 7:
            if (strncmp(lex->start, "default", 7) == 0) type = TOKEN_DEFAULT;
            else if (strncmp(lex->start, "alignof", 7) == 0) type = TOKEN_ALIGNOF;
            break;
        case 8:
            if (strncmp(lex->start, "continue", 8) == 0) type = TOKEN_CONTINUE;
            else if (strncmp(lex->start, "comptime", 8) == 0) type = TOKEN_COMPTIME;
            break;
    }

    if (type == TOKEN_IDENT) {
        type = identify_numeric_type(lex->start, len);
    }

    Token t = {
        .type = type,
        .line = lex->line,
        .col = lex->col,
    };
    t.view = (StringStream){
        .data = (char*)lex->start,
        .len = len
    };

    return t;
}

static Token identify_number(Lexer *lex) {
    TokenType type = TOKEN_LIT_INT;
    bool valid = true;
    unsigned base = 10;

    if (lex->current[0] == '0') {
        switch (lex->current[1]) {
            case 'x': base = 16; break;
            case 'b': base = 2; break;
            case 'o': base = 8; break;
        }
    }

    if (base != 10) {
        lexer_advance(lex);
        lexer_advance(lex);
        if (scan_digits(lex, base, &valid) == 0) valid = false;
    } else {
        scan_digits(lex, 10, &valid);

        if (lexer_peek(lex) == '.' && is_digit(lexer_peek_next(lex))) {
            type = TOKEN_LIT_FLOAT;
            lexer_advance(lex);
            scan_digits(lex, 10, &valid);
        }

        if (lexer_peek(lex) == 'e' || lexer_peek(lex) == 'E') {
            type = TOKEN_LIT_FLOAT;
            lexer_advance(lex);
            if (lexer_peek(lex) == '+' || lexer_peek(lex) == '-') lexer_advance(lex);
            if (scan_digits(lex, 10, &valid) == 0) valid = false;
        }
    }

    // Suffixes are not part of the language yet, so a letter or separator
    // immediately following a number makes the whole token malformed.
    while ((lexer_peek(lex) >= 'a' && lexer_peek(lex) <= 'z') ||
           (lexer_peek(lex) >= 'A' && lexer_peek(lex) <= 'Z') ||
           lexer_peek(lex) == '_' || is_digit(lexer_peek(lex))) {
        valid = false;
        lexer_advance(lex);
    }

    StringStream ss = {
        .data = (char*)lex->start,
        .len = (size_t)(lex->current - lex->start)
    };

    if (!valid) {
        return (Token) {
            .type = TOKEN_UNKNOWN,
            .view = ss,
            .line = lex->line,
            .col = lex->col,
        };
    }

    Token t = {
        .type = type,
        .line = lex->line,
        .col = lex->col,
        .view = sstream_remove(lex->arena, ss, '_'),
    };

    return t;
}

static TokenType identify_numeric_type(const char *name, size_t len) {
    // supported numeric types defined here
    // lexer only recognizes generic types
    // recognizing exact size is parser's responsibility
    static const struct {
        const char *name;
        TokenType type;
    } numeric_types[] = {
        {"i8", TOKEN_SIGNED_INT},
        {"i16", TOKEN_SIGNED_INT},
        {"i32", TOKEN_SIGNED_INT},
        {"i64", TOKEN_SIGNED_INT},
        {"u8", TOKEN_UNSIGNED_INT},
        {"u16", TOKEN_UNSIGNED_INT},
        {"u32", TOKEN_UNSIGNED_INT},
        {"u64", TOKEN_UNSIGNED_INT},
        {"f32", TOKEN_FLOAT},
        {"f64", TOKEN_FLOAT},
        {"isize", TOKEN_SIGNED_INT},
        {"usize", TOKEN_UNSIGNED_INT},
    };

    for (size_t i = 0; i < sizeof(numeric_types) / sizeof(numeric_types[0]); i++) {
        size_t candidate_len = strlen(numeric_types[i].name);
        if (len == candidate_len && memcmp(name, numeric_types[i].name, len) == 0) {
            return numeric_types[i].type;
        }
    }

    return TOKEN_IDENT;
}

Lexer lexer_new(StringStream ss, Arena *a) {
    return (Lexer) {
        .start = ss.data,
        .current = ss.data,
        .line = 1,
        .col = 1,
        .arena = a,
    };
}

Token lexer_next(Lexer *lex) {
    skip_whitespace(lex);
    lex->start = lex->current;

    size_t start_line = lex->line;
    size_t start_col = lex->col;

    if (is_at_end(lex)) {
        return (Token){ .type = TOKEN_EOF, .line = lex->line, .col = lex->col, .view = {0} };
    }

    char c = lexer_peek(lex);

    Token t;
    if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_') {
        t = identify_keyword(lex);
    } else if (is_digit(c)) {
        t = identify_number(lex);
    } else if (c == '"') {
        t = lex_string(lex);
    } else if (c == '\'') {
        t = lex_char(lex);
    } else {
        t = identify_symbols_and_operators(lex);
    }

    t.line = start_line;
    t.col = start_col;
    return t;
}
