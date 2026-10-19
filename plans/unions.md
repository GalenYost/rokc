# Unions

Status: Planned for 0.1.0

RokC will support C-style unions with overlapping member storage:

```rok
union ValueHolder {
    int integer;
    float decimal;
}
```

The `union` keyword is used only in the declaration. Once declared, code refers to the type by its
name directly (`ValueHolder*`, `ValueHolder value`), without repeating `union` at each use site.

There is no automatic discriminant or runtime tag. Size and alignment follow the largest member
and the target ABI. Existing union values are not implicitly copied or moved; direct assignment and
by-value passing/return are rejected. Explicit memory operations may copy or relocate the object
representation. Reading an inactive member follows target C/ABI behavior and is not a type-safe
conversion; code needing a reliable variant must track its own tag. The exact aliasing,
inactive-member, `const`, layout, and ABI rules must be documented per target.

Unions remain a core low-level option for compact overlapping storage and can avoid the compile-time
cost of generating equivalent specialized representations through future generics. Generics may
replace many tagged-union APIs, but do not replace the raw union construct.

Fresh union values use C-style initialization:

```rok
ValueHolder first = { .integer = 5 }; // designated member
ValueHolder second = { 5 };           // first member
ValueHolder empty = {};               // zero-initialized first member
```

At most one member may be selected. Unknown designators, incompatible values, and multiple member
initializers are compile-time errors. The selected member is only a programmer convention; there
is no runtime tag and later writes may change which member is considered active. Reading a
different member is permitted without active-member tracking or mandatory diagnostics and follows
target C/ABI behavior. Pattern matching is deferred for raw unions because the compiler cannot
know which member is active. RokC will not add language-level pattern matching for raw unions;
higher-level tagged abstractions remain ordinary library or user code.

Unions may also be declared incompletely before their definitions:

```rok
union Value;
struct Wrapper { Value* value; }
union Value { int integer; float decimal; }
```

Incomplete unions may be used behind pointers, but by-value fields and `sizeof` require a
complete definition.
