# Session Log — HW6: Processes & File Descriptors

## Context

- **Repository:** `_sp/` (systems programming exercises)
- **Baseline:** `HW6/` directory existed with an empty `README.md`
- **Sibling project:** `HW5/` (threading exercises — bank, dining philosophers, producer-consumer) was used as style reference

## Goal

Create a C project for **Exercise 6** demonstrating Unix process and file I/O system calls:
`fork`, `execvp`, `close`, `open`, `read`, `write`, `dup2`, and standard file descriptors (`stdin` 0, `stdout` 1, `stderr` 2).

## Decisions

- Followed `HW5/` conventions: individual `.c` files per program, a `Makefile`, and a `README.md`
- Three standalone programs to keep each concept isolated and testable
- Used `-Wall -Wextra -std=c99 -g` for compilation (same as HW5)
- No external libraries beyond POSIX headers

## Architecture

Three independent executables, each focusing on a specific subset of system calls:

```
HW6/
├── file_copy.c    (58 lines)  — open / read / write / close
├── shell.c        (92 lines)  — fork / execvp / waitpid / dup2 / stdin+stdout redirection
├── pipeline.c     (70 lines)  — pipe / fork(2x) / dup2 / execvp
├── Makefile       (19 lines)  — build / clean
└── README.md      (130 lines) — documentation with usage and examples
```

### Program Details

| Program | Syscalls Used | Input | Output |
|---------|--------------|-------|--------|
| `file_copy` | `open`, `read`, `write`, `close` | Source path, dest path | Copies file, prints "Copied successfully." |
| `shell` | `fork`, `execvp`, `waitpid`, `dup2`, `open`, `close` | Interactive prompt (supports `<` and `>` redirection) | Executes commands, respects I/O redirection |
| `pipeline` | `pipe`, `fork`(2x), `dup2`, `execvp`, `waitpid` | Two quoted command strings | Stdout of cmd1 piped to stdin of cmd2 |

## Changes

| File | Action | Summary |
|------|--------|---------|
| `HW6/file_copy.c` | Created | Copies a file using `open`/`read`/`write`/`close`; 4KB buffer, handles partial writes |
| `HW6/shell.c` | Created | Interactive mini-shell: parses `<`/`>`, forks, uses `dup2` to redirect stdin/stdout in child, then `execvp` |
| `HW6/pipeline.c` | Created | Takes two commands, creates a pipe, forks two children, redirects stdout→pipe→stdin via `dup2` |
| `HW6/Makefile` | Created | Builds all three targets; `make` / `make clean` |
| `HW6/README.md` | Written | Full documentation: syscall table, per-program explanation, build/run instructions, example session |

## Status

- All three programs compile **without warnings**
- All three programs tested and **verified working**:
  - `file_copy`: copied "hello world from system calls" correctly
  - `pipeline`: `cat fruits.txt | wc -l` returned correct line count (4)
  - `shell`: `ls -l > listing.txt` produced correct directory listing

No known issues or follow-up needed.

## Commands

```sh
# Build
cd HW6
gcc -Wall -Wextra -std=c99 -g -o file_copy file_copy.c
gcc -Wall -Wextra -std=c99 -g -o shell shell.c
gcc -Wall -Wextra -std=c99 -g -o pipeline pipeline.c

# Or with make (if available)
make
make clean

# Test
echo "hello" > /tmp/a.txt
./file_copy /tmp/a.txt /tmp/b.txt
cat /tmp/b.txt

./pipeline 'cat /tmp/fruits.txt' 'wc -l'

echo -e 'ls -l > /tmp/out.txt\nexit' | ./shell
cat /tmp/out.txt
```
