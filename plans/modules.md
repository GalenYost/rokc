# Modules, Imports, and Visibility — Core Language Plan

Status: Planned for 0.1.0

## Import model

Arqis imports follow the Java/Rust idea of either binding a specific leaf or binding a module namespace. Arqis uses `.` as the path separator:

```qis
import std.collections.HashMap;

HashMap.new();
```

```qis
import std.collections;

collections.HashMap.new();
```

```qis
import std;

std.collections.HashMap.new();
```

The final path component is bound in the local namespace. The resolver determines whether it refers to an exported declaration or an exported module. Wildcard imports are not part of the initial design.

## Initial module assumptions

- One source file represents one module.
- Module paths are resolved from a configured project/package source root.
- Relative imports such as `./utils` are not supported initially.
- A source file such as `src/core/parser.qis` is imported as `core.parser` from that source root.
- A declaration inside it may be imported as `core.parser.Parser`.
- Imported declarations must be exported by their defining module.
- Imports are local to the importing module and are never re-exported automatically.
- There is no explicit re-export syntax in the initial design.
- Name collisions are diagnosed rather than silently overwritten.
- Cyclic imports are rejected initially.

## Global initialization

Modules do not support runtime global-variable initialization. A global initializer must be resolved at compile time, or the declaration must use whatever zero/uninitialized policy the core language eventually specifies.

Examples of the intended distinction:

```qis
comptime const int EVAL_INT = 5 + 8 * 2;
const int INT_VAR = 21;
```

Global initializers may not call runtime functions, allocate runtime memory, or depend on module initialization order.

The intended forms are:

```qis
comptime int count = 5 + 5;
int other_count = 0;
```

If a global is declared without an initializer, it receives default zero-initialization. The exact zero value for pointers, booleans, floating-point values, structs, arrays, and enums must be specified by the type system.

For enums, the first declared variant has internal ordinal zero and is the default zero-initialized value.
- Import aliases may be added later if collisions require them.

## Visibility

The current visibility proposal is:

- declarations are public/exported by default
- `static` at module scope prevents a method, type, struct, or global constant/variable from being exported
- `static` at module scope means module-private visibility, not persistent storage
- `static` on a struct field means shared storage across instances
- `static` on a struct function means a type-namespace function with no instance receiver; it is called as `Player.method(...)`, not `p.method(...)`
- `static` inside a struct does not mean private
- struct fields and methods remain accessible according to the language’s normal extensibility rules
- ABI/FFI visibility is a separate concern and may use a future `Export` decorator or other explicit mechanism

Visibility applies to a declaration’s public interface, not to the implementation body:

- A private declaration may use public types and declarations.
- A public function may use private helpers and constants in its implementation body.
- A public function may not expose a private type in its parameters or return type.
- A public struct may not expose a private type in a field, method signature, or alias.
- A private constant may be used to initialize public data when the resulting public type and value are valid.
- Public struct fields and methods do not have a private member mode in the initial design.

This intentionally gives `static` different meanings by declaration scope, similar to C-family languages. The distinction must be documented clearly.

## Open questions

- Is an imported module name available only under its final component or also under its full path?
- Are private types allowed in exported function signatures?
- Can a module-private type appear in a public function signature?
- How are module initialization side effects ordered?
- Is `comptime const` an explicit request, or are all constant global initializers automatically compile-time evaluated?
- What syntax should import aliases use?
- How are source roots and project entry points configured?
- Are module and file names case-sensitive?
- What happens when a file and directory produce the same dotted module path?

## Handling shared dependencies without cycles

Code that would require mutual imports should use a one-way dependency structure instead:

- Move shared structs, constants, and independent helpers into a common module.
- Place mutually recursive declarations in one module when they are one conceptual unit.
- Use opaque handles or pointer-facing APIs when only an interface needs to be shared.
- Keep global state in a dedicated module with a clear owner.

For example, if `a` and `b` both need shared types, prefer:

```text
common.types  <-  a
common.types  <-  b
```

rather than making `a` and `b` import one another.
