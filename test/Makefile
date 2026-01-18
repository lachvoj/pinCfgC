
SHELL = /bin/sh

BUILDDIR=build
SRCDIR=../src
UNITY_ROOT=Unity
MOCKDIR=mocks
TESTDIR=.

CC = gcc
CPP = g++

SRCC = $(wildcard $(SRCDIR)/*.c)
MOCKSC = $(wildcard $(MOCKDIR)/*.c)
SRCCPP = $(wildcard $(SRCDIR)/*.cpp)
MOCKSCPP = $(wildcard $(MOCKDIR)/*.cpp)

# Modular test source files
TESTC = test_main.c test_helpers.c test_core.c test_components.c test_parsing.c \
        test_cli.c test_measurements.c test_edge_cases.c test_integration.c \
        test_persistent_config.c test_stress.c

INCLUDES = -I. -I../include -I$(MOCKDIR) -I$(UNITY_ROOT)/src -I$(SRCDIR)
LIBS = -lm

CFLAGS_BASE = -g
CFLAGS_BASE += -Wall
CFLAGS_BASE += -Wextra
CFLAGS_BASE += -Wpointer-arith
CFLAGS_BASE += -Wcast-align
CFLAGS_BASE += -Wwrite-strings
CFLAGS_BASE += -Wswitch-default
CFLAGS_BASE += -Wunreachable-code
CFLAGS_BASE += -Winit-self
CFLAGS_BASE += -Wmissing-field-initializers
CFLAGS_BASE += -Wno-unknown-pragmas
CFLAGS_BASE += -Wno-misleading-indentation
CFLAGS_BASE += -DUNIT_TEST
CFLAGS_BASE += -DFEATURE_I2C_MEASUREMENT
CFLAGS_BASE += -DFEATURE_SPI_MEASUREMENT
CFLAGS_BASE += -DFEATURE_LOOPTIME_MEASUREMENT
CFLAGS_BASE += -DFEATURE_ANALOG_MEASUREMENT
CFLAGS_BASE += -DMY_TRANSPORT_ERROR_LOG
CFLAGS_BASE += -DMY_TRANSPORT_ERROR_LOG_SIZE=16
#CFLAGS_BASE += -Wstack-usage=200 #-D'F(x)=((const char*)x)'
# CFLAGS_BASE += -fsanitize=vptr -fsanitize=address -static-libasan

# Test binary variants with different flag combinations
EXE1 = test_malloc_errmsg
EXE2 = test_malloc_errcode
EXE3 = test_static_errmsg
EXE4 = test_static_errcode

# Flag combinations
CFLAGS1 = $(CFLAGS_BASE) -D USE_MALLOC -D PINCFG_USE_ERROR_MESSAGES
CFLAGS2 = $(CFLAGS_BASE) -D USE_MALLOC
CFLAGS3 = $(CFLAGS_BASE) -D PINCFG_USE_ERROR_MESSAGES
CFLAGS4 = $(CFLAGS_BASE)

.PHONY: all

all: $(EXE1) $(EXE2) $(EXE3) $(EXE4)

# Quick test target: core + edge cases only (faster iteration)
EXE_QUICK = test_quick
CFLAGS_QUICK = $(CFLAGS_BASE) -D USE_MALLOC -D PINCFG_USE_ERROR_MESSAGES \
               -U RUN_STRESS_TESTS -U RUN_INTEGRATION_TESTS

.PHONY: quick
quick: $(EXE_QUICK)
	@echo "\n=== Running Quick Tests (Core + Edge Cases + CLI + Measurements) ==="
	@ASAN_OPTIONS=detect_leaks=0 ./$(BUILDDIR)/$(EXE_QUICK) 2>&1 | tail -3

.PHONY: full
full: all
	@$(MAKE) run

debug:
	@echo "=== Source Files ==="
	@echo "SRCC: $(SRCC)"
	@echo "MOCKSC: $(MOCKSC)"
	@echo "SRCCPP: $(SRCCPP)"
	@echo "MOCKSCPP: $(MOCKSCPP)"
	@echo ""
	@echo "=== test_malloc_errcode Objects ==="
	@echo "test_malloc_errcode_OBJC: $(test_malloc_errcode_OBJC)"
	@echo ""
	@echo "test_malloc_errcode_OBJCPP: $(test_malloc_errcode_OBJCPP)"

echo:
	@echo $(SRCCPP)
	@echo 
	@echo $(OBJCPP)

run: all
	@echo "\n=== Running test_malloc_errmsg (USE_MALLOC + PINCFG_USE_ERROR_MESSAGES) ==="
	@ASAN_OPTIONS=detect_leaks=0 ./$(BUILDDIR)/$(EXE1) 2>&1 | tail -3
	@echo "\n=== Running test_malloc_errcode (USE_MALLOC only) ==="
	@ASAN_OPTIONS=detect_leaks=0 ./$(BUILDDIR)/$(EXE2) 2>&1 | tail -3
	@echo "\n=== Running test_static_errmsg (PINCFG_USE_ERROR_MESSAGES only) ==="
	@./$(BUILDDIR)/$(EXE3) 2>&1 | tail -3
	@echo "\n=== Running test_static_errcode (No flags) ==="
	@./$(BUILDDIR)/$(EXE4) 2>&1 | tail -3
	@echo ""

qemu:
	@echo "\n=== Building and running QEMU tests ==="
	@cd qemu_test && $(MAKE)

qemu-run:
	@echo "\n=== Building and running QEMU tests ==="
	@cd qemu_test && $(MAKE) run-all

clean:
	-rm -fr $(BUILDDIR)
	@cd qemu_test && $(MAKE) clean 2>/dev/null || true

$(BUILDDIR):
	mkdir -p $@

# Helper function to compile with specific flags
define compile_variant
$(BUILDDIR)/$(1)_obj:
	mkdir -p $$@

$(BUILDDIR)/$(1)_obj/%.o: $(SRCDIR)/%.c | $(BUILDDIR)/$(1)_obj
	$(CC) $(2) $(INCLUDES) -c $$< -o $$@

$(BUILDDIR)/$(1)_obj/%.o: $(MOCKDIR)/%.c | $(BUILDDIR)/$(1)_obj
	$(CC) $(2) $(INCLUDES) -c $$< -o $$@

$(BUILDDIR)/$(1)_obj/%.o: $(UNITY_ROOT)/src/%.c | $(BUILDDIR)/$(1)_obj
	$(CC) $(2) $(INCLUDES) -c $$< -o $$@

$(BUILDDIR)/$(1)_obj/%.o: $(TESTDIR)/test_%.c | $(BUILDDIR)/$(1)_obj
	$(CC) $(2) $(INCLUDES) -c $$< -o $$@

$(BUILDDIR)/$(1)_obj/test_%.o: $(TESTDIR)/test_%.c | $(BUILDDIR)/$(1)_obj
	$(CC) $(2) $(INCLUDES) -c $$< -o $$@

$(BUILDDIR)/$(1)_obj/%.o: $(SRCDIR)/%.cpp | $(BUILDDIR)/$(1)_obj
	$(CPP) $(2) $(INCLUDES) -c $$< -o $$@

$(BUILDDIR)/$(1)_obj/%.o: $(MOCKDIR)/%.cpp | $(BUILDDIR)/$(1)_obj
	$(CPP) $(2) $(INCLUDES) -c $$< -o $$@

$(1)_OBJC = $$(patsubst $(SRCDIR)/%.c, $(BUILDDIR)/$(1)_obj/%.o, $(SRCC))
$(1)_OBJC += $$(patsubst $(MOCKDIR)/%.c, $(BUILDDIR)/$(1)_obj/%.o, $(MOCKSC))
$(1)_OBJC += $(BUILDDIR)/$(1)_obj/unity.o
$(1)_OBJC += $$(patsubst %.c, $(BUILDDIR)/$(1)_obj/%.o, $(TESTC))
$(1)_OBJCPP = $$(patsubst $(SRCDIR)/%.cpp, $(BUILDDIR)/$(1)_obj/%.o, $(SRCCPP))
$(1)_OBJCPP += $$(patsubst $(MOCKDIR)/%.cpp, $(BUILDDIR)/$(1)_obj/%.o, $(MOCKSCPP))

$(1): $$($(1)_OBJC) $$($(1)_OBJCPP) | $(BUILDDIR)
	$(CPP) $(2) $(INCLUDES) -o $(BUILDDIR)/$$@ $$($(1)_OBJC) $$($(1)_OBJCPP) $(LIBS)
endef

# Generate build rules for each variant
$(eval $(call compile_variant,$(EXE1),$(CFLAGS1)))
$(eval $(call compile_variant,$(EXE2),$(CFLAGS2)))
$(eval $(call compile_variant,$(EXE3),$(CFLAGS3)))
$(eval $(call compile_variant,$(EXE4),$(CFLAGS4)))
$(eval $(call compile_variant,$(EXE_QUICK),$(CFLAGS_QUICK)))

