# Makefile for STM32F1XX Firmware Tests (Multi-Target Test Suite)
# Builds multiple test ELF files to fit within 65KB flash limit

# Toolchain - Use PlatformIO's toolchain if available, otherwise system toolchain
PLATFORMIO_ARM_TOOLCHAIN := $(HOME)/.platformio/packages/toolchain-gccarmnoneeabi/bin
ifneq (,$(wildcard $(PLATFORMIO_ARM_TOOLCHAIN)/arm-none-eabi-gcc))
    TOOLCHAIN_PREFIX := $(PLATFORMIO_ARM_TOOLCHAIN)/arm-none-eabi-
    $(info Using PlatformIO ARM toolchain: $(PLATFORMIO_ARM_TOOLCHAIN))
else
    TOOLCHAIN_PREFIX := arm-none-eabi-
    $(info Using system ARM toolchain)
endif

# QEMU - Use workspace QEMU if available, otherwise system qemu
WORKSPACE_QEMU := $(HOME)/workspace/qemu/build/qemu-system-arm
ifneq (,$(wildcard $(WORKSPACE_QEMU)))
    QEMU := $(WORKSPACE_QEMU)
    $(info Using workspace QEMU: $(WORKSPACE_QEMU))
else
    QEMU := qemu-system-arm
    $(info Using system QEMU)
endif
QEMU_MACHINE := stm32f103c8-bluepill
QEMU_FLAGS := -M $(QEMU_MACHINE) -semihosting-config enable=on,target=native -monitor none -nographic

# Toolchain
CC = $(TOOLCHAIN_PREFIX)gcc
CXX = $(TOOLCHAIN_PREFIX)g++
LD = $(TOOLCHAIN_PREFIX)ld
OBJCOPY = $(TOOLCHAIN_PREFIX)objcopy
OBJDUMP = $(TOOLCHAIN_PREFIX)objdump
SIZE = $(TOOLCHAIN_PREFIX)size

# Test targets - Split into 6 ELFs for unoptimized debug builds
# Each target fits within 65KB flash limit with -O0 and full debug symbols
TARGETS := test1 test2 test3 test4 test5 test6

# Build directory
# Directories
SRC_DIR := ../../src
MOCK_DIR := ../mocks
UNITY_DIR := ../Unity/src
TEST_DIR := ..
BUILD_DIR := build

# Include paths
INCLUDES := \
	-I. \
	-I$(TEST_DIR) \
	-I$(SRC_DIR) \
	-I$(MOCK_DIR) \
	-I$(UNITY_DIR)
	
# Auto-discover pinCfgC sources (all .c files in src/)
PINCFG_SRCS := $(wildcard $(SRC_DIR)/*.c)

# Auto-discover pinCfgC C++ wrappers
PINCFG_CPP_SRCS := $(wildcard $(SRC_DIR)/*.cpp)

# Mock sources
MOCK_C_SRCS := $(wildcard $(MOCK_DIR)/*.c)
MOCK_CPP_SRCS := $(wildcard $(MOCK_DIR)/*.cpp)

# Unity source
UNITY_SRCS := $(UNITY_DIR)/unity.c

# System startup source
SYS_SRCS = startup.c

# Test sources - Define per-target test file combinations
# test1: Core tests only
TEST1_SRCS = \
	../test_main.c \
	../test_helpers.c \
	../test_core.c

# test2: Components + Parsing
TEST2_SRCS = \
	../test_main.c \
	../test_helpers.c \
	../test_components.c \
	../test_parsing.c

# test3: CLI tests
TEST3_SRCS = \
	../test_main.c \
	../test_helpers.c \
	../test_cli.c

# test4: Measurements (largest - 32+ tests)
TEST4_SRCS = \
	../test_main.c \
	../test_helpers.c \
	../test_measurements.c

# test5: Persistent config + Integration
TEST5_SRCS = \
	../test_main.c \
	../test_helpers.c \
	../test_persistent_config.c \
	../test_integration.c

# test6: Stress + Edge cases
TEST6_SRCS = \
	../test_main.c \
	../test_helpers.c \
	../test_stress.c \
	../test_edge_cases.c

# Common sources shared across all test targets
COMMON_C_SRCS := $(PINCFG_SRCS) $(MOCK_C_SRCS) $(UNITY_SRCS) $(SYS_SRCS)
COMMON_CPP_SRCS := $(PINCFG_CPP_SRCS) $(MOCK_CPP_SRCS)

# Common object files (compiled once, reused by all targets)
COMMON_C_OBJS := $(addprefix $(BUILD_DIR)/,$(notdir $(COMMON_C_SRCS:.c=.o)))
COMMON_CPP_OBJS := $(addprefix $(BUILD_DIR)/,$(notdir $(COMMON_CPP_SRCS:.cpp=.o)))
COMMON_OBJS := $(COMMON_C_OBJS) $(COMMON_CPP_OBJS)

# Per-target object files (test sources only)
TEST1_OBJS := $(addprefix $(BUILD_DIR)/test1/,$(notdir $(TEST1_SRCS:.c=.o)))
TEST2_OBJS := $(addprefix $(BUILD_DIR)/test2/,$(notdir $(TEST2_SRCS:.c=.o)))
TEST3_OBJS := $(addprefix $(BUILD_DIR)/test3/,$(notdir $(TEST3_SRCS:.c=.o)))
TEST4_OBJS := $(addprefix $(BUILD_DIR)/test4/,$(notdir $(TEST4_SRCS:.c=.o)))
TEST5_OBJS := $(addprefix $(BUILD_DIR)/test5/,$(notdir $(TEST5_SRCS:.c=.o)))
TEST6_OBJS := $(addprefix $(BUILD_DIR)/test6/,$(notdir $(TEST6_SRCS:.c=.o)))

# MCU settings
MCU_FLAGS = -mcpu=cortex-m3 -mthumb

# Common compiler flags - No optimization for better debugging
# Use -ffunction-sections and -fdata-sections for better dead code elimination
CFLAGS = $(MCU_FLAGS) -Og -g -Wall -Wextra \
	-ffunction-sections -fdata-sections \
	-DSTM32F103xB \
	-DUNIT_TEST \
	-DPINCFG_USE_ERROR_MESSAGES \
	-DPINCFG_FEATURE_I2C_MEASUREMENT \
	-DPINCFG_FEATURE_SPI_MEASUREMENT \
	-DPINCFG_FEATURE_ANALOG_MEASUREMENT \
	-DPINCFG_FEATURE_LOOPTIME_MEASUREMENT

CXXFLAGS = $(CFLAGS) \
	-fno-rtti -fno-exceptions -fno-threadsafe-statics

# Shared linker script for STM32F103C8T6
LDSCRIPT = linker.ld

# VPATH for finding sources
VPATH = . $(SRC_DIR) $(MOCK_DIR) $(UNITY_DIR) $(TEST_DIR)

# Linker flags (shared for all targets)
# --gc-sections: Remove unused sections (requires -ffunction-sections -fdata-sections)
# --print-gc-sections: Show what sections are being removed
# --specs=nano.specs: Use newlib-nano for smaller printf (integer-only, no float support)
LDFLAGS = $(MCU_FLAGS) \
	-T$(LDSCRIPT) \
	-lc -lgcc -lm \
	--specs=nano.specs \
	--specs=rdimon.specs \
	-Wl,--gc-sections #\
 	-Wl,--print-gc-sections

# Build targets
all: $(TARGETS)

# Individual target rules
test1: $(BUILD_DIR)/test1.elf $(BUILD_DIR)/test1.bin $(BUILD_DIR)/test1.hex
test2: $(BUILD_DIR)/test2.elf $(BUILD_DIR)/test2.bin $(BUILD_DIR)/test2.hex
test3: $(BUILD_DIR)/test3.elf $(BUILD_DIR)/test3.bin $(BUILD_DIR)/test3.hex
test4: $(BUILD_DIR)/test4.elf $(BUILD_DIR)/test4.bin $(BUILD_DIR)/test4.hex
test5: $(BUILD_DIR)/test5.elf $(BUILD_DIR)/test5.bin $(BUILD_DIR)/test5.hex
test6: $(BUILD_DIR)/test6.elf $(BUILD_DIR)/test6.bin $(BUILD_DIR)/test6.hex

# Create build directories
$(BUILD_DIR):
	@mkdir -p $(BUILD_DIR)

$(BUILD_DIR)/test1:
	@mkdir -p $(BUILD_DIR)/test1

$(BUILD_DIR)/test2:
	@mkdir -p $(BUILD_DIR)/test2

$(BUILD_DIR)/test3:
	@mkdir -p $(BUILD_DIR)/test3

$(BUILD_DIR)/test4:
	@mkdir -p $(BUILD_DIR)/test4

$(BUILD_DIR)/test5:
	@mkdir -p $(BUILD_DIR)/test5

$(BUILD_DIR)/test6:
	@mkdir -p $(BUILD_DIR)/test6

$(COMMON_OBJS): | $(BUILD_DIR)
$(TEST1_OBJS): | $(BUILD_DIR)/test1
$(TEST2_OBJS): | $(BUILD_DIR)/test2
$(TEST3_OBJS): | $(BUILD_DIR)/test3
$(TEST4_OBJS): | $(BUILD_DIR)/test4
$(TEST5_OBJS): | $(BUILD_DIR)/test5
$(TEST6_OBJS): | $(BUILD_DIR)/test6

# Compile C files
$(BUILD_DIR)/%.o: %.c
	@mkdir -p $(BUILD_DIR)
	@echo "CC $<"
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

# Compile C++ files
$(BUILD_DIR)/%.o: %.cpp
	@mkdir -p $(BUILD_DIR)
	@echo "CXX $<"
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c $< -o $@

# Compile test files for test1 (core tests only)
$(BUILD_DIR)/test1/%.o: %.c | $(BUILD_DIR)/test1
	@echo "CC [test1] $<"
	$(CC) $(CFLAGS) $(INCLUDES) \
		-DRUN_CORE_TESTS=1 \
		-DRUN_COMPONENTS_TESTS=0 \
		-DRUN_PARSING_TESTS=0 \
		-DRUN_CLI_TESTS=0 \
		-DRUN_MEASUREMENTS_TESTS=0 \
		-DRUN_EDGE_CASE_TESTS=0 \
		-DRUN_INTEGRATION_TESTS=0 \
		-DRUN_PERSISTENT_CFG_TESTS=0 \
		-DRUN_STRESS_TESTS=0 \
		-c $< -o $@

# Compile test files for test2 (components + parsing)
$(BUILD_DIR)/test2/%.o: %.c | $(BUILD_DIR)/test2
	@echo "CC [test2] $<"
	$(CC) $(CFLAGS) $(INCLUDES) \
		-DRUN_CORE_TESTS=0 \
		-DRUN_COMPONENTS_TESTS=1 \
		-DRUN_PARSING_TESTS=1 \
		-DRUN_CLI_TESTS=0 \
		-DRUN_MEASUREMENTS_TESTS=0 \
		-DRUN_EDGE_CASE_TESTS=0 \
		-DRUN_INTEGRATION_TESTS=0 \
		-DRUN_PERSISTENT_CFG_TESTS=0 \
		-DRUN_STRESS_TESTS=0 \
		-c $< -o $@

# Compile test files for test3 (CLI tests only)
$(BUILD_DIR)/test3/%.o: %.c | $(BUILD_DIR)/test3
	@echo "CC [test3] $<"
	$(CC) $(CFLAGS) $(INCLUDES) \
		-DRUN_CORE_TESTS=0 \
		-DRUN_COMPONENTS_TESTS=0 \
		-DRUN_PARSING_TESTS=0 \
		-DRUN_CLI_TESTS=1 \
		-DRUN_MEASUREMENTS_TESTS=0 \
		-DRUN_EDGE_CASE_TESTS=0 \
		-DRUN_INTEGRATION_TESTS=0 \
		-DRUN_PERSISTENT_CFG_TESTS=0 \
		-DRUN_STRESS_TESTS=0 \
		-c $< -o $@

# Compile test files for test4 (measurements only)
$(BUILD_DIR)/test4/%.o: %.c | $(BUILD_DIR)/test4
	@echo "CC [test4] $<"
	$(CC) $(CFLAGS) $(INCLUDES) \
		-DRUN_CORE_TESTS=0 \
		-DRUN_COMPONENTS_TESTS=0 \
		-DRUN_PARSING_TESTS=0 \
		-DRUN_CLI_TESTS=0 \
		-DRUN_MEASUREMENTS_TESTS=1 \
		-DRUN_EDGE_CASE_TESTS=0 \
		-DRUN_INTEGRATION_TESTS=0 \
		-DRUN_PERSISTENT_CFG_TESTS=0 \
		-DRUN_STRESS_TESTS=0 \
		-c $< -o $@

# Compile test files for test5 (persistent config + integration)
$(BUILD_DIR)/test5/%.o: %.c | $(BUILD_DIR)/test5
	@echo "CC [test5] $<"
	$(CC) $(CFLAGS) $(INCLUDES) \
		-DRUN_CORE_TESTS=0 \
		-DRUN_COMPONENTS_TESTS=0 \
		-DRUN_PARSING_TESTS=0 \
		-DRUN_CLI_TESTS=0 \
		-DRUN_MEASUREMENTS_TESTS=0 \
		-DRUN_EDGE_CASE_TESTS=0 \
		-DRUN_INTEGRATION_TESTS=1 \
		-DRUN_PERSISTENT_CFG_TESTS=1 \
		-DRUN_STRESS_TESTS=0 \
		-c $< -o $@

# Compile test files for test6 (stress + edge cases)
$(BUILD_DIR)/test6/%.o: %.c | $(BUILD_DIR)/test6
	@echo "CC [test6] $<"
	$(CC) $(CFLAGS) $(INCLUDES) \
		-DRUN_CORE_TESTS=0 \
		-DRUN_COMPONENTS_TESTS=0 \
		-DRUN_PARSING_TESTS=0 \
		-DRUN_CLI_TESTS=0 \
		-DRUN_MEASUREMENTS_TESTS=0 \
		-DRUN_EDGE_CASE_TESTS=1 \
		-DRUN_INTEGRATION_TESTS=0 \
		-DRUN_PERSISTENT_CFG_TESTS=0 \
		-DRUN_STRESS_TESTS=1 \
		-c $< -o $@

# Link test1.elf
$(BUILD_DIR)/test1.elf: $(COMMON_OBJS) $(TEST1_OBJS) $(LDSCRIPT)
	@echo "Linking $@..."
	$(CXX) $(COMMON_OBJS) $(TEST1_OBJS) $(LDFLAGS) -Wl,-Map=$(BUILD_DIR)/test1.map -o $@
	@echo "Size information:"
	@$(SIZE) $@

# Link test2.elf
$(BUILD_DIR)/test2.elf: $(COMMON_OBJS) $(TEST2_OBJS) $(LDSCRIPT)
	@echo "Linking $@..."
	$(CXX) $(COMMON_OBJS) $(TEST2_OBJS) $(LDFLAGS) -Wl,-Map=$(BUILD_DIR)/test2.map -o $@
	@echo "Size information:"
	@$(SIZE) $@

# Link test3.elf
$(BUILD_DIR)/test3.elf: $(COMMON_OBJS) $(TEST3_OBJS) $(LDSCRIPT)
	@echo "Linking $@..."
	$(CXX) $(COMMON_OBJS) $(TEST3_OBJS) $(LDFLAGS) -Wl,-Map=$(BUILD_DIR)/test3.map -o $@
	@echo "Size information:"
	@$(SIZE) $@

# Link test4.elf
$(BUILD_DIR)/test4.elf: $(COMMON_OBJS) $(TEST4_OBJS) $(LDSCRIPT)
	@echo "Linking $@..."
	$(CXX) $(COMMON_OBJS) $(TEST4_OBJS) $(LDFLAGS) -Wl,-Map=$(BUILD_DIR)/test4.map -o $@
	@echo "Size information:"
	@$(SIZE) $@

# Link test5.elf
$(BUILD_DIR)/test5.elf: $(COMMON_OBJS) $(TEST5_OBJS) $(LDSCRIPT)
	@echo "Linking $@..."
	$(CXX) $(COMMON_OBJS) $(TEST5_OBJS) $(LDFLAGS) -Wl,-Map=$(BUILD_DIR)/test5.map -o $@
	@echo "Size information:"
	@$(SIZE) $@

# Link test6.elf
$(BUILD_DIR)/test6.elf: $(COMMON_OBJS) $(TEST6_OBJS) $(LDSCRIPT)
	@echo "Linking $@..."
	$(CXX) $(COMMON_OBJS) $(TEST6_OBJS) $(LDFLAGS) -Wl,-Map=$(BUILD_DIR)/test6.map -o $@
	@echo "Size information:"
	@$(SIZE) $@

# Generate binary files
$(BUILD_DIR)/%.bin: $(BUILD_DIR)/%.elf
	@echo "Creating binary $@..."
	$(OBJCOPY) -O binary $< $@

# Generate hex files
$(BUILD_DIR)/%.hex: $(BUILD_DIR)/%.elf
	@echo "Creating hex $@..."
	$(OBJCOPY) -O ihex $< $@

# Generate listing files
$(BUILD_DIR)/%.lst: $(BUILD_DIR)/%.elf
	@echo "Creating listing $@..."
	$(OBJDUMP) -d -S $< > $@

# Disassembly target - generates .lst files for all targets
disasm: $(addprefix $(BUILD_DIR)/,$(addsuffix .lst,$(TARGETS)))
	@echo "Disassembly files generated"

# QEMU run targets
run-test1: $(BUILD_DIR)/test1.elf
	@echo "Running test1.elf in QEMU..."
	$(QEMU) $(QEMU_FLAGS) -kernel $<

run-test2: $(BUILD_DIR)/test2.elf
	@echo "Running test2.elf in QEMU..."
	$(QEMU) $(QEMU_FLAGS) -kernel $<

run-test3: $(BUILD_DIR)/test3.elf
	@echo "Running test3.elf in QEMU..."
	$(QEMU) $(QEMU_FLAGS) -kernel $<

run-test4: $(BUILD_DIR)/test4.elf
	@echo "Running test4.elf in QEMU..."
	$(QEMU) $(QEMU_FLAGS) -kernel $<

run-test5: $(BUILD_DIR)/test5.elf
	@echo "Running test5.elf in QEMU..."
	$(QEMU) $(QEMU_FLAGS) -kernel $<

run-test6: $(BUILD_DIR)/test6.elf
	@echo "Running test6.elf in QEMU..."
	$(QEMU) $(QEMU_FLAGS) -kernel $<

# Run all tests sequentially
run-all: $(TARGETS)
	@echo "========================================"
	@echo "Running all test targets in QEMU..."
	@echo "========================================"
	@for target in $(TARGETS); do \
		echo "" ; \
		echo "=== Running $$target.elf ===" ; \
		$(QEMU) $(QEMU_FLAGS) -kernel $(BUILD_DIR)/$$target.elf ; \
		echo "" ; \
	done
	@echo "========================================"
	@echo "All tests completed"
	@echo "========================================"

# Clean
clean:
	rm -rf $(BUILD_DIR)

# Help target
help:
	@echo "Available targets:"
	@echo "  all       - Build all test ELF files (currently: $(TARGETS))"
	@echo "  test1     - Build test1.elf (core tests)"
	@echo "  test2     - Build test2.elf (components + parsing)"
	@echo "  test3     - Build test3.elf (CLI tests)"
	@echo "  test4     - Build test4.elf (measurements)"
	@echo "  test5     - Build test5.elf (persistent config + integration)"
	@echo "  test6     - Build test6.elf (stress + edge cases)"
	@echo "  run-test1 - Run test1.elf in QEMU"
	@echo "  run-test2 - Run test2.elf in QEMU"
	@echo "  run-test3 - Run test3.elf in QEMU"
	@echo "  run-test4 - Run test4.elf in QEMU"
	@echo "  run-test5 - Run test5.elf in QEMU"
	@echo "  run-test6 - Run test6.elf in QEMU"
	@echo "  run-all   - Run all test targets sequentially"
	@echo "  disasm    - Generate disassembly listings"
	@echo "  clean     - Remove all build artifacts"
	@echo ""
	@echo "Build configuration:"
	@echo "  Optimization: -Og (optimized for debugging)"
	@echo "  Debug symbols: -g (debug information)"
	@echo "  Printf: Integer-only (--specs=nano.specs, no float support)"
	@echo "  Flash limit: 65KB per ELF"
	@echo ""
	@echo "Environment variables:"
	@echo "  QEMU      - Path to qemu-system-arm (default: auto-detect)"

.PHONY: all test1 test2 test3 test4 test5 test6 disasm help clean run-test1 run-test2 run-test3 run-test4 run-test5 run-test6 run-all
