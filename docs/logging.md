# Logging System

Arqis features a centralized, uniform logging system used across the compiler's codebase. It facilitates debugging and troubleshooting by providing color-coded terminal messages and writing persistent records to a log file.

The logging system is declared in `src/utils.h` and implemented in `src/utils.c`.

---

## Configuration

Configuration options for logging in `config.h`:

```c
#define ENABLE_LOGS 0             // 1 - enable, 0 - disable
#define LOG_FILE_PATH "build.log" // relative to the current working directory
```

`ENABLE_LOGS` acts as a compile-time switch: when set to `0`, the `LOG` macro expands to a no-op, so no logging happens at all. When set to `1`, every `LOG` call writes to both the console and the file.

By default, persistent logs are appended to `build.log` in the current working directory of execution.
Logs are not cleared automatically, so to gather fresh logs and see the result only for last execution - run `./nob clear`

---

## Log Levels

The system supports 5 log levels defined in the `LogLevel` enum (`src/utils.h`):

| Level | Color Prefix (Console) | Description | Typical Use Case |
| :--- | :--- | :--- | :--- |
| `DEBUG` | Cyan | Verbose development details. | Internal AST parsing or memory allocation details. |
| `INFO` | Green | General progress and operations. | Successfully parsed or processed components. |
| `WARN` | Yellow | Warnings about potentially abnormal behavior. | Use of non-critical deprecated features or code configurations. |
| `ERROR` | Red | Non-fatal/recoverable error conditions. | File read failures, syntax validation issues. |
| `FATAL` | Dark Red | Critical issues requiring program termination. | Unrecoverable memory allocation failures. |

---

## API Reference

### Macros

#### `LOG(level, fmt, ...)`
The primary interface for logging within Arqis. This macro automatically writes the formatted log message to both the console (with ANSI color codes) and the log file defined by `LOG_FILE_PATH`.

- **`level`**: One of the `LogLevel` enum values (e.g., `INFO`, `ERROR`).
- **`fmt`**: A `printf`-style format string.
- **`...`**: Variadic arguments corresponding to format specifiers in `fmt`.

```c
#if ENABLE_LOGS
#define LOG(level, ...) \
    do { \
        log_to_console(level, __VA_ARGS__); \
        log_to_file(level, LOG_FILE_PATH, __VA_ARGS__); \
    } while (0)
#else
#define LOG(level, ...) ((void)0)
#endif
```

When `ENABLE_LOGS` is `0`, every `LOG(...)` call is compiled out entirely (expands to `((void)0)`).

### Functions

#### `void log_to_console(LogLevel level, const char *fmt, ...)`
Writes a formatted log message to standard output (`stdout`). The message is prefixed with a colored level identifier (e.g., `[INFO]`) using ANSI terminal escape codes.

#### `void log_to_file(LogLevel level, const char *path, const char *fmt, ...)`
Appends a formatted log message to the file specified by `path`. The message is prefixed with a plain-text level identifier (e.g., `[INFO]`) without ANSI coloring to ensure log files remain readable by generic text editors.

---

## Behavior and Conventions

1. **Non-terminating Design**: The logging functions and macros themselves do **not** call `exit()` or abort the program, even at the `FATAL` level. Code that encounters an unrecoverable error must log the message using `FATAL` and then explicitly handle termination or clean up.
   
   *Example:*
   ```c
   if (!buf) {
       LOG(FATAL, "Could not allocate %ld bytes to read file %s", fsize, path);
       fclose(f);
       exit(1);
   }
   ```

2. **Console Formatting**: Console logs are formatted with color prefixes:
   - `DEBUG`: `\033[36m` (Cyan)
   - `INFO`: `\033[92m` (Green)
   - `WARN`: `\033[33m` (Dark Yellow)
   - `ERROR`: `\033[91m` (Red)
   - `FATAL`: `\033[31m` (Dark Red)

3. **No Trailing Newlines**: When using `LOG`, do not append a trailing newline (`\n`) to your format string. The internal loggers append newlines automatically.
   
   *Incorrect:*
   ```c
   LOG(INFO, "Compiler initialized.\n"); // Will print an extra empty line
   ```
   
   *Correct:*
   ```c
   LOG(INFO, "Compiler initialized.");
   ```
