#ifndef SEARCH_CONSTANTS_HPP
#define SEARCH_CONSTANTS_HPP

// ============================================================================
// Search Constants and Parameters
// ============================================================================
// This file centralizes all search-related constants to make tuning easier
// and reduce magic numbers scattered throughout the codebase.
// ============================================================================

namespace SearchParams {

// ============================================================================
// Time Management Constants
// ============================================================================

constexpr int TIME_CHECK_INTERVAL = 2048;     // Check time every N nodes (must be power of 2 - 1)
constexpr double EARLY_STOP_RATIO = 0.6;      // Stop early if elapsed > optimum * ratio
constexpr double PANIC_TIME_EXTENSION = 1.5;  // Extend time by this factor in panic mode
constexpr int SCORE_FLUCTUATION_THRESHOLD = 50;  // Centipawns change to trigger panic

// ============================================================================
// Pruning Parameters
// ============================================================================

// Futility pruning margins per depth
constexpr int FutilityMargin[7] = { 0, 150, 300, 450, 600, 750, 900 };

// Razoring margins per depth
constexpr int RazorMargin[4] = { 0, 300, 500, 700 };

// Reverse futility pruning (static null move) margins
constexpr int RFPMargin[7] = { 0, 80, 160, 240, 320, 400, 480 };

// Late move pruning thresholds (skip quiet moves after this many tries at low depth)
constexpr int LMPThreshold[8] = { 0, 8, 12, 18, 25, 33, 42, 52 };

// Maximum depth for specific pruning techniques
constexpr int FUTILITY_MAX_DEPTH = 6;
constexpr int RAZORING_MAX_DEPTH = 3;
constexpr int RFP_MAX_DEPTH = 6;
constexpr int LMP_MAX_DEPTH = 7;
constexpr int SEE_QUIET_MAX_DEPTH = 3;
constexpr int SEE_CAPTURE_MAX_DEPTH = 4;

// SEE thresholds
constexpr int SEE_CAPTURE_THRESHOLD_PER_DEPTH = -50;
constexpr int SEE_QUIET_THRESHOLD_PER_DEPTH = -100;

// History Leaf Pruning threshold (prune moves with very negative history)
constexpr int HISTORY_LEAF_PRUNING_DEPTH = 4;    // Max depth for history pruning
constexpr int HISTORY_LEAF_PRUNING_MARGIN = 8000; // Base margin per depth

// ============================================================================
// Extension Parameters
// ============================================================================

constexpr int MAX_EXTENSIONS = 16;           // Maximum total extensions in path

// Singular extension parameters
constexpr int SINGULAR_DEPTH = 6;            // Minimum depth for singular extension
constexpr int SINGULAR_MARGIN = 64;          // Score margin for singularity

// ============================================================================
// Multi-Cut Parameters
// ============================================================================

constexpr int MULTI_CUT_DEPTH = 5;           // Minimum depth for multi-cut
constexpr int MULTI_CUT_COUNT = 3;           // Number of moves to try
constexpr int MULTI_CUT_REQUIRED = 2;        // Number of cutoffs required

// ============================================================================
// ProbCut Parameters
// ============================================================================

constexpr int PROBCUT_DEPTH = 5;             // Minimum depth for ProbCut
constexpr int PROBCUT_MARGIN = 100;          // Score margin above beta

// ============================================================================
// Null Move Parameters
// ============================================================================

constexpr int NULL_MOVE_MIN_DEPTH = 3;       // Minimum depth for null move pruning
constexpr int NULL_MOVE_VERIFY_DEPTH = 12;   // Depth to start verification search

// ============================================================================
// Aspiration Window Parameters
// ============================================================================

constexpr int ASPIRATION_INITIAL_DELTA = 50; // Initial aspiration window size
constexpr int ASPIRATION_MIN_DEPTH = 5;      // Minimum depth to use aspiration windows
constexpr int ASPIRATION_MAX_FAILS = 3;      // Max fails before using full window

// ============================================================================
// IIR (Internal Iterative Reductions) Parameters
// Replaces IID - more efficient depth reduction instead of shallow search
// ============================================================================

constexpr int IIR_MIN_DEPTH = 4;             // Minimum depth for IIR
constexpr int IIR_REDUCTION = 1;             // Depth reduction when no TT move
constexpr int IIR_PV_REDUCTION = 1;          // Reduction in PV nodes
constexpr int IIR_CUT_REDUCTION = 2;         // Reduction in cut nodes (more aggressive)

// ============================================================================
// Quiescence Search Parameters
// ============================================================================

constexpr int QSEARCH_CHECK_DEPTH = 2;       // Depth to search quiet checks in qsearch
constexpr int DELTA_PRUNING_MARGIN = 200;    // Delta pruning margin

} // namespace SearchParams

#endif // SEARCH_CONSTANTS_HPP
