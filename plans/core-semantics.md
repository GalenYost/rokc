# Core Expression Semantics

Status: Planned for 0.1.0

Arqis evaluates ordinary expressions deterministically from left to right. This is a deliberate
departure from C's unspecified operand order and is intended to make side effects portable and
debuggable without changing the language's C-like syntax or unsafe memory model.

Rules:

- A call evaluates the callee first, then arguments left to right.
- Binary operators evaluate the left operand before the right operand.
- `&&` and `||` short-circuit after evaluating the left operand when its value determines the
  result.
- Assignment evaluates the destination/address, then the right-hand side, then stores the
  result.
- Prefix and postfix operations are sequenced according to their position in this order.
- `defer` evaluates and captures its callee and arguments at registration time.

The optimizer must preserve observable sequencing. These rules do not provide bounds checking,
pointer lifetime safety, race detection, or other memory-safety guarantees.

The exact sequencing of future operators (conditional expressions, comma expressions, atomics,
and volatile operations) must be added when those features are specified.

## Precedence and associativity

Operators shared with C use the following precedence, from highest to lowest:

1. postfix calls, indexing, and member access
2. parenthesized grouping
3. unary operators (`!`, `~`, unary `+`/`-`, prefix `++`/`--`)
4. `*`, `/`, `%`
5. `+`, `-`
6. `<<`, `>>`
7. `<`, `<=`, `>`, `>=`
8. `==`, `!=`
9. bitwise `&`
10. bitwise `^`
11. bitwise `|`
12. logical `&&`
13. logical `||`
14. conditional `?:`, if enabled
15. assignment and compound assignment, right-associative
16. comma operator, if enabled

Exponentiation is not a 0.1 core operator. A future standard-library operation or language
revision may define it after integer and floating-point behavior are specified. Commas used only
to separate arguments or initializer items are separators, not necessarily comma operators.

The conditional operator `?:` is included between logical OR and assignment. It is
right-associative. Its condition must be `bool`; the condition is evaluated first and only the
selected branch executes. Both result branches must have compatible types, except that a
contextually typed literal may adopt the other branch's type when representable.

The comma operator is included at the lowest precedence and is left-associative. It evaluates
each operand from left to right and produces the value and type of the rightmost operand:

```qis
int result = (a += 1, a += 2);
```

Commas used to separate arguments or initializer items remain separators. Parentheses are needed
to use a comma expression inside those comma-delimited constructs.

## Unary and pointer operators

The core language retains C-style unary and pointer operators:

```qis
int value = 10;
int* pointer = &value;
int copy = *pointer;
```

- `&` takes the address of an addressable lvalue.
- `*` dereferences a pointer and yields an lvalue; null/invalid dereferences remain invalid
  behavior.
- Unary `+` and `-` require numeric operands.
- `!` requires `bool` and returns `bool`.
- `~` requires an integer operand.
- Prefix and postfix `++`/`--` require a mutable assignable lvalue; prefix yields the updated
  value and postfix yields the previous value.

The `*` token is context-sensitive between pointer declarators, unary dereference, and binary
multiplication. Pointer arithmetic, including pointer `++`/`--`, remains to be specified with
the raw-array model.

## Pointer arithmetic

Pointers to object types support C-style arithmetic. `p + n` and `p - n` advance by `n`
elements, scaled by `sizeof(*p)`, while `p++` and `p--` advance by one element. Pointer plus
pointer is invalid. Pointer subtraction is valid only for pointers into the same array/object
(including its one-past position) and returns a signed difference type such as `isize`.

A one-past pointer may be formed and compared but cannot be dereferenced. Out-of-range pointer
arithmetic and invalid dereferences remain invalid behavior; raw pointers receive no bounds
checks. Function pointers do not support object-pointer arithmetic.

## `switch` control flow

`switch` follows C's fallthrough model. A matching case begins execution and continues into later
case bodies until `break`, `return`, or scope exit. Cases do not implicitly break. `break` exits
the nearest switch or loop. If no case matches, `default` executes when present; otherwise no
case body executes.

Duplicate case values and duplicate `default` labels are errors. Enum switches must cover all
variants or include `default`. A future diagnostic may warn about unintended fallthrough without
changing this behavior.

Switch expressions may be integers, enums, `char`, or strings. String case labels are compile-time
literals and compare their null-terminated contents rather than pointer addresses, conceptually
through `strcmp`-equivalent logic. A string switch value must reference valid string storage;
null/invalid pointers remain invalid to inspect.

Exhaustiveness is domain-sensitive: an enum switch may omit `default` only when all variants are
handled, while switches over strings, `char`, and integer domains require `default`.

## Function returns

Every reachable path through a non-`void` function must return a value. Reaching the closing
brace is a compile-time error for such a function. `void` functions may return with `return;` or
fall through, but may not return a value. Return expressions evaluate before active `defer`
statements run, and those defers execute during the return. Infinite loops and a future
`noreturn` annotation must be handled by control-flow analysis.

## Fatal errors

Language-level fatal errors use defer-aware unwinding: active scopes are exited and their
registered defers run in LIFO order until one deferred call fails fatally, at which point
unwinding stops and the process terminates when no recovery mechanism is available. External
termination and hardware faults may bypass defers. Future recovery semantics remain open.

## `for` and `while` loops

`for` and `while` retain C-compatible behavior. A `for` initializer declaration is scoped over
the condition, increment, and body. The condition runs before each iteration; the `for`
increment runs after the body and before the next condition. `while` also checks its condition
before each iteration. `continue` targets the nearest loop (the increment in a `for`, the next
condition check in a `while`), while `break` exits the nearest loop or switch.

Arqis also supports C-compatible `do`/`while`: the body executes once before its condition is
checked, `continue` proceeds to the condition, `break` exits the loop, and the trailing
semicolon is required. The lexer/parser implementation of the `do` keyword remains pending.

## `goto` and labels

The core language omits `goto` and user-defined labels. Structured control flow is preferred,
and unrestricted jumps would complicate scope initialization and `defer` cleanup. Any future
low-level jump escape hatch would require separate semantics.

## Raw indexing

For raw arrays and pointers, `base[index]` is syntactic sugar for `*(base + index)`. The index
must be an integer, pointer arithmetic performs element scaling, and the result is an lvalue.
Base and index expressions evaluate left-to-right. Raw indexing performs no bounds checks;
negative, out-of-range, null, one-past, and otherwise invalid accesses remain invalid behavior.
The reversed C spelling `index[base]` is not required. Library views and containers define their
own checked indexing behavior.

## Aggregate initialization

Struct aggregate initializers use C-style positional or designated syntax, but all fields are
required unless they declare defaults. Missing fields, duplicate designators, invalid positional
order, and incompatible values are compile-time errors. Nested aggregates follow the same rule;
whole uninitialized globals retain their separate zero-initialization behavior.

## Assignment

Assignment follows C-style rules. The left operand must be a mutable lvalue; literals,
temporaries, `const` bindings, and read-only pointees are not assignable. Assignment is
right-associative and yields the stored value, so `a = b = 5` is valid. Compound assignments
evaluate the destination once before applying the operation. Struct, union, and fixed-array
assignment are rejected unless an explicitly defined copy/move operation is invoked; all other
assignment conversions obey Arqis's strict conversion rules.

## Comparisons

`==` and `!=` require compatible operand types. Numeric comparisons require the same numeric
type, and enum comparisons require the same enum type. Compatible pointers may compare with
other pointers or `null`; relational pointer comparisons are valid only within one
array/object. Structs have no automatic equality or ordering. Floating-point comparisons follow
IEEE behavior, including `NaN`. Logical operators accept only `bool`.

## Local initialization

Locals may omit initializers and retain C-like indeterminate contents. The core compiler does not
perform definite-assignment analysis or require an `uninit` marker. Reading an indeterminate local
is invalid program behavior and remains the programmer's responsibility. Global variables without
initializers are zero-initialized.

Explicit panic is provided by the runtime/standard library rather than core syntax. A future
`noreturn` declaration attribute will allow return-path analysis to recognize non-returning
functions.
