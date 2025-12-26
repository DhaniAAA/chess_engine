#ifndef THREAD_HPP
#define THREAD_HPP

#include "search.hpp"  // For SearchLimits, MAX_PLY, MAX_MOVES, score functions
#include "board.hpp"
#include "move.hpp"
#include "moveorder.hpp"
#include "tt.hpp"
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <memory>
#include <mutex>
#include <thread>
#include <vector>

// Forward declaration
class ThreadPool;

// ============================================================================
// Thread Constants
// ============================================================================

constexpr int MAX_THREADS = 256;


// ============================================================================
// Per-Thread Search Stack
// ============================================================================

struct ThreadStack {
    int ply;
    Move currentMove;
    Move excludedMove;
    Move killers[2];
    int staticEval;
    int moveCount;
    int extensions;
    bool inCheck;
    bool ttPv;
    bool ttHit;
    bool nullMovePruned;
};

// ============================================================================
// PV Line for each thread
// ============================================================================

struct ThreadPVLine {
    int length = 0;
    Move moves[MAX_PLY];

    void clear() { length = 0; }

    void update(Move m, const ThreadPVLine& child) {
        moves[0] = m;
        for (int i = 0; i < child.length && i + 1 < MAX_PLY; ++i) {
            moves[i + 1] = child.moves[i];
        }
        length = child.length + 1;
    }

    std::string to_string() const {
        std::string s;
        for (int i = 0; i < length; ++i) {
            if (i > 0) s += " ";
            s += move_to_string(moves[i]);
        }
        return s;
    }

    Move first() const {
        return length > 0 ? moves[0] : MOVE_NONE;
    }

    Move second() const {
        return length > 1 ? moves[1] : MOVE_NONE;
    }
};

// ============================================================================
// Thread Class - Each thread runs its own search
// ============================================================================

class SearchThread {
public:
    explicit SearchThread(int id);
    ~SearchThread();

    // Thread control
    void start_searching();
    void wait_for_search_finished();

    // Thread identification
    int id() const { return threadId; }
    bool is_main() const { return threadId == 0; }

    // Search state
    std::atomic<bool> searching{false};
    std::atomic<bool> exit{false};

    // Per-thread data
    Board* rootBoard = nullptr;
    int rootDepth = 0;
    int completedDepth = 0;
    Move bestMove = MOVE_NONE;
    Move ponderMove = MOVE_NONE;
    int bestScore = 0;

    // Per-thread statistics
    U64 nodes = 0;
    U64 tbHits = 0;
    int selDepth = 0;

    // Per-thread history tables
    KillerTable killers;
    CounterMoveTable counterMoves;
    HistoryTable history;

    // Per-thread search stack and PV
    ThreadStack stack[MAX_PLY + 4];
    ThreadPVLine pvLines[MAX_PLY];

    // Previous move for counter move heuristic
    Move previousMove = MOVE_NONE;

    // Clear history (between games)
    void clear_history();

    // Thread-local random for diversity
    U64 rand_seed;
    int rand_int(int max);

private:
    int threadId;
    std::thread nativeThread;
    std::mutex mutex;
    std::condition_variable cv;

    void idle_loop();
};

// ============================================================================
// Thread Pool - Manages all search threads
// ============================================================================

class ThreadPool {
public:
    ThreadPool();
    ~ThreadPool();

    // Thread management
    void set_thread_count(int count);
    int thread_count() const { return static_cast<int>(threads.size()); }

    // Get main thread
    SearchThread* main() { return threads.empty() ? nullptr : threads[0].get(); }
    const SearchThread* main() const { return threads.empty() ? nullptr : threads[0].get(); }

    // Start/stop searching on all threads
    void start_thinking(Board& board, const SearchLimits& limits);
    void stop();
    void on_ponderhit();  // Transition from ponder to normal search
    void wait_for_search_finished();

    // Check if any thread is still searching
    bool searching() const;

    // Aggregate statistics from all threads
    U64 total_nodes() const;
    U64 total_tb_hits() const;
    int max_sel_depth() const;

    // Get best move from main thread or best thread
    Move best_move() const;
    Move ponder_move() const;
    int best_score() const;

    // Global stop flag
    std::atomic<bool> stop_flag{false};

    // Search limits (shared by all threads)
    SearchLimits limits;

    // Time management
    std::chrono::steady_clock::time_point startTime;
    int optimumTime = 0;
    int maximumTime = 0;

    // Clear all history tables (new game)
    void clear_all_history();

private:
    std::vector<std::unique_ptr<SearchThread>> threads;

    void init_time_management(Color us);
};

// ============================================================================
// Global Thread Pool Instance
// ============================================================================

extern ThreadPool Threads;

// ============================================================================
// Lazy SMP Search Functions (implemented in thread.cpp)
// ============================================================================

namespace LazySMP {

// Main entry point for parallel search
void search(SearchThread* thread, Board& board, int depth);

// Iterative deepening for a single thread
void iterative_deepening(SearchThread* thread, Board& board);

// Alpha-beta search
int alpha_beta(SearchThread* thread, Board& board, int alpha, int beta,
               int depth, bool cutNode, int ply);

// Quiescence search
int qsearch(SearchThread* thread, Board& board, int alpha, int beta, int ply);

// Static evaluation
int evaluate(const Board& board);

// Time check
bool should_stop(SearchThread* thread);

// UCI output (only from main thread)
void report_info(SearchThread* thread, int depth, int score);

}  // namespace LazySMP

#endif // THREAD_HPP
