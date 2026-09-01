# drivers

A C++17 driver library for embedded peripherals on Linux. It provides a common communication layer (UART, I2C, SPI) and sensor/device drivers built on top of it.

## Modules

| Module | Description | Status |
|---|---|---|
| `comms` | UART, I2C, SPI access behind an `ITransport` interface. Thread-safe, validated `Config` types (`UartConfig`, `SpiConfig`, `I2CConfig`). | Stable |
| `imu` | MPU6050 accelerometer/gyroscope driver | In development |
| `barometer` | BMP180 pressure/temperature driver | In development |
| `display` | 2x16 character LCD driver | In development |
| `gpio` | sysfs-based GPIO control | In development |

## Requirements

- CMake >= 3.14
- A C++17-capable compiler (GCC/Clang)
- `libi2c-dev` (I2C/SMBus headers, required by the `comms` target)
- (Optional) `clang-format`, `clang-tidy` - for the code-quality targets

## Building

The project ships `CMakePresets.json` with `default` (debug) and `release` presets, plus a `Makefile` as a shortcut over them.

```bash
make build       # debug build (configures first if needed)
make release     # optimized release build
make rebuild     # clean + build
make test        # build and run tests via ctest
make install     # install to the system with cmake --install
make uninstall   # safely remove installed files
make clean       # remove the build/ directory
```

Equivalent raw CMake:

```bash
cmake --preset default
cmake --build --preset default
ctest --preset default
```

### Code quality

```bash
make format                    # check formatting (no changes)
make format-fix                # auto-format sources
make lint                      # run clang-tidy on all sources
make lint-module MODULE=comms  # run clang-tidy on a single module
make lint-fix                  # clang-tidy --fix
make quality                   # format + lint
```

`make help` lists all available targets; `make info` shows a project summary.

## Project Layout

```
include/drivers/<module>/   Public headers
src/<module>/               Sources + that module's CMakeLists.txt
test/                       Unit tests
cmake/                      Packaging/uninstall helper scripts
```

Each module defines its own static library via `src/<module>/CMakeLists.txt` and is wired into the build with `add_subdirectory(...)` from the top-level `CMakeLists.txt`.

## Code Style and Naming

- Formatting is enforced via `.clang-format` (LLVM-based, 4-space indent, 100-column limit); CI runs `make format`.
- `.cpp`/`.hpp` file names must be `lower_snake_case` (also checked by CI).
- Comments should be short; add a brief doxygen `///` line above a function only when it clarifies non-obvious behavior - avoid comments that just restate what the code already says.

## Commit Convention

Commit messages must follow `<type>: <description>`; CI (`commit-check`) validates this with a regex and rejects commits that don't match:

```
<type>: <short, imperative description>
```

Available `type` values:

| Type | When to use it |
|---|---|
| `feat` | A new feature or capability is added |
| `fix` | A bug is fixed |
| `update` | An existing feature is improved/enhanced |
| `docs` | Documentation-only changes (README, comments, etc.) |
| `style` | Formatting changes with no behavior change |
| `refactor` | Code restructuring with no behavior change |
| `test` | Adding or updating tests |
| `chore` | Maintenance work: build, CI, dependencies, etc. |

Examples:

```
feat: add low pass filter
fix: uart init deadlock on reconnect
update: math library
docs: add build instructions to README
refactor: extract shared posix device open helper
chore: bump cmake minimum version
```

Rules:

- The description starts with a lowercase letter and has no trailing period.
- Each commit represents a single logical change; don't bundle unrelated changes together.
- If a body is needed, add it after a blank line and explain *why*, not *what* (the diff already shows that).

## License

MIT - see [LICENSE](LICENSE) for details.
