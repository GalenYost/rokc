# Function Pointers and Callback Types

Status: Planned for 0.1.0

RokC uses C-style function-pointer declarators in the core language:

```rok
int apply(int (*operation)(int, int), int left, int right) {
    return operation(left, right);
}
```

Function pointers have an exact signature. The initial implementation supports pointers to
named functions only; closures and captured environments are postponed. A function pointer
stores a code address and follows the ordinary nullable-pointer rules, so calling a null or
invalid pointer is invalid behavior.

The address-of form (`&function_name`) is the explicit callback-passing form in examples. The
rules for bare function-name decay, calling conventions, ABI compatibility, and overloads are
resolved: RokC allows C-style decay of a named function to a compatible function pointer, while
the explicit `&function` form remains valid and equivalent. This implicit conversion is limited
to compatible function-pointer contexts.

A future `@Callback` convenience facility may generate a named callback type, but it must be a
compiler-supported type-level feature. It cannot be implemented as an ordinary standard-library
function because libraries cannot add type grammar or change type checking. It should wait for
the decorator/comptime and type-reflection designs. RokC has no C++-style reference types, so
callback parameters use explicit pointers when indirect access is required.

The core language omits C-style variadic functions and `...` parameters. Variable-argument APIs
should use typed slices/arrays or future generics; variadic foreign-function support is deferred
to a later explicit FFI design.
