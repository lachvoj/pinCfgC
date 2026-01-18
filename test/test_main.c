#include "test_helpers.h"

// ============================================================================
// Test Module Registration Functions
// ============================================================================

// Forward declarations of test registration functions
void register_core_tests(void);
void register_components_tests(void);
void register_parsing_tests(void);
void register_cli_tests(void);
void register_measurements_tests(void);
void register_edge_cases_tests(void);
void register_integration_tests(void);
void register_persistent_config_tests(void);
void register_stress_tests(void);

// ============================================================================
// Test Selection Macros
// ============================================================================

// Define which test modules to run (default: all enabled)
#ifndef RUN_CORE_TESTS
#define RUN_CORE_TESTS 1
#endif

#ifndef RUN_COMPONENTS_TESTS
#define RUN_COMPONENTS_TESTS 1
#endif

#ifndef RUN_PARSING_TESTS
#define RUN_PARSING_TESTS 1
#endif

#ifndef RUN_CLI_TESTS
#define RUN_CLI_TESTS 1
#endif

#ifndef RUN_MEASUREMENTS_TESTS
#define RUN_MEASUREMENTS_TESTS 1
#endif

#ifndef RUN_EDGE_CASE_TESTS
#define RUN_EDGE_CASE_TESTS 1
#endif

#ifndef RUN_INTEGRATION_TESTS
#define RUN_INTEGRATION_TESTS 1
#endif

#ifndef RUN_PERSISTENT_CFG_TESTS
#define RUN_PERSISTENT_CFG_TESTS 1
#endif

#ifndef RUN_STRESS_TESTS
#define RUN_STRESS_TESTS 1
#endif

// ============================================================================
// Main Test Entry Point
// ============================================================================

#ifdef UNIT_TEST
int main(void)
#else
int test_main(void)
#endif
{
    // Print struct sizes for debugging and memory optimization
    printf("=== Struct Sizes ===\n");
    printf("sizeof(GLOBALS_T): %ld\n", sizeof(GLOBALS_T));
    printf("sizeof(PRESENTABLE_T): %ld\n", sizeof(PRESENTABLE_T));
    printf("sizeof(CLI_T): %ld\n", sizeof(CLI_T));
    printf("sizeof(SWITCH_T): %ld\n", sizeof(SWITCH_T));
    printf("sizeof(INPIN_T): %ld\n", sizeof(INPIN_T));
    printf("sizeof(TRIGGER_T): %ld\n", sizeof(TRIGGER_T));
    printf("sizeof(TRIGGER_SWITCHACTION_T): %ld\n", sizeof(TRIGGER_SWITCHACTION_T));
    printf("sizeof(SENSOR_T): %ld\n", sizeof(SENSOR_T));
    printf("sizeof(CPUTEMPMEASURE_T): %ld\n", sizeof(CPUTEMPMEASURE_T));
#ifdef PINCFG_FEATURE_LOOPTIME_MEASUREMENT
    printf("sizeof(LOOPTIMEMEASURE_T): %ld\n", sizeof(LOOPTIMEMEASURE_T));
#endif
    printf("\n");

    UNITY_BEGIN();

    // Register and run test modules based on compile-time flags
#if RUN_CORE_TESTS
    printf("=== Running Core Tests ===\n");
    register_core_tests();
#endif

#if RUN_COMPONENTS_TESTS
    printf("=== Running Component Tests ===\n");
    register_components_tests();
#endif

#if RUN_PARSING_TESTS
    printf("=== Running Parsing Tests ===\n");
    register_parsing_tests();
#endif

#if RUN_CLI_TESTS
    printf("=== Running CLI Tests ===\n");
    register_cli_tests();
#endif

#if RUN_MEASUREMENTS_TESTS
    printf("=== Running Measurement Tests ===\n");
    register_measurements_tests();
#endif

#if RUN_EDGE_CASE_TESTS
    printf("=== Running Edge Case Tests ===\n");
    register_edge_cases_tests();
#endif

#if RUN_INTEGRATION_TESTS
    printf("=== Running Integration Tests ===\n");
    register_integration_tests();
#endif

#if RUN_PERSISTENT_CFG_TESTS
    printf("=== Running Persistent Config Tests ===\n");
    register_persistent_config_tests();
#endif

#if RUN_STRESS_TESTS
    printf("=== Running Stress Tests ===\n");
    register_stress_tests();
#endif

    return UNITY_END();
}
