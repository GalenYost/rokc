# Strings and Character Literals

Status: Planned for 0.1.0

String literals are immutable, null-terminated byte sequences:

```rok
const char* name = "Player1";
```

The literal contains a trailing `\0` byte, has static storage duration, and is inherently read-only.
Its type is `const char*`; assigning it to mutable `char*` requires an explicit cast, and modifying
the storage is invalid behavior. The source/literal encoding and non-ASCII handling remain open;
Unicode and richer text operations belong in standard-library types.

## Character literals

Character literals represent exactly one byte and use ordinary target `char` semantics by default.
They may be contextually represented as `u8` when the destination explicitly requests an unsigned
byte:

```rok
char letter = 'A';
u8 byte = '\xff';
```

Standard one-byte escapes such as `\n`, `\t`, `\\`, `\'`, and `\0` are supported. Multi-byte
Unicode characters are not core literals, and character literals do not implicitly become `int`.
Wider character and text types belong in the standard library.
