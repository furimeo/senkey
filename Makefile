BUILD_DIR ?= build

all:
	@cmake -B $(BUILD_DIR) -DCMAKE_BUILD_TYPE=Release
	@cmake --build $(BUILD_DIR) -j$$(nproc)
	@cp -f $(BUILD_DIR)/senkey ./senkey
	@if [ -f $(BUILD_DIR)/senkey-gui ]; then cp -f $(BUILD_DIR)/senkey-gui ./senkey-gui; fi

test:
	@cmake -B $(BUILD_DIR) -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTS=ON
	@cmake --build $(BUILD_DIR) -j$$(nproc)
	@ctest --test-dir $(BUILD_DIR) --output-on-failure

asan:
	@cmake -B build-asan -DCMAKE_BUILD_TYPE=Debug -DENABLE_ASAN=ON -DBUILD_TESTS=ON
	@cmake --build build-asan -j$$(nproc)
	@ctest --test-dir build-asan --output-on-failure

install: all
	@cmake --install $(BUILD_DIR)

clean:
	@rm -rf build build-asan senkey senkey-gui vtux vtux-gui

.PHONY: all test asan install clean
