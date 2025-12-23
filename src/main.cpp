// ============================================================================
// main.cpp - Chess Engine Entry Point
// ============================================================================
// This is the main entry point for the chess engine.
// It initializes all subsystems and runs the UCI loop.
// ============================================================================

#include <iostream>
#include "board.hpp"
#include "bitboard.hpp"
#include "magic.hpp"
#include "zobrist.hpp"
#include "tt.hpp"
#include "uci.hpp"

// ============================================================================
// Engine Initialization
// ============================================================================

void init_engine() {
    // 1. Initialize basic bitboard tables (Pawn, Knight, King attacks)
    Bitboards::init();

    // 2. Initialize Magic Bitboards (Bishop, Rook, Queen attacks)
    // Without this, slider piece move generation will crash
    Magics::init();

    // 3. Initialize Zobrist Hashing
    // Without this, hash keys will be 0 and TT will malfunction
    Zobrist::init();

    // 4. Initialize Position (if needed for additional setup)
    Position::init();
}

// ============================================================================
// Main Entry Point
// ============================================================================

int main([[maybe_unused]] int argc, [[maybe_unused]] char* argv[]) {
    // Ensure output is unbuffered for UCI communication
    std::cout.setf(std::ios::unitbuf);

    // Initialize all engine subsystems
    init_engine();

    // Run UCI loop (default mode)
    UCI::UCIHandler uci;
    uci.loop();

    return 0;
}
