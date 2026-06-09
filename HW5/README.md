# HW5: Concurrency and Synchronization

## Table of Contents
1. [Concurrency Concepts](#concurrency-concepts)
2. [Bank Deposit/Withdrawal Simulation](#bank-depositwithdrawal-simulation)
3. [Producer-Consumer Problem](#producer-consumer-problem)
4. [Dining Philosophers Problem](#dining-philosophers-problem)
5. [Build & Run](#build--run)

---

## Concurrency Concepts

### Thread
A **thread** is the smallest unit of execution within a process. Multiple threads within the same process share the same memory space (heap, global variables, file descriptors) but have their own stack and registers. Threads allow a program to perform multiple tasks concurrently, which is especially useful on multi-core systems.

Key properties:
- Threads within a process share the same address space.
- Creating and switching between threads is cheaper than processes.
- Threads can communicate via shared memory directly.

### Race Condition
A **race condition** occurs when multiple threads access shared data concurrently and at least one thread modifies that data, and the outcome depends on the unpredictable order of thread execution. Without synchronization, the final result is non-deterministic and often incorrect.

Example: Two threads both increment `counter++`. This is actually three operations (read, increment, write). If both threads read the same value before either writes, one increment is lost.

### Mutex (Mutual Exclusion)
A **mutex** is a synchronization primitive that prevents multiple threads from entering a critical section simultaneously. A thread must **lock** the mutex before accessing shared data and **unlock** it afterward. Only one thread can hold the lock at a time; others block until it is released.

Pthreads API: `pthread_mutex_init`, `pthread_mutex_lock`, `pthread_mutex_unlock`, `pthread_mutex_destroy`.

### Deadlock
A **deadlock** is a situation where two or more threads are each waiting for a resource held by another, forming a circular wait. None of the threads can proceed, and the program stalls indefinitely.

Four necessary conditions (Coffman conditions):
1. **Mutual Exclusion** – resources cannot be shared.
2. **Hold and Wait** – a thread holds one resource while waiting for another.
3. **No Preemption** – resources cannot be forcibly taken away.
4. **Circular Wait** – a cycle of threads each waiting for a resource held by the next.

Deadlock can be prevented by breaking any one of these conditions.

---

## Bank Deposit/Withdrawal Simulation

### Description
This program simulates a single person performing 100,000 deposit operations and 100,000 withdrawal operations (200,000 total operations) concurrently using multiple threads. The initial balance is $0, and each operation adds or subtracts $1. The final balance must be exactly $0.

### Why synchronization is needed
Without a mutex, the deposit and withdrawal threads race on the shared `balance` variable. The `balance++` and `balance--` operations are not atomic on most architectures — they involve reading, modifying, and writing. Interleaving of these steps causes lost updates, and the final balance will be non-zero.

### Solution
A single `pthread_mutex_t` protects every access to the shared `balance` variable. Each thread locks the mutex, performs the operation, then unlocks. This guarantees mutual exclusion and ensures the final balance is correct.

### Source: `bank.c`

---

## Producer-Consumer Problem

### Description
Also known as the **bounded-buffer problem**. A set of **producer** threads generate items and place them into a fixed-size buffer. A set of **consumer** threads remove items from the buffer and process them. Producers must wait when the buffer is full; consumers must wait when the buffer is empty.

### Synchronization primitives used
- **Mutex** – protects access to the shared buffer and indices.
- **Condition variables** – `pthread_cond_t` used to signal between threads:
  - `cond_not_full` – consumers signal producers when they remove an item (buffer no longer full).
  - `cond_not_empty` – producers signal consumers when they add an item (buffer no longer empty).

### Source: `producer_consumer.c`

---

## Dining Philosophers Problem

### Description
Five philosophers sit around a circular table. Each philosopher alternates between **thinking** and **eating**. To eat, a philosopher needs **both** the left and right chopstick (shared with neighbors). A philosopher can only pick up one chopstick at a time. The challenge is to avoid deadlock and starvation.

### Solution approach
Each chopstick is represented by a mutex. To avoid deadlock caused by every philosopher picking up their left chopstick simultaneously, an asymmetry is introduced:
- Even-indexed philosophers pick up the **left** chopstick first, then the **right**.
- Odd-indexed philosophers pick up the **right** chopstick first, then the **left**.

This breaks the circular-wait condition and prevents deadlock.

### Source: `dining_philosophers.c`

---

## Build & Run

### Prerequisites
- GCC (or any C compiler)
- POSIX threads library (`-lpthread`)
- Linux or WSL environment

### Build
```bash
make all
```

### Run individually
```bash
./bank
./producer_consumer
./dining_philosophers
```

### Clean
```bash
make clean
```
