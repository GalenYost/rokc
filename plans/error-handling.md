# Error Handling

Status: Rejected for the initial language; may be reconsidered later

The initial language does not provide exceptions, a generic `Result<T, E>` type, or Rust-style
postfix `?` propagation. Recoverable failures use explicit C-style conventions: nullable pointers
represent absence and are checked against `null`; non-pointer functions use documented status or
error values, enums, or output parameters. Fatal paths remain defer-aware and unrecoverable.

Null checks cannot represent every failure for value-returning functions, so each API must
document its status convention. The `?` token remains reserved for the ternary operator. A
`Result` abstraction may be reconsidered in a future design.

Status-returning APIs use `0` for success and nonzero documented values or enums for failure.
Output values use explicit pointer parameters. Nullable pointers represent absence only when
that is the operation's intended result. The compiler does not require callers to inspect a
returned status.

Explicit panic is a runtime/standard-library function, not a core keyword. A future `noreturn`
attribute should let control-flow analysis recognize that it cannot return.
