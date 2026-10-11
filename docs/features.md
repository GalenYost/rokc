# RokC Language Features

### Clarifications on OOP
OOP should be simple, it shouldnt complicate things, especially taking the fact that OOP is not necessary, so in RokC theres no inheritance, no private fields, just pure structs that contain fields and functions.
If you want to inherit something - you can attach decorator to a struct, this way you can have a struct that can be used as sort of a "template" for your future structs that will use that exact decorator as a strict path to follow

### Clarifications on build scripts
There are rust and zig as good examples of how build scripts should work, but in my opinion they are a bit more complicated, it feels like learning yet another part of the language.
RokC build system will allow you to simply define what you want from a compiler

### Clarifications on std library

A language should be self-explorable — you shouldn't need documentation just to figure out what a name means.
Instead of huge files like in C/C++ or long complicated names like in Java, RokC uses simple, self-explanatory module paths.

#### Examples

**C++** — not really self-explorable without docs:
```cpp
#include <unordered_map>

int main(void) {
    std::unordered_map<string, string> names;
}
```

**Java** — better, but still verbose:
```java
import java.util.HashMap;

public class Main {
    public static void main(String[] args) {
        HashMap<String, String> map = new HashMap<>();
    }
}
```

**Rust** — nice and simple:
```rs
use std::collections::HashMap;

fn main() {
    let map: HashMap<&str, &str> = HashMap::new();
}
```

**RokC** — takes C as a simplicity standard, goes even further:
```rkc
import std.collections.HashMap;

int main(void) {
    HashMap<char*, char*> map = HashMap.new();
}
```

RokC combines the simplicity of C, the modern readability of Java, and the self-explanatory naming of Rust.

### Clarifications on decorators
Basically decorators are macros in a way, but they operate on nodes, not strings, and the type checker runs downstream of
them, this is exactly perfect to enforce strict rules, while maintaining flexibility of concept as it is.

A nice example of decorators is Zig, which can achieve similar results through compile-time metaprogramming.
RokC uses the same comptime foundation, but wraps it in dedicated decorator syntax with type checking running downstream of them, which makes them more ergonomic and easier to reason about than raw metaprogramming.
A few examples:
```rkc
import std.tag; // existing in stdlib decorator
import std.io.print;

const Extern = @tag("Extern");
const Flag = @tag("Flag");

// @Extern can be used for single tag
// or
// @[Extern, Flag] for multiple tags
// no one stops you from doing @[Extern] tho
void print_hello() {
    print("Hello, world!");
}

int main(void) {
    return 0;
}
```
This is perfect system for tagging functions, this will help with embeddability (such as game engines for example) and custom marking of functions, structs or even expressions

Small example:
```rkc
import std.meta.StructDecl;

comptime StructDecl Serializable(StructDecl node) {
    node.inject_method("
        char* to_json() {}
    ");
    node.inject_fields("
        static int static_var1;
        int var2;
    "); // multiple fields at once
    node.inject_field("static bool static_var3"); // one field
    return node;
}

@Serializable
struct Player {
    int health;
    char* name;

    Player new(int health, char* name) {
        return {
            .health = health,
            .name = name,
        };
    }
};

int main(void) {
    Player p = Player.new(100, "Player1");

    char* json = p.to_json();
}
```
Another example with functions:
```rkc
import std.meta.FuncDecl;

comptime FuncDecl Route(FuncDecl node, char* path) {
    // create a registry for web framework to use later
    return node;
}

@Route("/api/users/get")
char*[] get_users() {
    return [];
}
```
Those examples are planned to exist in stdlib, but
hope you get the idea — they work similar to Rust macros and Python decorators and can extend functionality, with the type checker running downstream of them keeping things strict and predictable.

### Clarifications on visibility (public/private)
By default all of the expressions and functions are private, no need to mark them static (unless needed to preserve memory address).
Instead you will have an std library decorators: Export and ExportAll. At first glance it looks like a crutch, but it actually is perfectly valid to solve the visibility problem.
ExportAll is module-wide decorator, such decorators apply to the whole file where they are being used, can be used in case of creating big type / constants declaration modules.
Single Export as opposed can be used if you know you need this constant of function to be visible and usable from outside.
Example:
```rkc
// state.rkc
import std.ExportAll;
@[ExportAll]

static const int DEFAULT_THREADS = 12; // this is not exported, Export decorator simply ignores static variables and functions

// this struct is exported
struct AppState {
    static bool is_running = false;
    static int threads = DEFAULT_THREADS; // this can be changed, but DEFAULT_THREADS is always the same
};

static void private_helper() {} // this function stays private

void init() {} // this function is exported and can be accessed via state.init();
// can be moved directly to AppState tho, but the idea does not change
```
Basic idea is:
1. you can ExportAll if you know you will have to export a lot of things, but you can still mark something static to prevent its export
2. you can Export idividual parts, this adds some readability complexity, but not really a big issue and you wont have to mark everything else as static
