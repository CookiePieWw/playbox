# Top-level justfile for managing all projects

# List all available projects
list:
    @echo "Available projects:"
    @echo "  C projects:"
    @find c -mindepth 1 -maxdepth 1 -type d | sed 's|^c/|    c/|'
    @echo "  C++ projects:"
    @find cpp -mindepth 1 -maxdepth 1 -type d | sed 's|^cpp/|    cpp/|'

# Build a specific project
build project:
    #!/usr/bin/env bash
    set -euo pipefail
    if [[ -d "c/{{project}}" ]]; then
        cd "c/{{project}}" && just build
    elif [[ -d "cpp/{{project}}" ]]; then
        cd "cpp/{{project}}" && just build
    else
        echo "Project '{{project}}' not found in c/ or cpp/ directories"
        exit 1
    fi

# Run a specific project
run project +args="":
    #!/usr/bin/env bash
    set -euo pipefail
    if [[ -d "c/{{project}}" ]]; then
        cd "c/{{project}}" && just run {{args}}
    elif [[ -d "cpp/{{project}}" ]]; then
        cd "cpp/{{project}}" && just run {{args}}
    else
        echo "Project '{{project}}' not found in c/ or cpp/ directories"
        exit 1
    fi

# Build all projects
build-all:
    @echo "Building all C projects..."
    @for dir in c/*/; do \
        echo "Building $(basename $dir) in c/..."; \
        cd "$dir" && just build || echo "Failed to build $(basename $dir)"; \
        cd - > /dev/null; \
    done
    @echo "Building all C++ projects..."
    @for dir in cpp/*/; do \
        echo "Building $(basename $dir) in cpp/..."; \
        cd "$dir" && just build || echo "Failed to build $(basename $dir)"; \
        cd - > /dev/null; \
    done

# Clean all projects
clean:
    @echo "Cleaning all projects..."
    @for dir in c/*/ cpp/*/; do \
        if [[ -f "$dir/justfile" ]]; then \
            echo "Cleaning $(basename $dir)..."; \
            cd "$dir" && just clean || echo "Failed to clean $(basename $dir)"; \
            cd - > /dev/null; \
        fi \
    done

# Format all source files
format:
    @echo "Formatting all source files..."
    @find . -name "*.c" -o -name "*.cpp" -o -name "*.h" -o -name "*.hpp" | while read file; do \
        if [[ -f "$file" ]]; then \
            echo "Formatting $file"; \
            clang-format -i "$file"; \
        fi \
    done