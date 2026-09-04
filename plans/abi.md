# C ABI and Embeddability

Status: Deferred until after the core release and stable IR/backend

Arqis will not promise C ABI compatibility in 0.1.0. Source-level C-like syntax is not a binary
ABI guarantee. A later release must define:

- exported symbol naming and visibility;
- calling conventions;
- primitive type mappings;
- struct and union layout;
- pointer and string representation;
- allocator ownership across the boundary;
- status/error conventions;
- callback ABI;
- runtime initialization and shutdown.

The contract should be validated with generated C declarations and executable cross-language
tests before being called stable.
