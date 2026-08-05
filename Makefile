.PHONY: all build clean test package run run-once docs docs-clean
include Makefile.nice

### Build

build: ## Build the project
	@cd build && ninja

clean: ## Clean build artifacts
	@cd build && ninja clean || true
	@rm -rf build
	@mkdir -p build
	@cd build && meson .. 2>/dev/null || true

### Test

test: ## Run tests
	@cd build && ninja && ./solakonOne_test

### Package

package: ## Build Debian package
	dpkg-buildpackage -us -uc -b

### Run

run: build ## Run the monitor
	@cd build && ./solakonOne

run-once: build ## Run single snapshot
	@cd build && ./solakonOne --once

### Docs

docs: ## Generate Doxygen docs
	doxygen docs/Doxyfile

docs-clean: ## Clean Doxygen docs
	@rm -rf docs/html docs/xml
