# Arqis Grammar

This document outlines the syntax for Arqis using Extended Backus-Naur Form (EBNF) notation.

*Note: This is a work in progress and will evolve as the language matures.*

### Lexical Rules
```ebnf
<identifier> ::= [a-zA-Z_] [a-zA-Z0-9_]*
<number>     ::= [0-9]+ ( "." [0-9]+ )?
<string>     ::= '"' <any_character>* '"'
<char>       ::= "'" <any_character> "'"
```

### Declarations
```ebnf
<declaration>   ::= <variable_decl> | <function_decl> | <struct_decl> | <enum_decl> | <type_decl> | <import_decl>

<import_decl>   ::= "import" <identifier> ( "." <identifier> )* ";"
<variable_decl> ::= [ "static" ] [ "const" ] <type> <identifier> [ "=" <expression> ] ";"
<function_decl> ::= [ <decorator_list> ] [ "comptime" ] <type> <identifier> "(" <parameter_list> ")" <block>
<struct_decl>   ::= [ <decorator_list> ] "struct" <identifier> "{" <struct_body> "}" ";"
<enum_decl>     ::= "enum" <identifier> "{" <identifier> ( "," <identifier> )* "}" ";"
<type_decl>     ::= "type" <identifier> "=" <type> ";"
```

### Decorators
```ebnf
<decorator_list> ::= "@" <identifier> [ "(" <expression_list> ")" ]
                   | "@[" <identifier_list> "]"
```

### Types
```ebnf
<base_type> ::= "int" | "char" | "float" | "bool" | "void" | <identifier>
<type>      ::= <base_type> { "*" } [ "[" "]" ]
```

### Statements & Blocks
```ebnf
<block>     ::= "{" <statement>* "}"
<statement> ::= <declaration>
              | <expression> ";"
              | "return" [ <expression> ] ";"
              | "defer" <statement>
              | "break" ";"
              | "continue" ";"
              | <if_statement>
              | <while_statement>
              | <for_statement>

<if_statement>    ::= "if" "(" <expression> ")" <block> [ "else" ( <block> | <if_statement> ) ]
<while_statement> ::= "while" "(" <expression> ")" <block>
<for_statement>   ::= "for" "(" [ <variable_decl> | <expression> ] ";" [ <expression> ] ";" [ <expression> ] ")" <block>
```

### Expressions
```ebnf
<expression> ::= <binary_expr>
<binary_expr> ::= <unary_expr> { <binary_op> <unary_expr> }
<unary_expr>  ::= { <unary_op> } <primary_expr>
<primary_expr> ::= <literal>
                 | <identifier>
                 | "(" <expression> ")"
                 | <function_call>
                 | <member_access>
                 | <array_index>
                 | <struct_init>
                 | <array_init>

<function_call> ::= <identifier> "(" [ <expression_list> ] ")"
<member_access> ::= <primary_expr> "." <identifier>
<array_index>   ::= <primary_expr> "[" <expression> "]"
<struct_init>   ::= <identifier> "." "new" "(" [ <expression_list> ] ")"
                  | "{" "." <identifier> "=" <expression> ( "," "." <identifier> "=" <expression> )* "}"
<array_init>    ::= "[" [ <expression_list> ] "]"
```
