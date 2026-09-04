# Decorators — Plan

Status: Unscheduled

Decorators are a possible future AST-transformation feature. Their syntax, execution order, capabilities, diagnostics, source-location behavior, and interaction with type checking are not part of the current language specification.

Decorators are intended to replace textual macro use cases with structured transformations that
operate on typed/compiler-owned nodes. They are not a C preprocessor: Arqis source will not gain
`#define`, `#include`, or textual conditional-compilation directives.
