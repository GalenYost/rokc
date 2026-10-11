# Command-Line Interface — Core Tooling Plan

Status: Planned for 0.1.0

The compiler accepts explicit source paths rather than relying on a hardcoded entry file.

```text
rokc check source.rok       # lex, parse, and semantically validate one file
rokc dump-ast source.rok    # print the formatted AST for debugging
rokc help
rokc version
```

`check` returns success only when the front end accepts the input. `dump-ast` is a diagnostic
developer command and does not imply code generation. Build, project, and executable commands are
deferred to 0.2.0.
