# `defer` — Core Language Plan

Status: Planned for 0.1.0

## Syntax

The initial form is restricted to function calls:

```rok
defer free(ptr);
```

Suggested grammar:

```ebnf
<defer_statement> ::= "defer" <function_call> ";"
```

Arbitrary deferred statements are intentionally excluded from the first design because their capture and mutation semantics are harder to define.

## Semantics

- A deferred call is registered when execution reaches the `defer` statement.
- The callee and all arguments are evaluated immediately and their values are captured.
- The call itself executes when the current lexical scope exits.
- Deferred calls execute in last-in, first-out order.
- Every registered call executes exactly once when its scope exits normally.
- `return`, `break`, and `continue` execute deferred calls for every lexical scope they leave.
- Nested lexical scopes have independent defer stacks.
- Capturing a pointer captures the pointer value, not a copy of the pointed-to object.
- Deferred calls may return any type; their return values are intentionally discarded at scope
  exit.
- If a deferred call triggers a language-level fatal error, unwinding stops immediately and later
  deferred calls are skipped.
- Language-level fatal errors unwind active scopes and execute their deferred calls in LIFO order
  before termination when no recovery mechanism exists.
- Abnormal external termination and hardware faults do not guarantee deferred calls will execute.

Example:

```rok
void example(int* p) {
    defer free(p); // captures the current value of p
    p = null;
} // free is called with the captured original pointer
```

## Open questions

- What happens if a deferred function returns an error?
- What happens if a deferred function itself terminates execution?
- Are deferred calls allowed in global initialization?
- Are deferred calls allowed inside comptime code later?
- What happens if registering a deferred call fails?
