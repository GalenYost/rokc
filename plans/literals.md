# Numeric Literals

Status: Planned for 0.1.0

Supported forms:

```rkc
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
integer bases. Contextual typing also applies when a literal is passed directly to a function:

```rkc
void set_limit(u8 limit);

set_limit(10);   // valid: 10 fits u8
set_limit(300);  // compile-time error: 300 does not fit u8
```

Explicit literal type suffixes are postponed.

Unsuffixed floating-point literals are contextually typed as `f32`/`float` or `f64` when a
destination type is known. In a context-free expression they default to `f64`.
