# HW4 — System Programming Course Website

A single-page interactive website teaching **System Programming** concepts (process management, memory, threads, file I/O, IPC, networking, assembly). Content sourced from Chinese-language course materials and translated to English.

## Live Site

**[https://system-programming-course.vercel.app](https://system-programming-course.vercel.app)**

## Chapters

| # | Topic | Highlights |
|---|-------|------------|
| 1 | Introduction to System Programming | System calls, user vs kernel mode, dev environment |
| 2 | Process Management | `fork()`, `exec()`, `wait()`, signals, zombies |
| 3 | Memory Management | Virtual memory, `malloc`/`mmap`, Valgrind, alignment |
| 4 | Threads & Concurrency | pthread, mutex, deadlock, condition variables, thread-safe queue |
| 5 | File I/O | File descriptors, `dup2`, pipes, shell pipeline simulation |
| 6 | Inter-Process Communication | FIFO, shared memory (SysV & POSIX), message queues, semaphores |
| 7 | Network Programming | TCP/UDP sockets, concurrent servers, `select()`, HTTP parsing |
| 8 | Assembly, Linking & Loading | x86-64 AT&T syntax, ELF format, static/dynamic linking, loader |

Each chapter includes:
- Conceptual explanations
- C code examples (syntax-highlighted)
- Reference tables (signals, syscall flags, IPC mechanisms)
- Chapter summaries

## Project Structure

```
HW4/
├── index.html        # Single-page app (~850 lines)
├── vercel.json       # Vercel static deployment config
├── README.md         # This file
└── session-log.md    # Session history
```

## Architecture

- **Single-page application** — All 8 chapters in one `index.html` with hash-based routing (`#ch1`–`#ch8`)
- **No build step** — Pure HTML5, CSS3, JavaScript with zero dependencies
- **Dark theme** — GitHub-inspired color palette using CSS custom properties
- **Responsive** — Fixed sidebar on desktop; collapses to top nav on mobile (≤768px)
- **Navigation** — Sidebar chapter links + home page with clickable chapter cards

### How It Works

1. `DOMContentLoaded` handler builds the home page grid from chapter metadata
2. Hash change events toggle `.active` on `<section class="chapter">` elements
3. Nav links update `.active` state and scroll to top on chapter switch
4. Scroll-to-top button appears after 300px scroll

## Tech Stack

| Layer | Technology |
|-------|------------|
| Structure | HTML5 semantic elements (`<section>`, `<nav>`, `<pre>`, `<code>`) |
| Styling | CSS3 with custom properties, flexbox, media queries |
| Logic | Vanilla JavaScript (no frameworks) |
| Hosting | Vercel static hosting |

## Source Material

Adapted from the [\_sp repository](https://github.com/how101081/_sp/tree/df4973b75d58d8563c065cfeeee04d6ed1b6693a/homework/sp4) by how101081 (forked from ccc114b/\_sp). Original content was an AI-assisted system programming textbook in Chinese.

## Deployment

```bash
vercel --prod --yes
```

- **Builder**: `@vercel/static`
- **Routing**: All requests served by `index.html` (SPA fallback)
- **Production URL**: `https://system-programming-course.vercel.app`

Re-deploy to update:

```bash
cd HW4
vercel --prod
```

## Local Development

Open `index.html` directly in any browser — no server or build step needed.
