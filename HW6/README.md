# Homework 6 — Processes & File Descriptors

## System Calls Demonstrated

| Concept       | Calls                                    |
|---------------|------------------------------------------|
| Process       | `fork`, `execvp`, `waitpid`              |
| File I/O      | `open`, `close`, `read`, `write`         |
| Redirection   | `dup2`, `STDIN_FILENO`(0), `STDOUT_FILENO`(1), `STDERR_FILENO`(2) |
| IPC           | `pipe`                                   |

---

## Programs

### 1. `file_copy` — System-call based file copy

Uses `open`/`read`/`write`/`close` (no libc buffered I/O).

```
./file_copy <source> <dest>
```

**Key points:**
- `open()` with `O_RDONLY` for source, `O_WRONLY | O_CREAT | O_TRUNC` for destination
- Reads in 4096-byte chunks via `read()`
- Handles partial `write()` by looping until all bytes written
- Closes both descriptors on all paths

---

### 2. `shell` — Mini shell with I/O redirection

Demonstrates `fork` + `execvp` + `dup2` for `<` and `>` operators.

```
./shell
```

**Built-in commands:** `exit` to quit.

**Redirection examples:**
```
> ls > out.txt
> wc -l < in.txt
> cat < in.txt > out.txt
```

**How it works:**
1. Parses input line, extracting `< infile` and `> outfile` tokens
2. `fork()` creates a child process
3. In the child:
   - For `<`: `open(infile, O_RDONLY)`, then `dup2(fd, STDIN_FILENO)`, then `close(fd)`
   - For `>`: `open(outfile, ...)`, then `dup2(fd, STDOUT_FILENO)`, then `close(fd)`
4. `execvp()` replaces the child with the requested command
5. Parent calls `waitpid()` to reap the child

---

### 3. `pipeline` — Connect two processes via pipe

Demonstrates `pipe` + `fork` + `dup2` + `execvp` to implement `cmd1 | cmd2`.

```
./pipeline '<cmd1>' '<cmd2>'
```

**Example:**
```
./pipeline 'ls -l' 'wc -l'
```

**How it works:**
1. `pipe(pipefd)` creates an anonymous pipe (fd pair)
2. First `fork()`:
   - Closes read-end `pipefd[0]`
   - `dup2(pipefd[1], STDOUT_FILENO)` — redirect stdout to pipe write-end
   - `execvp(cmd1)` — all output goes into the pipe
3. Second `fork()`:
   - Closes write-end `pipefd[1]`
   - `dup2(pipefd[0], STDIN_FILENO)` — redirect stdin to pipe read-end
   - `execvp(cmd2)` — all input comes from the pipe
4. Parent closes both pipe ends and `waitpid`s both children

---

## Build

```sh
make        # build all three programs
make clean  # remove binaries
```

---

## Standard File Descriptors

| FD | Symbolic Constant      | Default      |
|----|------------------------|--------------|
| 0  | `STDIN_FILENO`         | keyboard     |
| 1  | `STDOUT_FILENO`         | terminal     |
| 2  | `STDERR_FILENO`         | terminal     |

`dup2(oldfd, newfd)` makes `newfd` point to the same open file description as `oldfd`, enabling redirection.

---

## Example Session

```sh
# File copy
$ echo "hello world" > a.txt
$ ./file_copy a.txt b.txt
Copied successfully.
$ cat b.txt
hello world

# Mini shell
$ ./shell
> ls -l > listing.txt
> cat listing.txt
... file listing ...
> exit

# Pipeline
$ echo -e "apple\nbanana\ncherry" > fruits.txt
$ ./pipeline 'cat fruits.txt' 'wc -l'
3
Pipeline completed.
```
