# Scopes and Declarations

Status: Planned for 0.1.0

Arqis uses lexical block scopes. A declaration in an inner block may shadow a declaration with
the same name in an outer block:

```qis
int value = 1;
{
    int value = 2;
    print(value); // 2
}
print(value); // 1
```

Name lookup resolves the nearest visible binding. Duplicate declarations in the same scope are
rejected unless a future feature explicitly defines a separate namespace or overload rule. A
local binding ends at its scope's closing brace.

Declaration visibility before the source position, function prototypes/forward declarations,
and interactions between type, value, and module namespaces remain open and must be specified
before the first release is complete.

Functions used before their definitions require explicit C-style prototypes:

```qis
int add(int, int);
int main() { return add(1, 2); }
int add(int left, int right) { return left + right; }
```

There is no implicit function hoisting. A prototype and its definition must have compatible
signatures, and a declared function must eventually have a definition for executable builds.

Function names are unique within a module in the initial language. Parameter-type-based
overloading is not supported; struct methods follow the same rule.

Recursive or otherwise forward-referenced structs require explicit incomplete declarations before
their definitions. Declarations-first organization is recommended but not enforced globally.

The same incomplete-declaration rule applies to unions: pointers may use an incomplete union,
while by-value fields and layout queries require the completed definition.

Type names cannot be reused by values, constants, functions, or other declarations in the same
scope/module. Nested shadowing of an outer type name remains a separate decision.

Type names cannot be shadowed by values in nested scopes. They remain visible through the scope
chain, while ordinary value-to-value shadowing remains allowed.

Variable declarations use explicit types in the initial language; `auto` and `var` inference are
not provided.
