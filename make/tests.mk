# Native and WASM verification, cross-target comparison, and sanitizers.
TEST_CPP := tests/test_suite.cpp tests/test_modes.cpp
TEST_C := tests/test_api.c tests/test_modes.c
TEST_SOURCES := $(TEST_CPP) $(TEST_C)
TEST_HEADERS := $(BRIDGE_INCLUDE) $(CELES_INCLUDE) tests/test_support.h
FIXTURES := tests/fixtures/a_elements.csv tests/fixtures/a_states.csv tests/fixtures/i_elements.csv tests/fixtures/i_states.csv
NATIVE_TEST_DIR := $(OUT)/tests/native
WASM_TEST_DIR := $(OUT)/tests/wasm
NATIVE_TEST_LIB := $(NATIVE_TEST_DIR)/libsgp4.a
NATIVE_C_OBJECTS := $(patsubst tests/%.c,$(NATIVE_TEST_DIR)/%.o,$(TEST_C))
WASM_C_OBJECTS := $(patsubst tests/%.c,$(WASM_TEST_DIR)/%.o,$(TEST_C))

.PHONY: test test-native test-wasm test-sanitize
test: $(NATIVE_TEST_DIR)/test_suite $(NATIVE_TEST_DIR)/test_faults $(WASM_TEST_DIR)/test_suite.js $(WASM_TEST_DIR)/test_faults.js
	$(PYTHON) tests/compare_targets.py $(NATIVE_TEST_DIR) $(WASM_TEST_DIR) $(NODE)

test-native: $(NATIVE_TEST_DIR)/test_suite $(NATIVE_TEST_DIR)/test_faults
	./$(NATIVE_TEST_DIR)/test_suite
	./$(NATIVE_TEST_DIR)/test_faults

$(NATIVE_TEST_DIR)/%.o: tests/%.c $(BRIDGE_INCLUDE)
	@mkdir -p $(@D)
	$(NATIVE_CC) -std=c11 $(C_FLAGS) -Wextra -I. -c $< -o $@

$(NATIVE_TEST_DIR)/sgp4.o: $(CELES_SRC) $(CELES_INCLUDE)
	@mkdir -p $(@D)
	$(NATIVE_CXX) -std=c++11 $(C_FLAGS) $(CELES_FLAGS) -c $< -o $@

$(NATIVE_TEST_DIR)/bridge.o: $(BRIDGE_SRC) $(TEST_HEADERS)
	@mkdir -p $(@D)
	$(NATIVE_CXX) -std=c++11 $(C_FLAGS) $(CPP_FLAGS) -c $< -o $@

$(NATIVE_TEST_LIB): $(NATIVE_TEST_DIR)/bridge.o $(NATIVE_TEST_DIR)/sgp4.o
	$(NATIVE_AR) rcs $@ $^

$(NATIVE_TEST_DIR)/test_suite: $(TEST_CPP) $(NATIVE_C_OBJECTS) $(TEST_HEADERS) $(NATIVE_TEST_LIB) $(FIXTURES)
	$(NATIVE_CXX) -std=c++11 $(C_FLAGS) $(CPP_FLAGS) -I. $(TEST_CPP) $(NATIVE_C_OBJECTS) $(NATIVE_TEST_LIB) -o $@

$(NATIVE_TEST_DIR)/test_faults: tests/test_faults.cpp $(BRIDGE_SRC) $(TEST_HEADERS)
	@mkdir -p $(@D)
	$(NATIVE_CXX) -std=c++11 $(C_FLAGS) $(CPP_FLAGS) -I. tests/test_faults.cpp $(BRIDGE_SRC) -o $@

test-wasm: $(WASM_TEST_DIR)/test_suite.js $(WASM_TEST_DIR)/test_faults.js
	$(NODE) $(WASM_TEST_DIR)/test_suite.js
	$(NODE) $(WASM_TEST_DIR)/test_faults.js

$(WASM_TEST_DIR)/%.o: tests/%.c $(BRIDGE_INCLUDE)
	@mkdir -p $(@D)
	$(WASM_CC) -std=c11 $(C_FLAGS) -Wextra -I. -c $< -o $@

$(WASM_TEST_DIR)/test_suite.js: $(TEST_CPP) $(WASM_C_OBJECTS) $(TEST_HEADERS) $(TARGET) $(FIXTURES)
	$(COMPILER) -std=c++11 $(C_FLAGS) $(CPP_FLAGS) -I. $(TEST_CPP) $(WASM_C_OBJECTS) $(TARGET) -sENVIRONMENT=node -sSINGLE_FILE=1 $(foreach file,$(FIXTURES),--embed-file $(file)) -o $@

$(WASM_TEST_DIR)/test_faults.js: tests/test_faults.cpp $(BRIDGE_SRC) $(TEST_HEADERS)
	@mkdir -p $(@D)
	$(COMPILER) -std=c++11 $(C_FLAGS) $(CPP_FLAGS) -I. tests/test_faults.cpp $(BRIDGE_SRC) -sENVIRONMENT=node -sSINGLE_FILE=1 -o $@

test-sanitize:
	$(MAKE) test-native OUT=$(OUT)/sanitize C_FLAGS='-O1 -g -Wall -fsanitize=address,undefined -fno-omit-frame-pointer'

