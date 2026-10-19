# Distinct Types — Plan

Status: Unscheduled

The initial `type Name = ExistingType` design will use transparent aliases. A future `distinct` type feature may provide nominal types with the same representation but separate compile-time identity:

```rok
type UserId = int;              // transparent alias
distinct type ProductId = int;  // future nominal type
```

With a distinct type, values such as `UserId` and `ProductId` would not be implicitly interchangeable even though both use an integer representation.

Open questions for the future feature include explicit conversions, operator inheritance, generic behavior, ABI representation, and whether distinct types can be declared over structs, enums, pointers, and aliases.
