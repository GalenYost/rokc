# RokC

RokC is a fast, statically typed, compiled programming language with C-like syntax and a
deliberately simple, unsafe memory model.

## Quick facts

| Property | Value |
|----------|-------|
| **Name** | RokC |
| **Extension** | `.rok` |
| **Syntax** | Mostly C — simple, transparent, nothing hidden or abstracted away |

## Key language features

- **No header files** — declarations are imported directly
- **Regular `defer`** — LIFO cleanup without hidden ownership rules
- **Type aliasing** — transparent `type` declarations for readable names
- **Structs and methods** — member functions with `.`-only access, implicit pointer dereference
- **Enums and unions** — fieldless enum variants; C-style overlapping unions with no automatic tag
- **Function pointers** — exact-signature callbacks; no closures in 0.1
- **Switch expressions** — value-producing `switch` with implicit arm values
- **`comptime`** — compile-time constant binding (grammar-level; execution deferred to later releases)

## Current status

The project is working toward a **0.1.0 front-end release** that lexes, parses, and validates
single-file programs and reports useful diagnostics. Native executable generation is deferred to
0.2.0.

- [v0.1 grammar](docs/grammar.md) — the complete EBNF specification
- [0.1.0 release scope](docs/releases/0.1.0.md) — in-scope features, acceptance criteria
- [0.2.0 planned scope](docs/releases/0.2.0.md) — executable compiler, modules, IR
- [Implementation TODO](TODO.md) — per-area checklist for the 0.1.0 front end
- [Plans](plans/PLANS.md) — language feature plans with status per release

## Project layout

```text
src/
  main.c            entry point (hardcoded to main.rok — to be replaced with CLI)
  core/
    lexer.c         token model and lexical analysis
    parser.c        partial parser (import, struct, enum, type only)
  ast.h             node type definitions
  ast.c             node allocation (arena-based)
  arena.c           bump allocator
  utils.c           file I/O, logging, token/node formatting
docs/
  grammar.md        v0.1 EBNF grammar
  releases/
    0.1.0.md        front-end release plan
    0.2.0.md        executable compiler release plan
plans/
  PLANS.md          index of all feature plans
  ...               per-feature design documents
```

## Building

See [docs/build.md](docs/build.md) for the build system and available commands.

## Contributing

For development setup and conventions, see the [docs/](docs/) directory.
