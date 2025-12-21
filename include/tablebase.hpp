#ifndef TABLEBASE_HPP
#define TABLEBASE_HPP

#include "board.hpp"
#include "move.hpp"
#include "tt.hpp"
#include <string>

// ============================================================================
// Syzygy Endgame Tablebase Support (TAHAP 5)
//
// Syzygy tablebases provide perfect play in endgames with few pieces.
// Two types of probes:
//   - WDL (Win/Draw/Loss): Quick probe for search pruning
//   - DTZ (Distance to Zero): For root move selection
//
// This is an interface - actual tablebase files and Fathom library
// integration would be needed for full functionality.
// ============================================================================

namespace Tablebase {

// ============================================================================
// WDL (Win/Draw/Loss) Result
// ============================================================================

enum WDLScore {
    WDL_LOSS         = -2,  // Losing regardless of 50-move rule
    WDL_BLESSED_LOSS = -1,  // Loss saved by 50-move rule
    WDL_DRAW         =  0,  // Draw
    WDL_CURSED_WIN   =  1,  // Win negated by 50-move rule
    WDL_WIN          =  2,  // Winning regardless of 50-move rule
    WDL_NONE         = -3   // No tablebase probe available
};

// ============================================================================
// DTZ (Distance to Zero) Result
// ============================================================================

// DTZ = moves/plies until:
// - Pawn move or capture (zeroing move)
// - Checkmate
// Negative values indicate drawing/losing positions

// ============================================================================
// Tablebase Configuration
// ============================================================================

struct TablebaseConfig {
    std::string path;              // Path to tablebase files
    int maxPieces = 6;             // Max pieces for probing (5, 6, or 7)
    bool probeAtRoot = true;       // Probe at root for perfect play
    bool probeInSearch = true;     // Probe during search for pruning
};

// ============================================================================
// Tablebase Class
// ============================================================================

class Tablebases {
public:
    Tablebases() : initialized(false), maxPieces(0) {}

    // Initialize tablebases
    bool init(const std::string& path) {
        tbPath = path;

        // In a full implementation, this would:
        // 1. Open the directory
        // 2. Scan for .rtbw and .rtbz files
        // 3. Initialize Fathom library

        // For now, just check if we can theoretically use TBs
        if (!path.empty()) {
            initialized = true;
            maxPieces = 6;  // Assume 6-man tables
            return true;
        }

        return false;
    }

    // Check if tablebases are available
    bool is_initialized() const { return initialized; }

    // Get max piece count
    int max_pieces() const { return maxPieces; }

    // Check if position can be probed
    bool can_probe(const Board& board) const {
        if (!initialized) return false;

        int pieceCount = popcount(board.pieces());
        if (pieceCount > maxPieces) return false;

        // Can't probe if castling is still possible
        if (board.castling_rights() != NO_CASTLING) return false;

        return true;
    }

    // Probe WDL (Win/Draw/Loss)
    // Returns score from side to move's perspective
    WDLScore probe_wdl(const Board& board) const {
        if (!can_probe(board)) return WDL_NONE;

        // In a full implementation, this would call:
        // tb_probe_wdl(board position...)

        // Placeholder: return NONE (no probe available)
        (void)board;
        return WDL_NONE;
    }

    // Probe DTZ (Distance to Zero/mate)
    // Returns DTZ value or 0 if not available
    int probe_dtz(const Board& board, Move& bestMove) const {
        if (!can_probe(board)) {
            bestMove = MOVE_NONE;
            return 0;
        }

        // In a full implementation, this would call:
        // tb_probe_root(board position, &results)
        // Then return the best move and DTZ

        // Placeholder
        (void)board;
        bestMove = MOVE_NONE;
        return 0;
    }

    // Probe root for best move (perfect endgame play)
    Move probe_root(const Board& board) const {
        Move bestMove = MOVE_NONE;
        probe_dtz(board, bestMove);
        return bestMove;
    }

    // Convert WDL to centipawn score for search
    static int wdl_to_score(WDLScore wdl, int ply) {
        switch (wdl) {
            case WDL_WIN:         return VALUE_MATE - ply - 100;
            case WDL_CURSED_WIN:  return 1;  // Slight advantage
            case WDL_DRAW:        return 0;
            case WDL_BLESSED_LOSS: return -1; // Slight disadvantage
            case WDL_LOSS:        return -(VALUE_MATE - ply - 100);
            default:              return 0;
        }
    }

private:
    bool initialized;
    int maxPieces;
    std::string tbPath;
};

// Global tablebase instance
extern Tablebases TB;

// ============================================================================
// Simple Endgame Recognition (without tablebases)
// For basic endgame evaluation when TBs are not available
// ============================================================================

namespace EndgameRules {

// Check for known drawn endgames
inline bool is_known_draw(const Board& board) {
    int whitePawns = popcount(board.pieces(WHITE, PAWN));
    int blackPawns = popcount(board.pieces(BLACK, PAWN));
    int whiteKnights = popcount(board.pieces(WHITE, KNIGHT));
    int blackKnights = popcount(board.pieces(BLACK, KNIGHT));
    int whiteBishops = popcount(board.pieces(WHITE, BISHOP));
    int blackBishops = popcount(board.pieces(BLACK, BISHOP));
    int whiteRooks = popcount(board.pieces(WHITE, ROOK));
    int blackRooks = popcount(board.pieces(BLACK, ROOK));
    int whiteQueens = popcount(board.pieces(WHITE, QUEEN));
    int blackQueens = popcount(board.pieces(BLACK, QUEEN));

    int whitePieces = whiteKnights + whiteBishops + whiteRooks + whiteQueens;
    int blackPieces = blackKnights + blackBishops + blackRooks + blackQueens;

    // KvK
    if (whitePieces == 0 && blackPieces == 0 && whitePawns == 0 && blackPawns == 0) {
        return true;
    }

    // KNvK or KvKN
    if (whitePawns == 0 && blackPawns == 0) {
        if (whitePieces == 0 && blackPieces == 1 && blackKnights == 1) return true;
        if (blackPieces == 0 && whitePieces == 1 && whiteKnights == 1) return true;
    }

    // KBvK or KvKB
    if (whitePawns == 0 && blackPawns == 0) {
        if (whitePieces == 0 && blackPieces == 1 && blackBishops == 1) return true;
        if (blackPieces == 0 && whitePieces == 1 && whiteBishops == 1) return true;
    }

    // KNNvK (usually draw)
    if (whitePawns == 0 && blackPawns == 0) {
        if (whitePieces == 0 && blackPieces == 2 && blackKnights == 2) return true;
        if (blackPieces == 0 && whitePieces == 2 && whiteKnights == 2) return true;
    }

    // Opposite colored bishops (with no other pieces)
    if (whitePawns == 0 && blackPawns == 0 &&
        whitePieces == 1 && blackPieces == 1 &&
        whiteBishops == 1 && blackBishops == 1) {
        // Check if opposite colors
        Square wb = lsb(board.pieces(WHITE, BISHOP));
        Square bb = lsb(board.pieces(BLACK, BISHOP));
        bool whiteOnLight = ((file_of(wb) + rank_of(wb)) % 2) == 0;
        bool blackOnLight = ((file_of(bb) + rank_of(bb)) % 2) == 0;
        if (whiteOnLight != blackOnLight) {
            return true;  // Opposite colored bishops = likely draw
        }
    }

    return false;
}

// Scale factor for endgame (0-128, where 128 = normal)
// Lower values indicate more drawish positions
inline int scale_factor(const Board& board) {
    int whitePawns = popcount(board.pieces(WHITE, PAWN));
    int blackPawns = popcount(board.pieces(BLACK, PAWN));

    // No pawns = very drawish
    if (whitePawns == 0 && blackPawns == 0) {
        return 32;  // 25% of normal
    }

    // One side has no pawns = harder to win
    if (whitePawns == 0 || blackPawns == 0) {
        return 64;  // 50% of normal
    }

    return 128;  // Normal
}

} // namespace EndgameRules

} // namespace Tablebase

#endif // TABLEBASE_HPP
