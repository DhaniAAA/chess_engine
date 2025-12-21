#ifndef ZOBRIST_HPP
#define ZOBRIST_HPP

#include "types.hpp"
#include "bitboard.hpp"

// ============================================================================
// Zobrist Hashing
//
// Uses 64-bit random numbers to create unique hash keys for board positions.
// The hash is computed incrementally using XOR operations.
// ============================================================================

using Key = U64;

// ============================================================================
// Zobrist Random Numbers
// ============================================================================

namespace Zobrist {

// Random numbers for each piece on each square (12 pieces × 64 squares)
extern Key PieceSquare[PIECE_NB][SQUARE_NB];

// Random numbers for castling rights (16 possible combinations)
extern Key Castling[CASTLING_RIGHT_NB];

// Random numbers for en passant file (8 files)
extern Key EnPassant[FILE_NB];

// Random number for side to move (XOR when black to move)
extern Key SideToMove;

// ============================================================================
// Initialization
// ============================================================================

void init();

// ============================================================================
// Hash Computation Helpers
// ============================================================================

// Get hash component for a piece on a square
inline Key piece_key(Piece pc, Square sq) {
    return PieceSquare[pc][sq];
}

// Get hash component for castling rights
inline Key castling_key(CastlingRights cr) {
    return Castling[cr];
}

// Get hash component for en passant file
inline Key enpassant_key(File f) {
    return EnPassant[f];
}

// Get hash component for side to move
inline Key side_key() {
    return SideToMove;
}

} // namespace Zobrist

#endif // ZOBRIST_HPP
