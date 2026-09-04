# Type System — Core Language Plan

Status: Planned for 0.1.0

## Primitive type direction

Arqis will provide both a target-native default integer and explicit-width integer types:

```text
int  // signed integer whose width follows the compilation target
i1, i2, i8, i32, i64, i256, ...
u1, u2, u8, u32, u64, u256, ...
```

The explicit integer forms are intended to lower directly to the compiler IR/backend where supported. The exact maximum width and unsupported-target behavior remain to be specified.

`int` should be represented internally as the target’s native signed integer type. Its width, range, alignment, overflow behavior, and ABI representation must be documented per target.

Arqis will follow Zig’s integer model for explicit-width integers:

- signed integers use two’s-complement representation
- `iN` and `uN` support arbitrary positive widths up to 65535 bits
- `i1` has the signed two’s-complement range `-1..0`
- `u1` has the range `0..1`
- ordinary arithmetic overflow is illegal behavior
- wrapping and saturating arithmetic use explicit operators rather than silently changing ordinary arithmetic

## Floating-point types

The initial floating-point model is intentionally fixed-width:

```text
float = f32
f32   = IEEE-754 binary32
f64   = IEEE-754 binary64
```

`float` is not target-native. Wider formats such as `f16`, `f80`, and `f128` may be added later when their target support and ABI rules are defined. Arbitrary-width `fN` types are not planned initially.

## Character and text direction

Core `char` is exactly one byte and follows the target platform’s C-compatible `char` signedness/ABI behavior. It is not a Unicode code point and does not promise to represent every human-readable character. Code that needs a guaranteed unsigned byte uses `u8`. Unicode strings, code points, grapheme clusters, encodings, and locale-aware text belong in standard-library types.

Raw `char*` values therefore represent byte-oriented memory; string termination, encoding, ownership, and length remain separate pointer/library concerns.

## Boolean direction

`bool` is a logical type with canonical representation values `0` and `1`:

```qis
bool enabled = true;
bool disabled = false;
```

The only boolean literals are `true` and `false`. Numeric literals `0` and `1` remain integers and are not boolean literals merely because they have the same representation values.

Conditions are strictly boolean. Control-flow conditions and logical operators do not implicitly convert integers or pointers to `bool`:

```qis
if (count != 0) { ... }
if (ptr != null) { ... }
```

Expressions such as `if (count)` and `if (ptr)` are rejected unless an explicit comparison or conversion rule is added later.

## Pointer and `null` direction

Pointers are nullable by definition. Conceptually, `T*` behaves like a pointer to `T` plus a null state, but users do not write an explicit union type for it:

```qis
Player* player = null;
```

Rules:

- `null` is assignable to pointer types.
- `null` is not an integer literal and `0` is not implicitly assignable to a pointer.
- `Player* p = 0` is rejected unless an explicit conversion is added later.
- Pointer comparisons with `null` produce `bool`.
- Dereferencing a null pointer remains invalid behavior under Arqis’s intentionally unsafe memory model.
- `null` is not assignable to integers, structs, enums, or booleans.

The compiler may use an internal null type for literal checking, but `null` is not intended to be a general user-declarable standalone type.

## Array categories

The core syntax distinguishes fixed-size arrays from raw pointers:

```qis
int[4] values = { 1, 2, 3, 4 };
int* pointer = values; // implicit decay to &values[0]
```

`T[N]` is a true fixed-size array with `N` known at compile time and storage inline in the
containing object. `T[]` remains pointer-like syntax equivalent to `T*`; it carries no length or
capacity. The standard library will provide `Slice<T>` for a non-owning pointer-plus-length view
and `Array<T>` for owning dynamic storage.

Fixed-size arrays use C-like indexing and aggregate initialization. In ordinary value contexts,
they implicitly decay to `T*`, including when passed to a raw-pointer parameter. The decay is
suppressed in contexts that require the array object itself, such as `sizeof`, `alignof`, and
address-of. Conversion to `Slice<T>` remains explicit because it introduces a length-bearing view;
array-to-slice construction is explicit. Initializers follow C's partial-initialization rule:
omitted elements are zero-initialized and excess elements are compile-time errors. A local array
declaration with no initializer is still subject to definite-assignment checking, while globals
without initializers follow the global zero-initialization rule.

Fixed arrays compose into multidimensional arrays:

```qis
int[3][4] matrix;
matrix[1][2] = 7;
```

Nested arrays are contiguous in row-major order. Decay applies to the outermost array first, so
`matrix` decays to `int[4]*`, not `int**`; inner dimensions remain part of the pointed-to type.

`sizeof` preserves the array object before decay, following C:

```qis
int[4] values;
int* pointer = values;

sizeof(values);  // 4 * sizeof(int)
sizeof(pointer); // pointer size
```

For a VLA, `sizeof` uses its runtime bound and returns a runtime `usize`.

Variable-length arrays are also part of the core language:

```qis
void process(int count) {
    int[count] values;
}
```

Their bound is an integer expression evaluated at declaration time. VLAs have automatic
block-scope storage, use the same unchecked indexing and pointer decay as fixed arrays, and do
not carry their bound after decay. They are not permitted as globals, `static` objects, or struct
fields, and cannot be used where a compile-time complete type is required. Non-positive bounds
are runtime errors. `sizeof` on a VLA is a runtime `usize`; `alignof` remains compile-time.

VLA syntax is not accepted for function parameters. APIs should use an explicit `T*`/`T[]` and
count, or a `Slice<T>` when the length must travel with the pointer; a decayed parameter does not
retain the caller's VLA bound.

VLA lifetime is intentionally C-like: pointers may escape the declaring scope without compiler
lifetime checks. Once the scope exits, the VLA storage is released and later use of an escaped
pointer is invalid behavior.

Fixed arrays are not implicitly copied. Direct assignment, initialization from another array, and
by-value array parameters or returns are rejected. Use a pointer (for example `int[4]*` when the
complete array bound should be preserved) to share storage. Explicit copying belongs to library
operations such as `std.mem.copy`, `std.mem.move`, or `std.mem.dup`; ordinary array-to-pointer decay
shares the original storage and never copies it.

## `void` and untyped pointers

`void` represents the absence of a value and is initially valid only as a function return type:

```qis
void log_message(char* message) {
    // no return value
}
```

`void*` is not a valid Arqis type. Generic functions and typed containers are the intended replacement for type-erased APIs. Raw byte-oriented APIs should use explicit types such as `u8*` together with a length. If C interoperability later requires untyped ABI pointers, that should be handled by an explicit FFI/opaque boundary rather than by making `void*` a normal language type.

## Pointer arithmetic

Pointers to object types support C-style arithmetic: adding or subtracting an integer moves by
that many elements (`sizeof(*p)` bytes per element), and increment/decrement moves by one
element. Pointer plus pointer is invalid. Subtracting pointers is restricted to the same
array/object (including one-past) and returns a signed pointer-difference type such as `isize`.
One-past pointers may be formed and compared but not dereferenced. Raw pointer arithmetic is
unchecked; function pointers do not participate in object-pointer arithmetic.

## Conversion direction

Conversions between distinct types are always explicit. There are no C-style implicit numeric promotions or pointer/integer conversions.

The intended exceptions are:

- identical types and transparent aliases require no conversion
- a contextually typed numeric literal may initialize a type when its value fits
- `null` may initialize a pointer

Integer-width, signedness, integer/float, pointer/pointer, pointer/integer, and integer/pointer conversions all require explicit casts. The programmer is responsible for truncation, alignment, range, and pointer validity.

The initial cast syntax is C-style:

```qis
i64 wider = (i64)value;
float ratio = (float)count;
```

Because this syntax overlaps with parenthesized expressions, parsing `(Type)expr` requires the compiler to know whether the name inside parentheses denotes a type. Type aliases and custom types therefore need to be registered early enough for parsing, or the parser must defer this distinction to a later phase.

## Literal typing

Unsuffixed numeric literals are compile-time values without a fixed runtime type. Their destination declaration may choose the type when the value is representable:

```qis
int x = 10;
i32 y = 10;
u8 z = 255;
```

This exception applies to literals, not to already-typed expressions:

```qis
int x = 10;
i32 wrong = x;       // rejected
i32 correct = (i32)x;
```

Literal range and sign validity are checked at compile time. The same contextual rule should be defined for floating-point literals and other literal forms.

## Expression compatibility

Binary operators require operands of the same type. Distinct typed operands are not implicitly promoted or converted:

```qis
i32 x = 10;
i64 y = 20;
i64 result = (i64)x + y;
```

The cast is explicit at the operation that depends on it. A literal may adopt the type of the other operand when representable, so `x + 1` is valid for an `i32` `x`; a typed variable of another type is not.

The same compatibility rule applies to comparisons, assignment expressions, and other operators unless a specific operator defines a different rule.

## Comparisons

Comparisons require compatible operand types. Numeric operands must have the same type; there are
no implicit promotions. Enum values compare only within the same enum type. Compatible pointers
may compare with one another or with `null`, while relational pointer comparisons are limited to
the same array/object. Structs have no automatic equality or ordering. Floating-point comparisons
follow IEEE behavior, including `NaN`; logical operators accept only `bool`.

## Local initialization

Local variables may be declared without an initializer, but the compiler performs definite-
assignment checking and rejects reads on paths where a value has not been written first. This is
not a general memory-safety proof: pointer aliasing, allocation, and external calls can escape
the analysis. Globals follow their separate zero-initialization rules.

## Aggregate and pointer semantics

Scalar values, enums, and pointers may be passed and assigned by value. Structs, unions, and fixed
arrays are not implicitly copied or moved: direct assignment and passing or returning an existing
aggregate by value are rejected. Use an explicit pointer to share or mutate an existing object,
or an explicitly authored copy/move/clone operation when a new aggregate value is wanted.
Pointer fields are only copied as part of such an explicit aggregate operation, and are shallow
unless that operation documents a deeper copy.

An explicit move is a raw memory-relocation operation, typically `memmove`-like. It has no hidden
constructor/destructor behavior, ownership transfer, source clearing, or compiler-enforced source
invalidation. Any rule that the source must no longer be used is an API/programmer contract.

Fresh aggregate construction and an explicitly defined clone method are allowed. No default
`clone`, copy, or move implementation is generated. A future explicit derive-style decorator may
generate such a visible method, but it must not change the no-implicit-copy rule.

## Const qualification

Arqis will use C-style qualifier placement for both const pointees and const pointer bindings:

```qis
const int* p;          // pointer to const int
int const* q;          // same meaning
int* const r = ...;    // const pointer to mutable int
const int* const s;    // const pointer to const int
```

The type system must represent these independently:

- pointee constness belongs to the pointed-to type
- pointer binding constness belongs to the variable/declarator

Adding `const` to an otherwise matching type does not require a cast. A type mismatch still requires an explicit conversion:

```qis
int a = 5;
i32 b = 10;

const int c = a; // valid: matching type, added const qualification
const int d = b; // rejected: i32 and int differ
const i32 e = b; // valid: matching type, added const qualification
```

For pointers, adding pointee constness is likewise allowed when the pointed-to types match (`T*` to `const T*`). Removing constness requires an explicit cast and does not provide ownership or lifetime safety. Writes through a cast-away const qualification require separately defined behavior.

## `sizeof`

Arqis provides C-style `sizeof(Type)` and `sizeof(expression)`. The result is a compile-time
unsigned target-sized value such as `usize`; the expression form does not evaluate its operand.
`sizeof(char)` is always `1`, struct sizes include padding/alignment, pointer sizes depend on the
target, and function types cannot be sized. Zero-sized types remain open. The lexer/parser still
need dedicated `sizeof` support.

Struct layout is declaration-ordered and C-compatible by default. Target ABI rules determine
alignment and padding; `sizeof` includes that padding, and fields are not automatically
reordered. Packed or explicitly aligned layouts are future features.

`alignof(Type)` is a public compile-time query returning a `usize` alignment requirement. It is
distinct from size: a struct's alignment is the maximum member alignment, while padding is added
only where required between fields and at the end. `alignof(char)` is `1`; `void` and function
types cannot be queried. The compiler still needs these values internally for layout and pointer
operations.

## Target-sized integers

`usize` and `isize` are built-in types. `usize` is an unsigned pointer-width integer used by
`sizeof`, `alignof`, lengths, capacities, and allocation sizes; `isize` is the signed counterpart
used for pointer differences and other signed size calculations. Conversions to other integer
types are explicit. Standard-library aliases may exist, but these compiler-level types do not
depend on library imports.

## Variable declarations

Variable declarations require explicit types. The initial language has no `auto` or `var`
inference syntax; only compile-time literals may adopt their explicitly declared destination type
when representable. Future inference must not obscure conversion behavior.

`const` expresses immutable binding/access, not compile-time evaluation. A local `const` may be
initialized from a runtime function or allocation. `comptime` (or a context explicitly requiring
a constant expression) is what demands compile-time evaluation. Global initialization still
forbids runtime work.

For aggregate values, constness is shallow but applies to the object itself: a `const` struct
cannot be reassigned or have direct fields modified through that access path. Pointer fields
remain pointers to mutable data unless their pointee type is also const-qualified (`const T*`).

## Open questions

- How are non-byte-aligned values laid out in structs and arrays?
- Are const-qualification conversions implicit or explicit under the no-implicit-conversion rule?
