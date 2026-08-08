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
	@cd build && ninja && ./solakon-monitor_test

### Package

package: ## Build Debian package
	dpkg-buildpackage -us -uc -b

### Run

run: build ## Run the monitor
	@cd build && ./solakon-monitor

run-once: build ## Run single snapshot
	@cd build && ./solakon-monitor --once

### Art Mode

run-art: build ## Run monitor in ASCII art mode
	@cd build && ./solakon-monitor --art

### Docs

docs: ## Generate Doxygen docs
	doxygen docs/Doxyfile

docs-clean: ## Clean Doxygen docs
	@rm -rf docs/html docs/xml

### Coverage

coverage: ## Run tests with coverage report
	@cd build && ninja solakon-monitor_test
	@cd build && ./solakon-monitor_test >/dev/null 2>&1
	@cd build && lcov --capture --directory solakon-monitor_test.p --output-file coverage.info --branch-coverage
	@lcov --remove build/coverage.info '/usr/*' '*/tests/*' --output-file coverage_filtered.info
	@genhtml coverage_filtered.info --output-directory coverage-report
	@echo "Coverage report: coverage-report/index.html"

coverage-clean: ## Clean coverage data
	@rm -rf build/coverage.info build/coverage_filtered.info build/coverage-report build/*.gcno build/*.gcda

### Clean

clean-all: clean docs-clean coverage-clean ## Cleans everthing.
	@echo All cleaned up.	
