#ifndef TESTING_HPP
#define TESTING_HPP

#include "types.hpp"
#include "board.hpp"
#include "movegen.hpp"
#include "search.hpp"
#include <string>
#include <vector>
#include <functional>
#include <chrono>
#include <fstream>
#include <sstream>
#include <iomanip>

// ============================================================================
// Test Result Structure
// ============================================================================

struct TestResult {
    std::string name;
    bool passed;
    std::string message;
    int64_t timeMs;
    U64 nodes;

    TestResult() : name(""), passed(false), message(""), timeMs(0), nodes(0) {}
    TestResult(const std::string& nm, bool p, const std::string& msg = "",
               int64_t t = 0, U64 nd = 0)
        : name(nm), passed(p), message(msg), timeMs(t), nodes(nd) {}
};

// ============================================================================
// Perft Position Structure
// ============================================================================

struct PerftPosition {
    std::string fen;
    std::string description;
    std::vector<U64> expectedCounts;  // Expected node counts for each depth

    PerftPosition(const std::string& f, const std::string& d, std::vector<U64> counts)
        : fen(f), description(d), expectedCounts(counts) {}
};

// ============================================================================
// Tactical Test Position (EPD format)
// ============================================================================

struct TacticalPosition {
    std::string fen;
    std::string id;
    std::vector<std::string> bestMoves;     // Expected best move(s) - bm tag
    std::vector<std::string> avoidMoves;    // Moves to avoid - am tag
    std::string description;
    int difficulty;  // 1-5 scale

    TacticalPosition() : difficulty(1) {}
    TacticalPosition(const std::string& f, const std::string& i,
                     const std::vector<std::string>& bm = {},
                     const std::string& desc = "", int diff = 1)
        : fen(f), id(i), bestMoves(bm), description(desc), difficulty(diff) {}
};

// ============================================================================
// Perft Testing Suite
// ============================================================================

namespace PerftTest {

// Standard perft function
U64 perft(Board& board, int depth);

// Perft with divide (shows move-by-move breakdown)
U64 perft_divide(Board& board, int depth);

// Run perft on a position and compare with expected
TestResult run_position(const PerftPosition& pos, int maxDepth);

// Run all standard perft positions
std::vector<TestResult> run_suite(int maxDepth = 5);

// Get all standard test positions
std::vector<PerftPosition> get_standard_positions();

// Debug: find discrepancy by comparing with another engine's perft
void compare_perft(Board& board, int depth, const std::string& compareFile);

}  // namespace PerftTest

// ============================================================================
// Tactical Test Suite
// ============================================================================

namespace TacticalTest {

// Run a single tactical position
TestResult run_position(const TacticalPosition& pos, int depth, int64_t timeMs = 10000);

// Run a complete test suite
std::vector<TestResult> run_suite(const std::vector<TacticalPosition>& positions,
                                   int depth, int64_t timeMs = 10000);

// Load EPD file
std::vector<TacticalPosition> load_epd(const std::string& filename);

// Parse EPD line
TacticalPosition parse_epd_line(const std::string& line);

// Built-in test suites
std::vector<TacticalPosition> get_wac_positions();           // Win At Chess
std::vector<TacticalPosition> get_bratko_kopec_positions();  // Bratko-Kopec
std::vector<TacticalPosition> get_sts_positions();           // Strategic Test Suite (sample)
std::vector<TacticalPosition> get_mate_positions();          // Checkmate puzzles

// Print summary
void print_summary(const std::vector<TestResult>& results);

}  // namespace TacticalTest

// ============================================================================
// Benchmark Testing
// ============================================================================

namespace BenchTest {

struct BenchmarkResult {
    std::string name;
    U64 totalNodes;
    int64_t totalTimeMs;
    double nps;  // Nodes per second
    int positionsRun;

    BenchmarkResult() : totalNodes(0), totalTimeMs(0), nps(0), positionsRun(0) {}
};

// Run standard benchmark (depth-based)
BenchmarkResult run_depth_benchmark(int depth = 12);

// Run time-based benchmark
BenchmarkResult run_time_benchmark(int64_t msPerPosition = 1000);

// Run node-based benchmark
BenchmarkResult run_node_benchmark(U64 nodesPerPosition = 1000000);

// Get benchmark positions
std::vector<std::string> get_benchmark_positions();

// Compare with previous benchmark (returns % difference in NPS)
double compare_benchmark(const BenchmarkResult& current, const BenchmarkResult& baseline);

// Save benchmark result to file
void save_result(const BenchmarkResult& result, const std::string& filename);

// Load benchmark result from file
BenchmarkResult load_result(const std::string& filename);

}  // namespace BenchTest

// ============================================================================
// Regression Testing (SPRT / Gauntlet)
// ============================================================================

namespace RegressionTest {

struct MatchConfig {
    std::string engine1Path;
    std::string engine2Path;
    std::string engine1Name;
    std::string engine2Name;
    int games;
    int64_t timeControlMs;
    int64_t incrementMs;
    int threads;
    int hashMB;
    std::string bookPath;
    std::string outputPgn;
    bool adjudicateDraw;
    bool adjudicateMate;
    int drawMoveLimit;
    int drawScoreLimit;

    MatchConfig() : games(100), timeControlMs(10000), incrementMs(100),
                    threads(1), hashMB(64), adjudicateDraw(true),
                    adjudicateMate(true), drawMoveLimit(50), drawScoreLimit(10) {}
};

struct MatchResult {
    int wins1;      // Engine 1 wins
    int wins2;      // Engine 2 wins
    int draws;
    double eloEstimate;
    double eloDelta;  // Error margin
    bool sprtPassed;
    std::string status;

    MatchResult() : wins1(0), wins2(0), draws(0), eloEstimate(0),
                    eloDelta(0), sprtPassed(false) {}
};

// Generate cutechess-cli command
std::string generate_cutechess_command(const MatchConfig& config);

// Generate fastchess command
std::string generate_fastchess_command(const MatchConfig& config);

// Parse cutechess-cli output
MatchResult parse_cutechess_output(const std::string& output);

// Calculate SPRT statistics
double calculate_elo_from_score(double score, int games);
bool check_sprt(int wins, int losses, int draws, double elo0, double elo1,
                double alpha, double beta);

// Create match configuration for SPRT testing
MatchConfig create_sprt_config(const std::string& engine1, const std::string& engine2,
                                double elo0 = 0, double elo1 = 5);

// Create configuration for gauntlet testing
MatchConfig create_gauntlet_config(const std::string& testedEngine,
                                    const std::vector<std::string>& opponents);

// Generate shell script for running tests
void generate_test_script(const MatchConfig& config, const std::string& filename);

}  // namespace RegressionTest

// ============================================================================
// Test Suite Runner
// ============================================================================

namespace TestRunner {

// Run all test categories
void run_all_tests();

// Run specific test category
void run_perft_tests(int maxDepth = 5);
void run_tactical_tests(int depth = 12, int64_t timeMs = 10000);
void run_benchmark(int depth = 12);

// Generate test report
void generate_report(const std::string& filename);

// Print test summary
void print_summary();

}  // namespace TestRunner

#endif // TESTING_HPP
