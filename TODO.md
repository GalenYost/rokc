# Arqis 0.1.0 TODO

This checklist tracks implementation work for the 0.1.0 front-end release. The release must
process a single `.qis` source file, validate it, and report useful diagnostics; native executable
generation is deferred to 0.2.0.

## Lexer and tokens

- [ ] Finalize keyword and reserved-feature token handling.
- [ ] Support numeric literals, separators, bases, and floating-point forms.
- [ ] Support string and one-byte character literals with escapes.
- [ ] Validate comments, unterminated literals, and invalid characters.
- [ ] Preserve accurate source locations on every token.

## AST and parser

- [ ] Complete AST node coverage for every rule in `docs/grammar.md`.
- [ ] Implement declarations, prototypes, aliases, structs, unions, and enums.
- [ ] Implement type-first pointers and array syntax, including fixed arrays and VLAs.
- [ ] Implement function-pointer declarations and parameters.
- [ ] Implement expressions with the documented precedence and associativity.
- [ ] Implement statements, loops, switch statements, switch expressions, and `defer`.
- [ ] Implement C-style casts, `sizeof`, and `alignof` syntax.
- [ ] Implement panic-mode parser recovery at safe synchronization points.

## Semantic analysis

- [ ] Build lexical scopes and enforce declaration/name-collision rules.
- [ ] Resolve types and transparent aliases.
- [ ] Enforce strict conversions and contextual literal typing.
- [ ] Validate pointer, array, enum, union, and aggregate rules.
- [ ] Validate `const`, `static`, global initialization, and visibility rules.
- [ ] Enforce boolean conditions and operator operand compatibility.
- [ ] Enforce non-`void` return-path requirements.
- [ ] Validate method receivers, static methods, and member-call dispatch.

## Diagnostics and CLI

- [ ] Add structured diagnostics with source spans, notes, and severity.
- [ ] Distinguish recoverable source errors from fatal/internal compiler errors.
- [ ] Implement `arqis check <source.qis>`.
- [ ] Implement `arqis dump-ast <source.qis>`.
- [ ] Implement `arqis help` and `arqis version`.
- [ ] Remove hardcoded source-file assumptions.

## Tests

- [ ] Add lexer tests for valid and invalid token streams.
- [ ] Add parser coverage for every grammar rule.
- [ ] Add semantic tests for valid and invalid programs.
- [ ] Add diagnostic recovery tests with multiple independent errors.
- [ ] Add end-to-end single-file `check` tests.

## Release gate

- [ ] `make build` succeeds without new warnings.
- [ ] Valid 0.1 programs pass `arqis check`.
- [ ] Invalid programs fail with useful diagnostics and nonzero status.
- [ ] Every 0.1 grammar rule has parser and test coverage.
