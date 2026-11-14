# Production WASM static library and its object files.
# Output for the final bridge combined library
LIB_OUT := $(OUT)/lib

# Output for bridge obj files
OBJ_OUT := $(OUT)/obj

# Output for celestrak obj files
CELES_OUT := $(OUT)/celestrak

TARGET=$(LIB_OUT)/libsgp4.a

CELES_OBJ=$(CELES_OUT)/sgp4.o
BRIDGE_OBJ=$(OBJ_OUT)/sgp4_bridge.o

CELES_SRC=celestrak-sgp4/SGP4.cpp
CELES_INCLUDE=celestrak-sgp4/SGP4.h
BRIDGE_SRC=sgp4_bridge.cpp
BRIDGE_INCLUDE=sgp4_bridge.h

all: $(TARGET)

$(TARGET): $(CELES_OBJ) $(BRIDGE_OBJ) | $(LIB_OUT)
	$(AR) rcs $@ $^
	
$(CELES_OBJ): $(CELES_SRC) $(CELES_INCLUDE) | $(CELES_OUT)
	$(COMPILER) $(C_FLAGS) $(CELES_FLAGS) -c $< -o $@

$(BRIDGE_OBJ): $(BRIDGE_SRC) $(BRIDGE_INCLUDE) $(CELES_INCLUDE) | $(OBJ_OUT)
	$(COMPILER) $(C_FLAGS) $(CPP_FLAGS) -c $< -o $@

$(LIB_OUT) $(CELES_OUT) $(OBJ_OUT): | $(OUT)
	@mkdir -p $@

$(OUT):
	@mkdir -p $@

.PHONY: all
