# RokC Grammar

This document is the complete EBNF grammar for the RokC language as designed for the core
release (0.1.0). It incorporates the design decisions recorded in the current language plans.
Rules marked `[reserved]` are future-feature syntax that the compiler must
diagnose as unsupported in this release rather than treating the token as an identifier.

Notation:

- `::=` defines a rule.
- `{ X }` means zero or more repetitions of `X`.
- A postfix `*` on a nonterminal has the same zero-or-more meaning (for example,
  `<struct_member>*`).
- `[ X ]` means zero or one occurrence of `X`.
- `( X | Y )` means a group of alternatives.
- `"..."` is a literal terminal; `<name>` is a nonterminal.
- The `// !! precedence` annotations below describe binding strength, not evaluation order.

---

## 1. Lexical rules

### Whitespace and layout

Whitespace is any run of space, horizontal tab, carriage return, or newline. Whitespace and
comments separate tokens but are not themselves tokens.

#### Comments

```ebnf
<line_comment>    ::= "//" <no_newline>*
<block_comment>   ::= "/*" <any_character>* "*/"
<comment>         ::= <line_comment> | <block_comment>
```

Block comments do not nest. An unterminated block comment is a lexical error. A `//` inside a
block comment does not start a line comment.

#### Identifiers

```ebnf
<identifier>     ::= <id_start> { <id_continue> }
<id_start>       ::= [a-zA-Z_]                       // ASCII letters and underscore
<id_continue>    ::= [a-zA-Z0-9_]
```

Identifiers are ASCII-only in the core language. Identifiers matching a reserved keyword are
lexed as that keyword, never as an identifier. Keyword/identifier precedence is therefore decided
at the lexer: keywords win.

### Reserved keywords

```ebnf
<keyword> ::= "as" | "bool" | "break" | "case" | "char" | "comptime" | "const"
            | "continue" | "default" | "defer" | "do" | "else" | "enum"
            | "float" | "for" | "if" | "import" | "int" | "null"
            | "return" | "static" | "struct" | "switch" | "true" | "false"
            | "type" | "union" | "void" | "while"
            | "sizeof" | "alignof"
            | "i1" | "i2" | "i8" | "i16" | "i32" | "i64" | "i128" | "i256"
            | "u1" | "u2" | "u8" | "u16" | "u32" | "u64" | "u128" | "u256"
            | "f16" | "f32" | "f64" | "f80" | "f128" | "usize" | "isize"
```

Explicit-width and target-sized types are part of the 0.1 type model and are reserved keywords.

### Numeric literals

```ebnf
<digit>            ::= [0-9]
<hex_digit>        ::= [0-9a-fA-F]
<binary_digit>     ::= [01]
<octal_digit>      ::= [0-7]
<dec_sep>          ::= "_"          // digit separator; only between digits
<decimal_int>      ::= <digit> { <dec_sep>? <digit> }
<hex_int>          ::= "0x" <hex_digit> { <dec_sep>? <hex_digit> }
<binary_int>       ::= "0b" <binary_digit> { <dec_sep>? <binary_digit> }
<octal_int>        ::= "0o" <octal_digit> { <dec_sep>? <octal_digit> }
<integer_literal>  ::= <decimal_int> | <hex_int> | <binary_int> | <octal_int>
<exponent>         ::= ( "e" | "E" ) [ "+" | "-" ] <decimal_int>
<float_literal>    ::= <decimal_int> "." <decimal_int> [ <exponent> ]
                     | <decimal_int> <exponent>
<number_literal>   ::= <integer_literal> | <float_literal>
```

A leading zero does not imply octal; `0o` is the explicit octal prefix. Digit separators may
appear only between digits, never at the beginning, end, or in a run. Explicit literal type
suffixes are `[reserved]`.

### String and character literals

```ebnf
<escape_seq>       ::= "\\" ( "n" | "t" | "r" | "\\" | "\"" | "\'" | "0" )
<string_literal>   ::= "\"" { <string_char> | <escape_seq> } "\""
<string_char>      ::= <any_character_except_quote_backslash_or_newline>
<char_literal>     ::= "\'" ( <char_char> | <escape_seq> ) "\'"
<char_char>        ::= <any_character_except_quote_or_newline>
```

String literals are immutable, null-terminated byte sequences with static storage duration.
Character literals represent exactly one byte. Multi-line strings, raw strings, and Unicode escape
handling are `[reserved]`. An unescaped newline inside a string literal is a lexer error.

### Operators and punctuation

```ebnf
<operator> ::= "+" | "-" | "*" | "/" | "%" | "++" | "--" | "+=" | "-=" | "*=" | "/=" | "%="
             | "==" | "!=" | "<" | "<=" | ">" | ">="
             | "<<" | ">>" | "<<=" | ">>=" | "&" | "|" | "^" | "~" | "&&" | "||"
             | "&=" | "|=" | "^="
             | "!" | "?" | ":" | "="
             | "**"          // [reserved]: exponentiation, not a 0.1 core operator
<punctuation> ::= "(" | ")" | "[" | "]" | "{" | "}" | "." | "," | ";" | "@" | "#" | "$"
```

The `**` token is lexed but is not a core expression operator in 0.1; using it produces an
unsupported-feature diagnostic. Exponentiation semantics (precedence above unary, right
associativity) may be reconsidered for a later release.

---

## 2. Types

```ebnf
<base_type>       ::= "int" | "char" | "float" | "bool" | "void"
                    | "i1" | "i2" | "i8" | "i16" | "i32" | "i64" | "i128" | "i256"
                    | "u1" | "u2" | "u8" | "u16" | "u32" | "u64" | "u128" | "u256"
                    | "f32" | "f64" | "usize" | "isize"
                    | <identifier>          // custom, enum, union, or alias type name
<type>            ::= [ "const" ] <base_type>
                      { [ "const" ] <type_suffix> } [ "const" ]
<type_suffix>     ::= "*"                  // pointer to the preceding type
                    | "[" "]"              // raw pointer-like array, equivalent to "*" for preceding type
                    | "[" <bound_expr> "]" // array with a bound: fixed-size if <bound_expr> is a
                                           // constant integer expression, otherwise a variable-length
                                           // array (automatic local objects only)
```

Suffixes read left to right but bind tightest to the nearest suffix, so `int[4]*` is a pointer to
(`int[4]`) and `int*[]` is (`int*`) followed by a raw array. `const` may appear before the base
type, between suffixes, or at the end, matching C-style pointee and binding qualification forms.

Array bounds: a constant integer expression denotes a fixed-size array; a non-constant integer
expression denotes a VLA (automatic local objects only). `[]` alone is raw pointer-like syntax.
`void*` is not a valid type. Function-pointer and array-vs-pointer decay are described under
Expressions and Conversions.

---

## 3. Declarations

```ebnf
<declaration>        ::= <import_decl>
                       | <struct_decl>
                       | <union_decl>
                       | <enum_decl>
                       | <type_decl>
                       | <var_decl>
                       | <function_pointer_decl>
                       | <func_decl>
                       | <func_prototype>
                       | <incomplete_decl>

<import_decl>        ::= "import" <dotted_name> [ "as" <identifier> ] ";"
<dotted_name>        ::= <identifier> { "." <identifier> }

<incomplete_decl>    ::= "struct" <identifier> ";"
                       | "union"  <identifier> ";"

<struct_decl>        ::= [ "static" ] "struct" <identifier> "{" <struct_member>* "}" ";"
<struct_member>      ::= <field_decl> | <member_func_decl> | <member_func_prototype>

<union_decl>         ::= [ "static" ] "union" <identifier> "{" { <field_decl> }* "}" ";"
                       // unions may also declare an incomplete form via <incomplete_decl>

<enum_decl>          ::= [ "static" ] "enum" <identifier> "{" <enum_variant> ( "," <enum_variant> )* [ "," ] "}" ";"
<enum_variant>       ::= <identifier>

<type_decl>          ::= [ "static" ] "type" <identifier> "=" <type> ";"

<field_decl>         ::= [ "static" ] [ "const" ] <type> <identifier> [ "=" <constant_expression> ] ";"

<member_func_decl>   ::= [ "static" ] <type> <identifier>
                         "(" <parameter_list> ")" <block>
<member_func_prototype> ::= [ "static" ] <type> <identifier>
                            "(" <parameter_list> ")" ";"

<var_decl>           ::= [ "static" ] [ "const" ] <declarator> ";"
<declarator>         ::= <type> <identifier> [ "=" <expression> ]

<func_decl>          ::= [ "static" ] <type> <identifier> "(" <parameter_list> ")" <block>
<func_prototype>     ::= [ "static" ] <type> <identifier> "(" <parameter_list> ")" ";"

<parameter_list>     ::= [ <parameter> ( "," <parameter> )* ]
<parameter>          ::= <type> [ <identifier> ]
                       | <function_pointer_parameter>
<function_pointer_parameter> ::= <type> "(" "*" [ <identifier> ] ")"
                                  "(" <parameter_list> ")"

<function_pointer_decl> ::= <type> "(" "*" <identifier> ")"
                            "(" <parameter_list> ")" [ "=" <expression> ] ";"
```

Notes:

- `static` at module scope means module-private (not exported); `static` on a struct field means
  shared storage; `static` on a struct function means type-namespace with no receiver. Scope
  determines the meaning.
- `const` on a value binding means immutable binding/access, not compile-time value. `comptime`
  demands compile-time evaluation.
- A struct member equals one struct; functions may not be declared at module scope with a `static`
  storage meaning — module `static` is visibility only.
- Global variable initializers must be compile-time-resolvable or zero-init; runtime initialization
  is disallowed.
- A function prototype has the signature but no body. Missing definitions and incompatible
  redeclarations are diagnosed.
- The `parameter` rule permits an unnamed parameter in a prototype and a named parameter in a
  definition.

---

## 4. Statements

```ebnf
<statement>          ::= <block>
                       | <if_statement>
                       | <while_statement>
                       | <do_statement>
                       | <for_statement>
                       | <switch_statement>
                       | <return_statement>
                       | <break_statement>
                       | <continue_statement>
                       | <defer_statement>
                       | <expression_statement>
                       | <declaration>
                       | ";"                    // empty statement

<block>              ::= "{" { <statement> }* "}"

<if_statement>       ::= "if" "(" <expression> ")" <statement> [ "else" <statement> ]

<while_statement>    ::= "while" "(" <expression> ")" <statement>

<do_statement>       ::= "do" <statement> "while" "(" <expression> ")" ";"

<for_statement>      ::= "for" "(" [ <for_init> ] ";" [ <expression> ] ";" [ <expression> ] ")" <statement>
<for_init>           ::= <for_var_decl>        // declaration form, without its trailing semicolon
                       | <expression>          // expression form
<for_var_decl>       ::= [ "static" ] [ "const" ] <declarator>

<switch_statement>   ::= "switch" "(" <expression> ")" "{" { <switch_clause> }* "}"
<switch_clause>      ::= "case" <case_value> ":" { <statement> }*
                       | "default" ":" { <statement> }*
<case_value>         ::= <constant_expression>     // integer, char, enum variant, or string literal

<return_statement>   ::= "return" [ <expression> ] ";"
<break_statement>    ::= "break" ";"
<continue_statement> ::= "continue" ";"

<defer_statement>    ::= "defer" <postfix_expression> ";"

<expression_statement> ::= <expression> ";"
```

`defer` requires the postfix expression to resolve to a call; deferring a non-call expression is a
semantic error.

Switch semantics follow C fallthrough: no implicit break; `break` exits the nearest switch or
loop. Enum switches must cover every variant or include `default`; char/integer/string switches
must include `default`. Duplicate case values and duplicate `default` labels are errors.

---

## 5. Expressions

The following precedence table is authoritative. Tighter binds higher. Unless stated otherwise,
binary operators are left-associative.

```ebnf
// 1  postfix: call, index, member                       (tightest, left-assoc)
// 2  primary: literals, identifiers, grouping, cast
// 3  unary:   ! ~ + - ++ -- * &                          (prefix; right-assoc by postfix arrival)
// 4  multiplicative: * / %
// 5  additive: + -
// 6  shift: << >>
// 7  relational: < <= > >=
// 8  equality: == !=
// 9  bitwise and: &
// 10 bitwise xor: ^
// 11 bitwise or: |
// 12 logical and: &&
// 13 logical or: ||
// 14 conditional: ? :                                    (right-assoc)
// 15 assignment: = += -= *= /= %= <<= >>= &= ^= |=       (right-assoc)
// 16 comma: ,                                            (loosest, left-assoc)
```

### Expression rules

```ebnf
<expression>         ::= <assignment_expression> { "," <assignment_expression> }   // left-assoc

<assignment_expression> ::= <conditional_expression>
                          | <unary_expression> <assignment_op> <assignment_expression>
<assignment_op>      ::= "=" | "+=" | "-=" | "*=" | "/=" | "%="
                       | "<<=" | ">>=" | "&=" | "^=" | "|="

<conditional_expression> ::= <logical_or_expression> [ "?" <expression> ":" <conditional_expression> ]

<logical_or_expression>  ::= <logical_and_expression> { "||" <logical_and_expression> }
<logical_and_expression> ::= <bit_or_expression> { "&&" <bit_or_expression> }
<bit_or_expression>      ::= <bit_xor_expression> { "|" <bit_xor_expression> }
<bit_xor_expression>     ::= <bit_and_expression> { "^" <bit_and_expression> }
<bit_and_expression>     ::= <equality_expression> { "&" <equality_expression> }
<equality_expression>    ::= <relational_expression> { ( "==" | "!=" ) <relational_expression> }
<relational_expression>  ::= <shift_expression> { ( "<" | "<=" | ">" | ">=" ) <shift_expression> }
<shift_expression>       ::= <additive_expression> { ( "<<" | ">>" ) <additive_expression> }
<additive_expression>    ::= <multiplicative_expression> { ( "+" | "-" ) <multiplicative_expression> }
<multiplicative_expression> ::= <unary_expression> { ( "*" | "/" | "%" ) <unary_expression> }

<unary_expression>   ::= ( "!" | "~" | "+" | "-" | "++" | "--" | "*" | "&" ) <unary_expression>
                       | <postfix_expression>

<postfix_expression> ::= <primary_expression> { <postfix_op> }
<postfix_op>         ::= "(" [ <argument_list> ] ")"        // call
                       | "[" <expression> "]"               // index
                       | "." <identifier>                   // member access
                       | "++"                               // postfix increment
                       | "--"                               // postfix decrement

<argument_list>      ::= <assignment_expression> ( "," <assignment_expression> )*

<primary_expression> ::= <literal_expression>
                       | <identifier>
                       | "(" <expression> ")"              // grouping
                       | <cast_expression>
                       | <struct_init>
                       | <union_init>
                       | <array_init>
                       | <switch_expression>
                       | <enum_value>                        // Color.Red handled by member access
                       | <sizeof_expression>
                       | <alignof_expression>

<literal_expression> ::= <number_literal>
                       | <string_literal>
                       | <char_literal>
                       | "true" | "false"
                       | "null"

<switch_expression>  ::= "switch" "(" <expression> ")" "{" { <switch_arm> }* "}"
<switch_arm>         ::= "case" <case_value> ":" <expression> ";"
                       | "default" ":" <expression> ";"
```

### Size and alignment queries

```ebnf
<sizeof_expression>  ::= "sizeof" "(" <sizeof_operand> ")"
<sizeof_operand>     ::= <type> | <expression>
<alignof_expression> ::= "alignof" "(" <type> ")"
```

`sizeof` on a complete non-VLA type is a compile-time `usize`; on a VLA it is a runtime `usize`.
The expression form does not evaluate its operand. `alignof` is a compile-time `usize`;
`alignof` is invalid for `void` and function types.

### Casts

```ebnf
<cast_expression>    ::= "(" <type> ")" <cast_operand>
<cast_operand>       ::= <unary_expression>
```

C-style casts require the parser to know whether the name inside the parentheses is a type. This
is resolved by the type-resolution phase; the grammar allows the ambiguity and the parser defers
it.

### Aggregate initialization

```ebnf
<bound_expr>       ::= <expression>       // see <type_suffix> for constant vs VLA classification
<constant_expression> ::= <expression>   // semantic analysis must prove compile-time evaluation

<struct_init>      ::= "{" <field_initializer_list> "}"
<field_initializer_list> ::= <member_initializer> { "," <member_initializer> }
<member_initializer> ::= "." <identifier> "=" <expression>          // designated
                       | <expression>                                // positional

<union_init>         ::= "{" <union_initializer> "}"
                       | "{" "}"
<union_initializer>  ::= "." <identifier> "=" <expression>
                       | <expression>        // initializes the first member

<array_init>         ::= "{" [ <array_element_list> ] "}"
<array_element_list> ::= <expression> { "," <expression> }

<enum_value>         ::= <identifier> "." <identifier>              // via postfix member access
```

Notes:

- Struct aggregate initializers require every field without a default; duplicate designators,
  positional-after-designated, incompatible values, and excess values are errors.
- Designing a fresh struct via a user-defined type-namespace function (`Point.new(...)`) is an
  ordinary call, not a grammar construct.
- Array initializers follow C partial-initialization: omitted trailing elements are
  zero-initialized; excess elements are errors.
- Union initializers may select at most one member; an empty `{}` zero-initializes the first
  member.

---

## 6. Statements referencing member calls

Method dispatch uses ordinary postfix member access plus a call. Whether a member resolves to a
field, a receiver method, or a no-receiver object-qualified function is decided by name and signa-
ture resolution, not by additional grammar.

```ebnf
// object-based receiver call
<member_call> ::= <postfix_expression> "." <identifier> "(" [ <argument_list> ] ")"
// type-namespace call (static method)
<type_namespace_call> ::= <identifier> "." <identifier> "(" [ <argument_list> ] ")"
```

Both forms are already covered by `<postfix_expression>`; these are listed only to make the
dispatch semantics explicit.

---

## 7. Reserved future syntax

The following is not part of 0.1 and must produce an unsupported-feature diagnostic:

```ebnf
<decorator>          ::= "@" <identifier> [ "(" <expression_list> ")" ]       [reserved]
                       | "@[" <identifier_list> "]"                            [reserved]
<reserved_comptime_decl> ::= "comptime" <declaration>                          [reserved]
<generic_suffix>     ::= "<" <type_list> ">"                                   [reserved]
<comptime_block>     ::= "comptime" <block>                                    [reserved in statement position]
<distinct_type_decl> ::= "distinct" "type" <identifier> "=" <type> ";"         [reserved]
<string_interp>      ::= "\"" { ... "\${" <expression> "}" ... } "\""          [reserved]
<exponentiation>     ::= <unary_expression> "**" <exponentiation>              [reserved]
<type_list>           ::= <type> { "," <type> }
<identifier_list>     ::= <identifier> { "," <identifier> }
<expression_list>     ::= <assignment_expression> { "," <assignment_expression> }
```

The `[reserved]` items are lexed as their own tokens but must produce an unsupported-feature
diagnostic when used in 0.1.

---

## 8. Grammar completeness

Every nonterminal referenced by the 0.1 grammar is defined in this document. Precedence is
expressed by the layered expression rules in Section 5 rather than by separate `<binary_op>` or
`<unary_op>` nonterminals. Reserved future syntax is listed for diagnostics only and does not
expand the accepted 0.1 grammar.
