#ifndef BOOK_HPP
#define BOOK_HPP

#include "board.hpp"
#include "move.hpp"
#include <string>
#include <vector>
#include <fstream>
#include <random>
#include <algorithm>

// ============================================================================
// Polyglot Opening Book Support (TAHAP 5)
//
// Polyglot format is a standard opening book format used by many chess engines.
// Each entry is 16 bytes:
//   - 8 bytes: Zobrist key (using Polyglot's own hashing scheme)
//   - 2 bytes: move (encoded in special format)
//   - 2 bytes: weight (popularity/strength)
//   - 4 bytes: learn data (usually 0)
// ============================================================================

namespace Book {

// ============================================================================
// Polyglot Key Computation
// Different from our Zobrist - uses Polyglot's standard random numbers
// ============================================================================

// Polyglot random numbers (initialized in book.cpp)
extern const U64* PolyglotRandomPiece;       // [piece][square] - 768 values
extern const U64* PolyglotRandomCastling;    // [castling rights] - 16 values
extern const U64* PolyglotRandomEnPassant;   // [en passant file] - 8 values
extern const U64 PolyglotRandomTurn;         // Side to move

// Piece mapping from our encoding to Polyglot
// Polyglot: BP=0, WP=1, BN=2, WN=3, BB=4, WB=5, BR=6, WR=7, BQ=8, WQ=9, BK=10, WK=11
inline int polyglot_piece(Piece pc) {
    if (pc == NO_PIECE) return -1;

    PieceType pt = type_of(pc);
    Color c = color_of(pc);

    // Polyglot uses: pawn=0, knight=1, bishop=2, rook=3, queen=4, king=5
    // Then color: black=0, white=1
    int base = 0;
    switch (pt) {
        case PAWN:   base = 0; break;
        case KNIGHT: base = 1; break;
        case BISHOP: base = 2; break;
        case ROOK:   base = 3; break;
        case QUEEN:  base = 4; break;
        case KING:   base = 5; break;
        default:     return -1;
    }

    // Polyglot indexes: color * 6 + piece_type, but reversed (black=0, white=1)
    return base * 2 + (c == WHITE ? 1 : 0);
}

// Compute Polyglot hash key for a position
inline U64 polyglot_key(const Board& board) {
    U64 key = 0;

    // Pieces on board
    for (Square s = SQ_A1; s <= SQ_H8; ++s) {
        Piece pc = board.piece_on(s);
        if (pc != NO_PIECE) {
            int poly_piece = polyglot_piece(pc);
            if (poly_piece >= 0) {
                key ^= PolyglotRandomPiece[64 * poly_piece + s];
            }
        }
    }

    // Castling rights
    int castling = 0;
    CastlingRights cr = board.castling_rights();
    if (cr & WHITE_OO)  castling |= 1;
    if (cr & WHITE_OOO) castling |= 2;
    if (cr & BLACK_OO)  castling |= 4;
    if (cr & BLACK_OOO) castling |= 8;
    key ^= PolyglotRandomCastling[castling];

    // En passant (only if pawn can actually capture)
    Square ep = board.en_passant_square();
    if (ep != SQ_NONE) {
        File f = file_of(ep);
        // Check if there's actually a pawn that can capture
        Bitboard pawns = board.pieces(board.side_to_move(), PAWN);
        Bitboard canCapture = 0;
        int offset = (board.side_to_move() == WHITE) ? -8 : 8;
        if (f > FILE_A) canCapture |= square_bb(Square(int(ep) - 1 + offset));
        if (f < FILE_H) canCapture |= square_bb(Square(int(ep) + 1 + offset));

        if (pawns & canCapture) {
            key ^= PolyglotRandomEnPassant[f];
        }
    }

    // Side to move (Polyglot: XOR if white to move)
    if (board.side_to_move() == WHITE) {
        key ^= PolyglotRandomTurn;
    }

    return key;
}

// ============================================================================
// Polyglot Move Encoding
// ============================================================================

// Polyglot move format:
// bits 0-5: to square
// bits 6-11: from square
// bits 12-14: promotion piece (0=none, 1=knight, 2=bishop, 3=rook, 4=queen)

inline Move decode_polyglot_move(const Board& board, uint16_t poly_move) {
    int to_sq = poly_move & 0x3F;
    int from_sq = (poly_move >> 6) & 0x3F;
    int promo = (poly_move >> 12) & 0x7;

    Square from = Square(from_sq);
    Square to = Square(to_sq);

    // Handle castling (Polyglot encodes king destination, not rook)
    Piece pc = board.piece_on(from);
    if (type_of(pc) == KING) {
        // Kingside castling
        if (from == SQ_E1 && to == SQ_H1) to = SQ_G1;
        if (from == SQ_E8 && to == SQ_H8) to = SQ_G8;
        // Queenside castling
        if (from == SQ_E1 && to == SQ_A1) to = SQ_C1;
        if (from == SQ_E8 && to == SQ_A8) to = SQ_C8;
    }

    // Create move using static factory methods
    if (promo > 0) {
        PieceType promo_pt = PieceType(promo + 1);  // 1=knight, 2=bishop, 3=rook, 4=queen
        return Move::make_promotion(from, to, promo_pt);
    }

    // En passant detection
    if (type_of(pc) == PAWN && to == board.en_passant_square()) {
        return Move::make_enpassant(from, to);
    }

    // Castling detection
    if (type_of(pc) == KING && std::abs(file_of(from) - file_of(to)) > 1) {
        return Move::make_castling(from, to);
    }

    return Move::make(from, to);
}

// ============================================================================
// Book Entry Structure
// ============================================================================

struct BookEntry {
    U64 key;
    uint16_t move;
    uint16_t weight;
    uint32_t learn;

    BookEntry() : key(0), move(0), weight(0), learn(0) {}
};

// ============================================================================
// Opening Book Class
// ============================================================================

class OpeningBook {
public:
    OpeningBook() : loaded(false), variety(true) {}

    // Load a Polyglot book file
    bool load(const std::string& filename) {
        std::ifstream file(filename, std::ios::binary);
        if (!file.is_open()) {
            return false;
        }

        entries.clear();

        while (file) {
            BookEntry entry;

            // Read 8 bytes for key (big-endian)
            U64 key = 0;
            for (int i = 0; i < 8; ++i) {
                char c;
                if (!file.get(c)) break;
                key = (key << 8) | static_cast<unsigned char>(c);
            }
            if (!file) break;
            entry.key = key;

            // Read 2 bytes for move (big-endian)
            char buf[2];
            file.read(buf, 2);
            if (!file) break;
            entry.move = (static_cast<unsigned char>(buf[0]) << 8) |
                          static_cast<unsigned char>(buf[1]);

            // Read 2 bytes for weight (big-endian)
            file.read(buf, 2);
            if (!file) break;
            entry.weight = (static_cast<unsigned char>(buf[0]) << 8) |
                            static_cast<unsigned char>(buf[1]);

            // Read 4 bytes for learn (skip)
            char learn_buf[4];
            file.read(learn_buf, 4);
            if (!file) break;

            entries.push_back(entry);
        }

        // Sort entries by key for binary search
        std::sort(entries.begin(), entries.end(),
            [](const BookEntry& a, const BookEntry& b) {
                return a.key < b.key;
            });

        loaded = !entries.empty();
        return loaded;
    }

    // Check if book is loaded
    bool is_loaded() const { return loaded; }

    // Get number of entries
    size_t size() const { return entries.size(); }

    // Set variety mode (random selection from weighted moves)
    void set_variety(bool v) { variety = v; }

    // Probe the book for a move
    Move probe(const Board& board) const {
        if (!loaded) return MOVE_NONE;

        U64 key = polyglot_key(board);

        // Find all matching entries using binary search
        auto lower = std::lower_bound(entries.begin(), entries.end(), key,
            [](const BookEntry& e, U64 k) { return e.key < k; });

        if (lower == entries.end() || lower->key != key) {
            return MOVE_NONE;
        }

        // Collect all moves for this position
        std::vector<std::pair<Move, int>> moves;
        for (auto it = lower; it != entries.end() && it->key == key; ++it) {
            Move m = decode_polyglot_move(board, it->move);
            if (m != MOVE_NONE) {
                moves.push_back({m, it->weight});
            }
        }

        if (moves.empty()) {
            return MOVE_NONE;
        }

        // Select move (variety mode = weighted random, else = best)
        if (variety && moves.size() > 1) {
            // Calculate total weight
            int totalWeight = 0;
            for (const auto& mv : moves) {
                totalWeight += mv.second;
            }

            // Random selection based on weight
            static std::random_device rd;
            static std::mt19937 gen(rd());
            std::uniform_int_distribution<> dis(0, totalWeight - 1);

            int pick = dis(gen);
            int cumulative = 0;
            for (const auto& mv : moves) {
                cumulative += mv.second;
                if (pick < cumulative) {
                    return mv.first;
                }
            }
        }

        // Return highest weighted move
        auto best = std::max_element(moves.begin(), moves.end(),
            [](const auto& a, const auto& b) { return a.second < b.second; });

        return best->first;
    }

    // Get all book moves for a position (for analysis)
    std::vector<std::pair<Move, int>> get_moves(const Board& board) const {
        std::vector<std::pair<Move, int>> moves;
        if (!loaded) return moves;

        U64 key = polyglot_key(board);

        auto lower = std::lower_bound(entries.begin(), entries.end(), key,
            [](const BookEntry& e, U64 k) { return e.key < k; });

        for (auto it = lower; it != entries.end() && it->key == key; ++it) {
            Move m = decode_polyglot_move(board, it->move);
            if (m != MOVE_NONE) {
                moves.push_back({m, it->weight});
            }
        }

        // Sort by weight (descending)
        std::sort(moves.begin(), moves.end(),
            [](const auto& a, const auto& b) { return a.second > b.second; });

        return moves;
    }

private:
    std::vector<BookEntry> entries;
    bool loaded;
    bool variety;
};

// Global book instance
extern OpeningBook book;

} // namespace Book

#endif // BOOK_HPP
