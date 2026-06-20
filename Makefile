PROJECT_NAME := drivers

# ANSI color codes
RED    := \033[0;31m
GREEN  := \033[0;32m
YELLOW := \033[0;33m
BLUE   := \033[0;34m
PURPLE := \033[0;35m
CYAN   := \033[0;36m
RESET  := \033[0m

INFO  = printf "$(CYAN)[INFO]$(RESET) %s\n"
OK    = printf "$(GREEN)[OK]$(RESET) %s\n"
WARN  = printf "$(YELLOW)[WARN]$(RESET) %s\n"
ERROR = printf "$(RED)[ERROR]$(RESET) %s\n"

# Target directories
BUILD_DIR         := build/debug
RELEASE_DIR       := build/release
SRC_DIR           := src
INCLUDE_DIR       := include
TEST_DIR          := test

# Files fed to clang-format / clang-tidy.
CPP_FILES := $(shell find $(SRC_DIR) $(INCLUDE_DIR) $(TEST_DIR) \
               \( -name '*.cpp' -o -name '*.hpp' \) 2>/dev/null)

CMAKE_CACHE := $(BUILD_DIR)/CMakeCache.txt
MAKEFLAGS   += --no-print-directory

.PHONY: all
all: build

.PHONY: configure
configure:
	@$(INFO) "Configuring CMake (preset: default)..."
	@cmake --preset default
	@$(OK) "Configuration complete."

.PHONY: build
build:
	@if [ ! -f $(CMAKE_CACHE) ]; then \
		$(WARN) "CMake cache not found - configuring first..."; \
		cmake --preset default; \
	fi
	@$(INFO) "Building..."
	@cmake --build --preset default
	@$(OK) "Build complete."

.PHONY: release
release:
	@$(INFO) "Building optimized release..."
	@cmake --preset release
	@cmake --build --preset release
	@$(OK) "Release build complete."

.PHONY: rebuild
rebuild: clean build

.PHONY: test
test: build
	@$(INFO) "Running tests..."
	@ctest --preset default

.PHONY: install
install: build
	@$(INFO) "Installing drivers..."
	@cmake --install $(BUILD_DIR)
	@$(OK) "Install complete."

.PHONY: uninstall
uninstall:
	@if [ ! -f $(BUILD_DIR)/install_manifest.txt ]; then \
		$(ERROR) "No install manifest found in $(BUILD_DIR)."; \
		$(WARN)  "Nothing to uninstall (was it installed from this build?)."; \
		exit 1; \
	fi
	@$(WARN) "Uninstalling drivers (removing installed files)..."
	@cmake --build $(BUILD_DIR) --target uninstall
	@$(OK) "Uninstall complete."

.PHONY: format
format:
	@$(INFO) "Checking formatting..."
	@clang-format --dry-run --Werror $(CPP_FILES)
	@$(OK) "Formatting OK."

.PHONY: format-fix
format-fix:
	@$(INFO) "Formatting sources..."
	@clang-format -i $(CPP_FILES)
	@$(OK) "Formatting done."

.PHONY: lint
lint: configure
	@$(INFO) "Running clang-tidy..."
	@clang-tidy -p $(BUILD_DIR) \
		--extra-arg=-I/usr/include/c++/11 \
		--extra-arg=-I/usr/include/x86_64-linux-gnu/c++/11 \
		$(CPP_FILES)

.PHONY: lint-fix
lint-fix: configure
	@$(INFO) "Running clang-tidy with --fix..."
	@clang-tidy -p $(BUILD_DIR) --fix $(CPP_FILES)

.PHONY: lint-module
lint-module: configure
	@if [ -z "$(MODULE)" ]; then \
		$(ERROR) "Usage: make lint-module MODULE=<name>  (e.g. progress)"; \
		exit 1; \
	fi
	@$(INFO) "Running clang-tidy on module '$(MODULE)'..."
	@FILES=$$(find $(SRC_DIR)/$(MODULE) $(INCLUDE_DIR)/drivers/$(MODULE) \
		\( -name '*.cpp' -o -name '*.hpp' \) 2>/dev/null); \
	if [ -z "$$FILES" ]; then \
		$(ERROR) "No files found for module '$(MODULE)'."; \
		exit 1; \
	fi; \
	clang-tidy -p $(BUILD_DIR) \
		--extra-arg=-I/usr/include/c++/11 \
		--extra-arg=-I/usr/include/x86_64-linux-gnu/c++/11 \
		$$FILES

.PHONY: quality
quality: format lint

.PHONY: clean
clean:
	@$(WARN) "Cleaning build artifacts..."
	@rm -rf build
	@$(OK) "Clean complete."

.PHONY: info
info:
	@printf "$(BLUE)========================================$(RESET)\n"
	@printf "$(BLUE)  Project  :$(RESET) $(PROJECT_NAME)\n"
	@printf "$(BLUE)  Std      :$(RESET) C++17\n"
	@printf "$(BLUE)  Sources  :$(RESET) $(words $(CPP_FILES)) files\n"
	@printf "$(BLUE)  Build    :$(RESET) $(BUILD_DIR)\n"
	@printf "$(BLUE)========================================$(RESET)\n"

.PHONY: help
help:
	@printf "$(PURPLE)========================================$(RESET)\n"
	@printf "$(PURPLE)  $(PROJECT_NAME) - Available Targets$(RESET)\n"
	@printf "$(PURPLE)========================================$(RESET)\n"
	@printf "\n"
	@printf "$(BLUE)  Build:$(RESET)\n"
	@printf "    $(CYAN)configure$(RESET)      Configure CMake (no compile)\n"
	@printf "    $(CYAN)build$(RESET)          Compile (debug)\n"
	@printf "    $(CYAN)release$(RESET)        Compile optimized release\n"
	@printf "    $(CYAN)rebuild$(RESET)        Clean then build\n"
	@printf "    $(CYAN)test$(RESET)           Build and run tests\n"
	@printf "    $(CYAN)clean$(RESET)          Remove all build artifacts\n"
	@printf "\n"
	@printf "$(BLUE)  Install:$(RESET)\n"
	@printf "    $(CYAN)install$(RESET)        Install to system\n"
	@printf "    $(CYAN)uninstall$(RESET)      Safely remove installed files\n"
	@printf "\n"
	@printf "$(BLUE)  Code Quality:$(RESET)\n"
	@printf "    $(CYAN)format$(RESET)         Check formatting (no changes)\n"
	@printf "    $(CYAN)format-fix$(RESET)     Auto-format sources\n"
	@printf "    $(CYAN)lint$(RESET)           Run clang-tidy\n"
	@printf "    $(CYAN)lint-module$(RESET)    Run clang-tidy on one module (MODULE=name)\n"
	@printf "    $(CYAN)lint-fix$(RESET)       Run clang-tidy with --fix\n"
	@printf "    $(CYAN)quality$(RESET)        format + lint\n"
	@printf "\n"
	@printf "$(BLUE)  Info:$(RESET)\n"
	@printf "    $(CYAN)info$(RESET)           Show project info\n"
	@printf "    $(CYAN)help$(RESET)           Show this help\n"
	@printf "\n"
	@printf "$(PURPLE)========================================$(RESET)\n"