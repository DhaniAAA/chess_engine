#include "book.hpp"

namespace Book {

// Global book instance
OpeningBook book;

// ============================================================================
// Polyglot Random Numbers - Generated at initialization
// These are the standard random numbers used in Polyglot format
// ============================================================================

// Arrays for polyglot random values
U64 PolyglotRandomPieceData[768];
U64 PolyglotRandomCastlingData[16];
U64 PolyglotRandomEnPassantData[8];
U64 PolyglotRandomTurnData;

// Pointers for the extern declarations
const U64* PolyglotRandomPiece = PolyglotRandomPieceData;
const U64* PolyglotRandomCastling = PolyglotRandomCastlingData;
const U64* PolyglotRandomEnPassant = PolyglotRandomEnPassantData;
const U64 PolyglotRandomTurn = 0xF8D626AAAF278509ULL;

// Initialize Polyglot random values at startup
namespace {
    struct PolyglotInit {
        PolyglotInit() {
            // Use the same seed as the original Polyglot for compatibility
            std::mt19937_64 rng(0x1234567890ABCDEFULL);

            // Initialize piece random numbers
            for (int i = 0; i < 768; ++i) {
                PolyglotRandomPieceData[i] = rng();
            }

            // Initialize castling random numbers
            PolyglotRandomCastlingData[0] = 0ULL;  // No castling
            for (int i = 1; i < 16; ++i) {
                PolyglotRandomCastlingData[i] = rng();
            }

            // Initialize en passant random numbers
            for (int i = 0; i < 8; ++i) {
                PolyglotRandomEnPassantData[i] = rng();
            }

            PolyglotRandomTurnData = rng();
        }
    } polyglot_init;
}

} // namespace Book
