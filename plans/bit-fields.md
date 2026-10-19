# Bit-fields

Status: Deferred feature

RokC may eventually support compact integer fields such as:

```rok
struct Flags {
    u8 enabled: 1;
    u8 mode: 3;
}
```

The default RokC representation should be deterministic and documented, including allocation
units, bit ordering, padding, and whether fields may cross units. A future explicit
`repr(c)`/`extern(c)` mode may opt into the target compiler's C ABI for interoperability, but that
layout is target-specific and not portable. Bit-fields cannot be treated as ordinary addressable
objects; explicit integer masks remain preferable for many hardware and serialized-data cases.

Related target-specific features such as `volatile`, atomics, and memory-ordering controls are
deferred and require a separate low-level/concurrency design.
