# Compiler Backend

Status: Planned for 0.2.0

RokC will use [QBE](https://c9x.me/compile/) as its initial compiler backend. The 0.1.0
release is a front end and does not include code generation.

The planned compilation path is:

1. Parse and semantically validate RokC source.
2. Lower the validated program into a stable RokC internal representation.
3. Translate that representation into QBE's intermediate language and use QBE for native code
   generation.

Keeping a RokC-owned IR between semantic analysis and QBE should isolate language semantics from
backend-specific details and leave room to change or add backends later. The exact IR invariants,
supported targets, ABI behavior, linking model, and QBE integration method remain to be specified
for 0.2.0.
