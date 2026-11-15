# Shared output directory, toolchains and compiler flags.
# Command-line overrides remain available, e.g. make OUT=build/custom.
OUT := build

COMPILER=em++
AR=emar
C_FLAGS = -O2 -Wall

# preprocessor flags
CPP_FLAGS = -I./celestrak-sgp4/

CELES_FLAGS := \
    -Wno-dangling-else \
    -Wno-unused-but-set-variable \
    -Wno-unused-function

NATIVE_CC ?= cc
NATIVE_CXX ?= c++
NATIVE_AR ?= ar
WASM_CC ?= emcc
WASM_CXX ?= $(COMPILER)
WASM_AR ?= $(AR)
NODE ?= node
PYTHON ?= python3
