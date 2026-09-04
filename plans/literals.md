# Numeric Literals

Status: Planned for 0.1.0

Supported forms:

```qis
255          // decimal
0xff         // hexadecimal
0b1111       // binary
0o377        // octal
1_000_000    // digit separators
1.5
1.0e-3       // scientific notation
```

Octal uses an explicit `0o` prefix; leading zero does not imply octal. Separators are allowed
only between digits. Contextual destination typing and compile-time range checks apply to all
integer bases. Explicit literal type suffixes are postponed.
