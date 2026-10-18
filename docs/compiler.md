# RokC Compiler Phases

This document outlines the internal architecture and compilation process for the RokC compiler.

### Lexical Analysis
The Lexer takes the `.rok` source code and converts it into a stream of `Token`s.
It handles ignoring whitespace and comments, identifies keywords, identifiers, string literals, and operators.

### Syntax Analysis
The Parser is a Recursive Descent parser that consumes the `Token` stream and builds an Abstract Syntax Tree (AST).
It validates that the tokens form valid RokC grammar constructs (as defined in `grammar.md`). The result is a tree of `Node` structures.

### Semantic Analysis & Type Checking
The compiler traverses the AST to resolve types, ensure variables are declared before use, and validate that operations are performed on compatible types. It also checks that decorators are valid and applied to correct node types.

### Comptime Evaluation
Any function or expression marked with `comptime` is executed during this phase. This is how custom decorators (like `@Serializable`) manipulate the AST before final code generation. The AST is rewritten or expanded based on the output of the comptime functions.

### Code Generation
The validated and expanded AST is translated into the target output (e.g., C code, LLVM IR, or directly to machine code depending on future implementation details).
