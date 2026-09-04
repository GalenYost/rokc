# Structs and Methods — Core Language Plan

Status: Planned for 0.1.0

## Struct contents

A struct may contain fields and functions. Functions do not imply hidden object state, inheritance, virtual dispatch, or a vtable.

## Aggregate copying

Structs are not implicitly copied or moved. Passing an existing struct by value, assigning it, or
returning it as an existing value is rejected. Use an explicit pointer parameter/receiver to share
or mutate an object. A user-defined method may explicitly copy, move, or clone a struct and return
the resulting value; pointer fields are shallow-copied only when that operation specifies it.
Constructing a fresh struct from literals or a constructor is not an implicit copy.

An explicit move means memory relocation, typically with `memmove`-like behavior. It is not a
compiler-generated move constructor, copy-plus-clear operation, or ownership transfer. The
compiler does not invalidate or track the source after a move; source validity is the programmer's
responsibility.

Fresh construction and explicitly authored clone methods are valid:

```qis
Point point = Point.new(10, 20);
Point copy = point.clone();
```

The compiler does not synthesize `clone`, copy, or move methods. A future explicit derive-style
decorator (for example `Clone`) may generate a visible method, but opting in must never introduce
implicit copying.

## Member access

Arqis uses only `.` for struct member access. The C-style `->` operator is intentionally omitted:

```qis
Point point;
Point* pointer;

point.x;
pointer.x; // equivalent to (*pointer).x
```

When the left operand is a pointer to a struct, the compiler inserts the dereference needed for
member access. Null or invalid pointer access remains invalid behavior.

## Layout

Struct fields retain declaration order and use target/ABI-compatible alignment and padding. The
compiler does not reorder fields automatically, and `sizeof` includes inserted padding. Packed
or explicitly aligned layouts require a future attribute or layout mechanism.

## Incomplete declarations

Recursive structs use explicit C-style incomplete declarations when needed:

```qis
struct Node;
struct Node { int value; Node* next; };
```

An incomplete type may be used behind a pointer but not as a by-value field or in `sizeof` until
its definition is complete. Declaration-first organization is a style convention, not a global
ordering requirement.

## Aggregate initialization

Structs use C-style positional and designated initializers, but partial initialization is not
allowed by default:

```qis
Point point = { .x = 10, .y = 20 };
```

Every field must be supplied unless it declares a default value. Missing required fields,
duplicate fields, invalid positional order, and incompatible values are compile-time errors.
Nested aggregates follow the same rule. A whole global object with no initializer still follows
the language's separate zero-initialization rule.

Defaults are intended to be compile-time-resolvable initially. Future `comptime` evaluation may
permit more expressive calculations, but the syntax must distinguish a runtime field with a
compile-time-computed default from a compile-time-only field.

```qis
struct Player {
    int health;

    char* to_json(Player* self) {
        // explicit mutable pointer receiver
    }

    char* name(Player const* self) {
        // explicit read-only pointer receiver
    }

    Player new(int health) {
        // non-static, object-qualified function with no receiver
    }

    static Player from_default() {
        // explicit type-namespace function; no instance receiver
    }
};
```

## Receiver rules

- The receiver is an ordinary explicit parameter chosen by the programmer.
- It may be a mutable pointer or read-only pointer; value receivers are not allowed because they
  would implicitly copy the struct.
- It may be absent for a non-static object-qualified function.
- A non-static function with no receiver is still object-qualified and receives no injected object.
- A receiver is not implicitly inserted into the function declaration.
- There is no mandatory `self` or `this` keyword.
- Methods do not automatically imply inheritance, dynamic dispatch, or hidden fields.
- A function is treated as an instance method when its first parameter is a compatible pointer to
  the containing struct type (with supported const qualification).
- A function whose first parameter is not compatible with the containing struct remains a
  non-static object-qualified function with no injected receiver.
- `static` on a struct function explicitly makes it a type-namespace function with no instance receiver.
- A static struct function is called through the type namespace, not through an instance.

Member-call syntax is provided as syntax sugar when the declared function has a compatible pointer
receiver:

```qis
p.to_json();
```

would correspond to something like:

```qis
Player.to_json(&p);
```

The exact receiver matching rules, const qualification, and value-receiver behavior remain to be specified. C++-style reference receivers are not part of the language.

## Construction

There is no special constructor or `new` syntax. Type-namespace functions are ordinary user-
defined functions; `new`, `from_default`, or any other name is purely a convention and does not
implicitly allocate or initialize hidden state.

## Call forms

- A non-static function whose first parameter matches the struct receives an implicit object
  receiver for `object.method(args)`.
- A non-static function with no matching receiver is still callable only through
  `object.method(args)`, but receives no injected object argument.
- A `static` function has no receiver and is callable only as `Type.method(args)`, never through
  an object.

The no-receiver, object-qualified form selects a member but provides no object-state access.

Fields and methods share one struct namespace. A field and method may not have the same name;
single-dot member access must resolve unambiguously.

## Open questions

- What syntax and evaluation rules define default field values?
- What explicit API or decorator convention should generate clone/copy methods?
- How should a future C-ABI layout attribute interact with struct methods and fixed-array fields?
