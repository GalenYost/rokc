# Switch Expressions and Pattern Matching

Status: Planned for 0.1.0 (switch expressions); richer patterns later

Arqis supports value-producing switch expressions with implicit arm values:

```qis
const int player_health_mult = switch (player.class) {
    case PlayerClass.Warrior: 3;
    case PlayerClass.Mage: 2;
    default: 1;
};
```

Expression-switch arms do not fall through. Every arm yields one value, and all arm values must
have compatible types. `return` inside an arm returns from the enclosing function, not from the
switch expression. Initial patterns are literal values, enum variants, and the `default`
wildcard. Ranges, OR-patterns, guards, and destructuring are future extensions.

Statement switches retain their separate C-style fallthrough semantics.
