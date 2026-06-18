# Docker Build Orchestration Makefile

# Default Architecture
ARCH ?= aarch64
IMAGE_NAME = trustreport_builder
BUILD_DIR = build

.PHONY: all clean build-docker extract

# Default target: build the image and extract the binary
all: build-docker extract

# Build the docker image
# We use --no-cache or a clean build directory approach to ensure it runs again if requested
build-docker:
	@echo "Building Docker image for architecture: $(ARCH)..."
	docker build --platform=linux/amd64 --build-arg ARCH=$(ARCH) --tag $(IMAGE_NAME) .

# Extract the build results from the container
extract:
	@echo "Extracting build results to $(BUILD_DIR)..."
	rm -rf $(BUILD_DIR)
	docker cp $$(docker create --platform=linux/amd64 $(IMAGE_NAME)):/opt/app $(BUILD_DIR)
	@echo "Build artifacts extracted to $(BUILD_DIR)/"

# Clean up the local build directory
clean:
	@echo "Cleaning build directory..."
	rm -rf $(BUILD_DIR)

# Force a rebuild by cleaning and then building
rebuild: clean all
