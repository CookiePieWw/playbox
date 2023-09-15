# Modified from https://github.com/NJU-ProjectN/abstract-machine/blob/master/Makefile
## 1. Basic Setup and Checks

### Default to compile a single file
ifeq ($(MAKECMDGOALS),)
  MAKECMDGOALS  = compile
  .DEFAULT_GOAL = compile
endif

### Override checks when `make clean/clean-all/html`
ifeq ($(findstring $(MAKECMDGOALS),clean),)

### Print build info message
$(info # Building $(NAME)-$(MAKECMDGOALS) [$(ARCH)])

### Check if there is something to build
ifeq ($(flavor SRCS), undefined)
  $(error Nothing to build)
endif

### Checks end here
endif

## 2. General Compilation Targets

### Create the destination directory (`build/$ARCH`)
WORK_DIR  = $(shell pwd)
DST_DIR   = $(WORK_DIR)/build
$(shell mkdir -p $(DST_DIR))

IMAGE     = $(DST_DIR)/$(NAME)

## 3. General Compilation Flags

### compilers
CC        = gcc
CXX       = g++

### Compilation flags
INC_PATH += $(WORK_DIR)/include
INCFLAGS += $(addprefix -I, $(INC_PATH))

CFLAGS   += -O2 -Wall -Werror $(INCFLAGS)
CXXFLAGS += $(CFLAGS)

## 5. Compilation Rules

### Rule (compile): a single `.c` -> exec (gcc)
$(DST_DIR)/%: %.c
	@mkdir -p $(dir $@) && echo + CC $<
	@$(CC) -std=gnu11 $(CFLAGS) -g -o $@ $(realpath $<)

### Rule (compile): a single `.cpp` -> exec (g++)
$(DST_DIR)/%: %.cpp
	@mkdir -p $(dir $@) && echo + CXX $<
	@$(CXX) -std=c++17 $(CXXFLAGS) -o $@ $(realpath $<)

## 6. Miscellaneous

.PHONY: run gdb compile

compile: $(IMAGE)
	@echo + build $(IMAGE)

run: compile
	@echo + run $(IMAGE)
	@$(IMAGE)

gdb: compile
	@echo + gdb $(IMAGE)
	@gdb $(IMAGE)

### Clean a single project (remove `build/`)
clean:
	rm -rf $(WORK_DIR)/build/
.PHONY: clean
