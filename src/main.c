#include "core/lexer.h"
#include "core/parser.h"

#include "arena.h"
#include "utils.h"

#include <stdio.h>

int main(int argc, char **argv) {
    if (argc < 2) {
        LOG(FATAL, "Source file name is required");
        exit(1);
    }

    const char *file_path = argv[1];
    StringStream ss = read_file(file_path);

    Arena a = arena_new(1024*1024);
    Arena format_arena = arena_new(1024);
    Lexer lex = lexer_new(ss, &a);
    Parser p = parser_new(&lex, &a, file_path);

    // Token cur = lexer_next(&lex);
    // while (cur.type != TOKEN_UNKNOWN && cur.type != TOKEN_EOF) {
    //     printf("%s\n", format_token(&a, cur));
    //     cur = lexer_next(&lex);
    // }

    Node *cur = parser_next(&p);
    while (cur) {
        char *formatted = format_node(&format_arena, cur);
        if (formatted) printf("%s\n", formatted);
        arena_clear(&format_arena);
        cur = parser_next(&p);
    }

    arena_free(&format_arena);
    arena_free(&a);
    return 0;
}
