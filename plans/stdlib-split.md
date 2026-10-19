# Standard-Library Split and External Dependencies — Plan

Status: Unscheduled

## Idea

Split the standard library into two broad categories:

- **Internal standard library**: small, commonly used functionality shipped with RokC or otherwise available as part of the normal toolchain.
- **External libraries**: optional dependencies fetched, built, cached, and linked according to a build recipe.

The goal is to avoid forcing every program to compile or ship the entire standard library.

Possible future build-recipe operations include:

```rok
import std.Build;

void build(Build b) {
    b.link_extern_remote("https://example.invalid/library");
    b.link_extern_local("path/to/library.so");
}
```

## Decisions still required

“Remote source dependency” and “external binary linking” must be separate concepts:

- A remote source dependency must be downloaded, versioned, built for the target, cached, and compiled or linked.
- A local `.so`, `.dll`, or similar file is already a binary and requires ABI metadata, target compatibility, symbol discovery, and a clear ownership/license model.

The design must specify:

- package identity and versioning
- immutable revisions or lock files
- cryptographic hashes or signatures
- offline/reproducible builds
- cache locations and invalidation
- source versus prebuilt binary dependencies
- target platform and architecture selection
- compiler/runtime/ABI compatibility
- static versus dynamic linking
- generated declarations or interface metadata
- license and transitive dependency handling
- network permissions for build scripts
- sandboxing and trust policy
- whether a build may execute arbitrary downloaded code

An operation named `link_extern_trusted` is not sufficient as a security model. Trust should be based on explicit policy, pinned versions, hashes/signatures, or a configured registry—not only on a name chosen by the build author.

Reducing binary size and compile time may also be addressed through separate compilation, caching, dead-code elimination, and lazy module compilation. Splitting the library is useful, but it should not be the only optimization strategy.
