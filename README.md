Arqis is a fast, embeddable, statically typed, compiled programming language.

## General Info

| Property | Value |
|----------|-------|
| **Name** | Arqis — coined name chosen for distinctiveness |
| **Extension** | `.qis` — compact suffix derived from Arqis |
| **Syntax** | Mostly C — simple, transparent, nothing hidden or abstracted away |

## Plans on Improving C

- **No header files** — they are meaningless
- **Decorators** — built-in syntax allowing to extend functionality and meaning of anything you want
- **Simple by design** — avoid the complexity of OOP from languages like C++ or Java; if you want it, you keep it simple
- **`defer`** — the dream of C programmers; simplifies code readability while keeping its idea the same
- **Type aliasing** — create standalone types to help develop complex architectures with more readable type names
- **Custom allocations** — a simplified version of `malloc` with optional args for allocation behavior
- **`import`** — since there are no macros, you get a separate keyword to include modules
- **Build scripts/recipes** — the build system should be simple and involve nothing other than the language compiler itself
- **Explicit `null` values** — available for pointer-oriented APIs; Arqis does not attempt to provide Rust-like memory safety
- **`comptime`** — mark things evaluable during compilation so functions or expressions can be reduced in the final binary
- **Generics** — C lacks convenient code generation for reusable typed containers; Arqis plans to provide this without requiring `void*`-style APIs
- **`std`** — a huge collection of modules; instead of `stdio`/`stdlib`, you import `std.math` and use `std.math.floor`
- **String interpolation** — readable strings without memorizing format flags and their unique features
- **Visibility and exports** — declarations are public by default; module-level `static` prevents export, while struct-level `static` controls shared storage rather than privacy
- **C-like memory model** — raw pointers and built-in arrays remain directly usable and are not automatically checked for nullability, lifetime, or bounds; higher-level checked containers will be provided by the standard library

## Documentation & Deep Dives

For detailed explanations of Arqis's design philosophy and internal features, please check out the `docs/` folder:

*   [Language Features](docs/features.md): Deep dives into Decorators, Memory Allocators, Visibilty/Exports, and OOP differences.
*   [Compiler Phases](docs/compiler.md): Step-by-step breakdown of how the Arqis compiler works internally.
*   [Grammar](docs/grammar.md): Formal EBNF grammar definitions for the Arqis language syntax.
*   [Logging System](docs/logging.md): Overview of the compiler's logging configuration, log levels, and conventions.
*   [Build System](docs/build.md): Details on the nob-based build system (constants, structs, functions).

### Naming
In Arqis just like in C you can pretty much do whatever you want, no one will force you to write what is "better", but there are some suggestions that will help in development:

1. **Variables and Functions**:
   - Variables (local and global) should use `snake_case`.
   - Functions should use `snake_case` (e.g., `print_hello()`).

2. **Constants**:
   - Global constants should be named `SCREAMING_SNAKE_CASE`.
   - Local constants (if contextually small) can be named `snake_case`.

3. **Types and Structs**:
   - Structs, Enums, and Type Aliases should use `PascalCase` (e.g., `AppState`, `HashMap`, `Player`).

4. **Decorators**:
   - Built-in and custom decorators should use `PascalCase` to distinguish them from standard functions (e.g., `@ExportAll`, `@Serializable`).

## TODO

### General
- [ ] Implement test runner (`nob.c`)
- [ ] Implement proper node printing with names / identifiers / other things (`src/utils.c`)

### Parser
- [ ] Implement variable / function declaration parsing, `NODE_VAR_DECL` / `NODE_FUNC_DECL` (`parse_var_or_func_decl`, `src/core/parser.c`)

#### Statements
- [ ] Parse blocks, `NODE_BLOCK`
- [ ] Parse if statements, `NODE_IF`
- [ ] Parse while statements, `NODE_WHILE`
- [ ] Parse for statements, `NODE_FOR`
- [ ] Parse return statements, `NODE_RETURN`
- [ ] Parse break statements, `NODE_BREAK`
- [ ] Parse continue statements, `NODE_CONTINUE`
- [ ] Parse defer statements, `NODE_DEFER`
- [ ] Parse expression statements, `NODE_EXPR_STMT`
- [ ] Parse switch statements, `NODE_SWITCH`
- [ ] Parse case statements, `NODE_CASE`

#### Expressions
- [ ] Parse literals, `NODE_LITERAL`
- [ ] Parse identifiers, `NODE_IDENTIFIER`
- [ ] Parse binary expressions, `NODE_BINOP`
- [ ] Parse unary expressions, `NODE_UNARY`
- [ ] Parse function calls, `NODE_CALL`
- [ ] Parse member access, `NODE_MEMBER_ACCESS`
- [ ] Parse index expressions, `NODE_INDEX`
- [ ] Parse struct initialization, `NODE_STRUCT_INIT`
- [ ] Parse array initialization, `NODE_ARRAY_INIT`
