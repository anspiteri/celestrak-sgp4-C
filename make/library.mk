# Production static libraries, with separate objects for each target.
LIB_OUT := $(OUT)/lib
NATIVE_OBJ_OUT := $(OUT)/obj/native
WASM_OBJ_OUT := $(OUT)/obj/wasm

NATIVE_LIB := $(LIB_OUT)/libsgp4_native.a
WASM_LIB := $(LIB_OUT)/libsgp4_wasm.a

CELES_SRC := celestrak-sgp4/SGP4.cpp
CELES_INCLUDE := celestrak-sgp4/SGP4.h
BRIDGE_SRC := sgp4_bridge.cpp
BRIDGE_INCLUDE := sgp4_bridge.h

.PHONY: all native wasm
all: native wasm
native: $(NATIVE_LIB)
wasm: $(WASM_LIB)

$(NATIVE_LIB): $(NATIVE_OBJ_OUT)/sgp4.o $(NATIVE_OBJ_OUT)/sgp4_bridge.o | $(LIB_OUT)
	$(NATIVE_AR) rcs $@ $^

$(WASM_LIB): $(WASM_OBJ_OUT)/sgp4.o $(WASM_OBJ_OUT)/sgp4_bridge.o | $(LIB_OUT)
	$(WASM_AR) rcs $@ $^

$(NATIVE_OBJ_OUT)/sgp4.o: $(CELES_SRC) $(CELES_INCLUDE) | $(NATIVE_OBJ_OUT)
	$(NATIVE_CXX) -std=c++11 $(C_FLAGS) $(CELES_FLAGS) -c $< -o $@

$(NATIVE_OBJ_OUT)/sgp4_bridge.o: $(BRIDGE_SRC) $(BRIDGE_INCLUDE) $(CELES_INCLUDE) | $(NATIVE_OBJ_OUT)
	$(NATIVE_CXX) -std=c++11 $(C_FLAGS) $(CPP_FLAGS) -c $< -o $@

$(WASM_OBJ_OUT)/sgp4.o: $(CELES_SRC) $(CELES_INCLUDE) | $(WASM_OBJ_OUT)
	$(WASM_CXX) -std=c++11 $(C_FLAGS) $(CELES_FLAGS) -c $< -o $@

$(WASM_OBJ_OUT)/sgp4_bridge.o: $(BRIDGE_SRC) $(BRIDGE_INCLUDE) $(CELES_INCLUDE) | $(WASM_OBJ_OUT)
	$(WASM_CXX) -std=c++11 $(C_FLAGS) $(CPP_FLAGS) -c $< -o $@

$(LIB_OUT) $(NATIVE_OBJ_OUT) $(WASM_OBJ_OUT):
	@mkdir -p $@
