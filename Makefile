# Solakon ONE Monitor - Terminal monitoring tool
# Reads real-time data from Solakon ONE via Modbus TCP
# Displays it in a btop-like terminal interface

.PHONY: help build clean test package

help: ## Show this help message
	@awk 'BEGIN {FS = ":.*##"; printf "\nUsage:\n  make <target>\n\nTargets:\n"} /^[$$()%a-zA-Z_-]+:.*?##/ { printf "  \033[36m%-15s\033[0m %s\n", $$1, $$2 }' $(MAKEFILE_LIST)

build: ## Build the project
	@ninja -C build

clean: ## Clean build artifacts
	@ninja -C build clean
	@rm -rf build
	@echo "Cleaned."

test: build ## Build and run the test suite
	@ninja -C build solakonOne_test
	@./build/solakonOne_test

test-with-coverage: build ## Build and run tests with verbose output
	@ninja -C build solakonOne_test
	@./build/solakonOne_test --success

package: build ## Build Debian package
	@echo "Packaging..."

run: build ## Run the application (default: 192.168.178.121)
	@./build/solakonOne

run-once: build ## Run a single snapshot
	@./build/solakonOne --once
