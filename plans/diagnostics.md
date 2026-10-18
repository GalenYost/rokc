# Compiler Diagnostics

Status: Planned for 0.1.0

The current logging and message-formatting infrastructure remains the output backend. Compiler
diagnostics add a severity and source-location policy:

- errors reject the program and prevent executable output;
- warnings are non-fatal by default;
- an optional warnings-as-errors mode promotes warnings to errors;
- notes attach context to another diagnostic;
- internal compiler errors identify compiler defects rather than source errors.

Each diagnostic should include a source path and primary span, with optional related spans, notes,
and suggestions. The parser uses hybrid panic-mode recovery: ordinary syntax and rule violations
should synchronize at safe boundaries such as semicolons, closing braces, and declaration starts so
multiple independent errors can be reported in one run. Critical failures, unrecoverable parser
state, resource exhaustion, and internal compiler errors stop compilation immediately.

Future language keywords and syntax are reserved before their implementations land. In a release
where a reserved feature is not yet available, the compiler emits a dedicated unsupported-feature
diagnostic instead of interpreting the token as an identifier or unrelated expression.
