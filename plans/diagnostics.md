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
and suggestions. Parser recovery should continue where possible to report multiple independent
errors in one run.
