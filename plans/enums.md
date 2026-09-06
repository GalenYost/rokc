# Enums — Core Language Plan

Status: Planned for 0.1.0

## Model

Arqis enums are simple, fieldless, nominal sets of named constants. Members are always qualified
through their enum type; unqualified member names are not introduced into the surrounding scope.

```qis
enum Color {
    Red,
    Green,
    Blue,
};

Color color = Color.Red;
```

The variants are accessed through the enum type namespace (`Color.Red`). Bare global variant names are not part of the intended model.

Enums do not support, in the initial design:

- payload/data-carrying variants
- user-assigned discriminant values
- duplicate variant values
- implicit conversion from enum to integer
- implicit conversion from integer to enum
- user-visible underlying-value behavior

The compiler still needs an internal representation for storage, comparison, switching, and ABI lowering. That representation is an implementation detail unless a later ABI feature explicitly exposes it.

For the initial implementation, every enum uses a fixed 32-bit representation. This gives the compiler and backend a simple, predictable layout while keeping the representation hidden from normal Arqis code.

Variants receive compiler-internal ordinal values in declaration order. The first declared variant is ordinal zero and is therefore the default value produced by zero-initialization.

## Operators and conversions

Enum values may be compared with `==` and `!=` only when both operands have the same enum type.
Ordering operators (`<`, `>`, `<=`, `>=`) are not defined for enums. Enums do not implicitly
convert to integers or to other enum types; any conversion must be explicit.

## Switch behavior

Switching on an enum must either:

- explicitly handle every variant, or
- provide a `default` case.

The compiler should diagnose a switch that is neither exhaustive nor equipped with a default case.

Switches may also operate on integers, `char`, and strings. String cases are compile-time
literals and compare contents (a `strcmp`-equivalent comparison), never pointer identity. Enum
switches may omit `default` only when every variant is handled; `char`, integer, and string
switches require `default` because their domains are not practically exhaustible.

## Open questions

- What happens when invalid enum bits enter through raw memory or foreign code?
- Can enum values be used as compile-time constants in array sizes or other declarations?
