# ═══════════════════════════════════════════════════════════
# My View - Complete Makefile (Wayland + Radeon VII Ready)
# ═══════════════════════════════════════════════════════════

TARGET   := myview
SRC_DIR  := src

INC_DIR := include

OBJ_DIR  := obj
SOURCES  := $(wildcard $(SRC_DIR)/main.c $(SRC_DIR)/utils.c $(SRC_DIR)/settings.c $(SRC_DIR)/thumbnails.c $(SRC_DIR)/ui.c $(SRC_DIR)/player.c)
OBJECTS  := $(addprefix $(OBJ_DIR)/,$(notdir $(SOURCES:.c=.o)))

# ── Compiler & Flags ────────────────────────────────────────
CC       := gcc
CFLAGS   := -Wall -Wextra -O2 -march=native -pipe \
            -I. -I$(SRC_DIR) -DRAYGUI_IMPLEMENTATION

# ── Libraries (Wayland Compatible) ──────────────────────────
# CRITICAL: -lEGL must come BEFORE -lGL for Wayland!
RAYLIB_LIBS := $(shell pkg-config --libs raylib 2>/dev/null || echo "-lraylib")
LDFLAGS     := $(RAYLIB_LIBS) -lEGL -lGL -lm -lcjson -lmpv

# ── Build Rules ─────────────────────────────────────────────
.PHONY: all run clean

all: $(TARGET)

$(TARGET): $(OBJECTS) | check_libs
	@echo "Linking $(TARGET)..."
	$(CC) $^ -o $@ $(LDFLAGS)
	@echo "✓ Build complete: ./$(TARGET)"

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c | $(OBJ_DIR)
	@echo "Compiling $<..."
	$(CC) $(CFLAGS) -c $< -o $@

$(OBJ_DIR):
	@mkdir -p $(OBJ_DIR)

# ── Utility Targets ─────────────────────────────────────────
run: $(TARGET)
	@export LIBVA_DRIVER_NAME=radeonsi && ./$(TARGET)

clean:
	rm -rf $(OBJ_DIR) $(TARGET)
	@echo "✓ Cleaned"

# ── Checks ──────────────────────────────────────────────────
check_libs:
	@echo "Checking required libraries..."
	@ldconfig -p | grep -q libEGL || (echo "ERROR: libEGL not found. Install: sudo pacman -S mesa libglvnd" && exit 1)
	@ldconfig -p | grep -q libraylib || (echo "WARNING: libraylib not found" && exit 0)
	@ldconfig -p | grep -q libmpv || (echo "ERROR: libmpv not found. Install: sudo pacman -S mpv" && exit 1)
	@ldconfig -p | grep -q libcjson || (echo "ERROR: libcjson not found. Install: sudo pacman -S cjson" && exit 1)
	@echo "✓ All libraries found"

# ── Info ────────────────────────────────────────────────────
info:
	@echo "Target:    $(TARGET)"
	@echo "Sources:   $(SOURCES)"
	@echo "Objects:   $(OBJECTS)"
	@echo "Compiler:  $(CC)"
	@echo "CFLAGS:    $(CFLAGS)"
	@echo "LDFLAGS:   $(LDFLAGS)"

help:
	@echo "Available targets:"
	@echo "  make         - Build the project"
	@echo "  make run     - Build and run with proper environment"
	@echo "  make clean   - Remove build files"
	@echo "  make info    - Show build configuration"
	@echo "  make help    - Show this help"