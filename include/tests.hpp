#ifndef TESTS_HPP
#define TESTS_HPP

// ============================================================================
// Test Suite Header
// ============================================================================
// Contains declarations for all test functions.
// Implementation is in tests.cpp (excluded from normal builds).
// ============================================================================

namespace Tests {

// Stage 1 tests (basic functionality)
void test_bitboards();
void test_magic_bitboards();

// Stage 2 tests (core engine components)
void test_move_generation();
void test_transposition_table();
void test_see();
void test_move_ordering();
void test_evaluation();
void test_perft();
void test_search();

// Test runners
void run_all_tests();
void run_benchmark();
void run_perft(int maxDepth = 5);

// Print help
void print_help();

} // namespace Tests

#endif // TESTS_HPP
