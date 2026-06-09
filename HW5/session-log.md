# Session Log — HW5: Concurrency and Synchronization

## Context
- **Project**: HW5 (standalone concurrency assignment)
- **Baseline**: Empty `README.md` in `HW5/` directory
- **Environment**: Arch Linux (WSL2), root user, no tools pre-installed

## Goal
Create a project demonstrating concurrency concepts:
1. Documentation explaining threads, race conditions, mutex, deadlock
2. Bank deposit/withdrawal simulation with 100,000 operations — balance must be correct
3. Producer-consumer simulation
4. Dining philosophers simulation
5. All documentation in `README.md`, all programs in English

## Files Created

| File | Lines | Description |
|---|---|---|
| `HW5/README.md` | 86 | Full documentation: concurrency concepts + program explanations + build instructions |
| `HW5/bank.c` | 54 | 2 deposit + 2 withdraw threads, 100k ops each, mutex-protected balance |
| `HW5/producer_consumer.c` | 102 | Bounded buffer with 2 producers, 2 consumers, mutex + condition variables |
| `HW5/dining_philosophers.c` | 69 | 5 philosophers, 3 meals each, deadlock prevention via asymmetric chopstick pickup |
| `HW5/Makefile` | 18 | Build targets for all 3 programs + clean |

## Decisions

- **Language**: C with POSIX threads (`pthread`) — standard for OS-level concurrency demos
- **Bank**: Used `pthread_mutex_t` wrapping every `balance++` / `balance--` to guarantee atomicity
- **Producer-Consumer**: Used `pthread_cond_t` (two condition variables: `cond_not_full`, `cond_not_empty`) with `while` loops guarding `pthread_cond_wait` to handle spurious wakeups
- **Dining Philosophers**: Deadlock prevented via asymmetric lock ordering (even-indexed philosophers grab left-first, odd grab right-first) — breaks circular wait
- **Fixed compiler warnings**: Added `(void)arg` to suppress unused parameter warnings

## Test Results

- **bank** (5 runs): All PASS, final balance = 0 every time
- **producer_consumer**: 20 items produced = 20 consumed, buffer empty at end
- **dining_philosophers**: 5 philosophers × 3 meals = 15 eat events, no deadlock, all finished

## Build & Run

```bash
# Build
gcc -Wall -Wextra -pthread -o bank bank.c
gcc -Wall -Wextra -pthread -o producer_consumer producer_consumer.c
gcc -Wall -Wextra -pthread -o dining_philosophers dining_philosophers.c

# Or with make:
make all

# Run
./bank
./producer_consumer
./dining_philosophers

# Clean
make clean
```

## Status
- All 3 programs compile with no errors (2 warnings fixed)
- All 3 programs produce correct output
- Git not configured — remote push not completed
