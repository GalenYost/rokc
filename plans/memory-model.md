# Runtime Memory Model

Status: Planned for 0.1.0

## Allocation failure

Ordinary allocation is fail-fast. If allocation cannot be satisfied, the runtime terminates via
its fatal-error path instead of returning a null pointer. This is a policy choice, not a claim
that allocation failure is rare: address-space limits, commit limits, fragmentation, quotas, and
OS policy can all cause failure despite available physical RAM.

A future explicitly fallible API (for example `try_alloc`) may return a nullable pointer or a
typed result for code that needs recovery. The fatal diagnostic, allocator interface,
`realloc` behavior, and zero-size allocation rules remain open. `free(null)` may be a no-op as in
C.

## Deallocation and reallocation

`free(null)` is a no-op. Freeing a non-allocator pointer, double-free, and use-after-free are
invalid behavior. Memory leaks are permitted and are not diagnosed by the core language.

`realloc` may move storage and invalidate pointers into the old allocation. Successful
reallocation preserves the existing byte prefix up to the new size. Ordinary reallocation
failure is fatal under the default fail-fast policy. Allocator families, alignment, and zero-size
reallocation still require specification.
