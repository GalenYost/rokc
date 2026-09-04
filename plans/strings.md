# Strings and Character Literals

Status: Planned for 0.1.0

String literals are immutable, null-terminated byte sequences:

```qis
const char* name = "Player1";
```

The literal contains a trailing `\0` byte and has static storage duration. In pointer contexts it
decays to `const char*`; assigning it to mutable `char*` requires an explicit cast, and modifying
the storage is invalid behavior. The source/literal encoding and non-ASCII handling remain open;
Unicode and richer text operations belong in standard-library types.

## Character literals

Character literals have type `char` and represent exactly one byte. Standard one-byte escapes
such as `\n`, `\t`, `\\`, `\'`, and `\0` are supported; multi-byte Unicode characters are not
core literals, and character literals do not implicitly become `int`. Wider character and text
types belong in the standard library.
