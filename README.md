# Playbox

A collection of small programming projects for learning and experimentation.

## Projects

- [**pstree**](c/pstree/) - Unix-like process tree viewer
- [**expr**](cpp/expr/) - Command-line expression calculator  
- [**coroutine**](cpp/coroutine/) - C++20 coroutine fibonacci generator
- [**timewheel-lru**](cpp/timewheel-lru/) - LRU cache with ttl based on a time-wheel algorithm
- [**linux-io**](cpp/linux-io/) - Linux IO examples for buffered IO, direct IO, AIO, and io-uring

## Quick Start

```bash
# List all available projects
just list

# Build and run a specific project
just build <project-name>
just run <project-name> [args...]
```

Install [just](https://github.com/casey/just) to use the build system.
