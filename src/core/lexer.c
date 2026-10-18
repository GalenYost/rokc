#include "lexer.h"
#include "../arena.h"
#include "../utils.h"

#include <stdbool.h>
#include <string.h>

static bool is_at_end(Lexer *lex);
static bool lexer_match(Lexer *lex, char expected);

static void skip_whitespace(Lexer *lex);

static bool is_digit(char c);

static char lexer_peek(Lexer *lex);
static char lexer_peek_next(Lexer *lex);
static char lexer_advance(Lexer *lex);

static Token lex_string(Lexer *lex);
static Token lex_char(Lexer *lex);

static Token identify_symbols_and_operators(Lexer *lex);
static Token identify_keyword(Lexer *lex);
static Token identify_number(Lexer *lex);

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
    lex->current++;

    size_t max_len = 0;
    const char *temp = lex->current;

    while (*temp != '"' && *temp != '\0') {
        if (*temp == '\\' && *(temp + 1) != '\0') {
            temp++;
            max_len++;
        }
        max_len++; 
        temp++; 
    }

    char *dest = (char *)arena_alloc(lex->arena, max_len + 1);
    size_t actual_len = 0;

    while (!is_at_end(lex) && *lex->current != '"') {
        if (*lex->current == '\n') {
            lex->line++;
            lex->col = 1;
        }

        if (*lex->current == '\\') {
            lex->current++;
            
            switch (*lex->current) {
                case 'n': dest[actual_len++] = '\n'; break;
                case 't': dest[actual_len++] = '\t'; break;
                case 'r': dest[actual_len++] = '\r'; break;
                case '\\': dest[actual_len++] = '\\'; break;
                case '"': dest[actual_len++] = '"'; break;
                case '0': dest[actual_len++] = '\0'; break;
                default:
                    dest[actual_len++] = *lex->current; 
                    break;
            }
            lex->current++;
            lex->col += 2;
        } else {
            dest[actual_len++] = *lex->current++;
            lex->col++;
        }
    }

    if (is_at_end(lex)) {
        return (Token) {
            .type = TOKEN_UNKNOWN,
            .view = {0},
            .line = lex->line,
            .col = lex->col,
        };
    }

    lex->current++;
    lex->col++;

    Token t = {
        .type = TOKEN_LIT_STR,
        .view = (StringStream) {
            .data = dest,
            .len = actual_len
        },
        .line = lex->line,
        .col = lex->col,
    };
    return t;
}

static Token lex_char(Lexer *lex) {
    lexer_advance(lex);

    if (lexer_peek(lex) == '\\') {
        lexer_advance(lex);
        lexer_advance(lex);
    } else if (lexer_peek(lex) != '\'') {
        lexer_advance(lex);
    }

    if (!lexer_match(lex, '\'')) {
        Token err = { .type = TOKEN_UNKNOWN, .line = lex->line, .col = lex->col };
        err.view = (StringStream){
            .data = (char*)lex->start,
            .len = (size_t)(lex->current - lex->start)
        };
        return err;
    }

    Token t = {
        .type = TOKEN_LIT_CHAR,
        .line = lex->line,
        .col = lex->col,
    };
    t.view = (StringStream){
        .data = (char*)lex->start,
        .len = (size_t)(lex->current - lex->start)
    };

    return t;
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
            break;
        case 3:
            if (strncmp(lex->start, "for", 3) == 0) type = TOKEN_FOR;
            else if (strncmp(lex->start, "int", 3) == 0) type = TOKEN_INT;
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
            break;
        case 5:
            if (strncmp(lex->start, "while", 5) == 0) type = TOKEN_WHILE;
            else if (strncmp(lex->start, "break", 5) == 0) type = TOKEN_BREAK;
            else if (strncmp(lex->start, "defer", 5) == 0) type = TOKEN_DEFER;
            else if (strncmp(lex->start, "const", 5) == 0) type = TOKEN_CONST;
            else if (strncmp(lex->start, "float", 5) == 0) type = TOKEN_FLOAT;
            else if (strncmp(lex->start, "false", 5) == 0) type = TOKEN_FALSE;
            break;
        case 6:
            if (strncmp(lex->start, "return", 6) == 0) type = TOKEN_RETURN;
            else if (strncmp(lex->start, "import", 6) == 0) type = TOKEN_IMPORT;
            else if (strncmp(lex->start, "static", 6) == 0) type = TOKEN_STATIC;
            else if (strncmp(lex->start, "struct", 6) == 0) type = TOKEN_STRUCT;
            else if (strncmp(lex->start, "switch", 6) == 0) type = TOKEN_SWITCH;
            break;
        case 7:
            if (strncmp(lex->start, "default", 7) == 0) type = TOKEN_DEFAULT;
            break;
        case 8:
            if (strncmp(lex->start, "continue", 8) == 0) type = TOKEN_CONTINUE;
            else if (strncmp(lex->start, "comptime", 8) == 0) type = TOKEN_COMPTIME;
            break;
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
    TokenType type = TOKEN_INT;
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
            type = TOKEN_FLOAT;
            lexer_advance(lex);
            scan_digits(lex, 10, &valid);
        }

        if (lexer_peek(lex) == 'e' || lexer_peek(lex) == 'E') {
            type = TOKEN_FLOAT;
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
