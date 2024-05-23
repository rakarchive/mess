// Copyright © 2023 Rak Laptudirm <rak@laptudirm.com>
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
// http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#ifndef CHESS_POSITION
#define CHESS_POSITION

#include <array>
#include <cassert>
#include <utility>
#include <cstdint>

#include "fen.hpp"
#include "move.hpp"
#include "moves.hpp"
#include "square.hpp"
#include "zobrist.hpp"
#include "castling.hpp"
#include "bitboard.hpp"

namespace Chess {
    // Position represents a particular chess board position.
    // It also exposes a variety of fields and methods which
    // allow fetching information and manipulating said position.
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-member-init)
    struct Position {
        // BitBoard Board representation.
        // x6 piece bbs, x2 color bbs.
        std::array<BitBoard, Piece::N> PieceBBs;
        std::array<BitBoard, Color::N> ColorBBs;

        // Zobrist Hash of the chess position.
        Hash Hash;

        // Checker BitBoard of the current Position. It
        // contains the location of all the pieces checking
        // /attacking the side to move's king.
        BitBoard Checkers;

        // Castling Rights of the current position records
        // all the ways it is possible to castle in the
        // current position and in the future.
        Castling::Rights Rights;

        // SideToMove records the current side to move Color.
        Color  SideToMove;

        // EpTarget records the current En-Passant Target.
        Square EpTarget;

        // DrawClock records the current 50-move rule
        // draw clock which determines if the game is a
        // draw due to the said rule.
        uint8_t DrawClock;

        // CheckNum stores the number of checkers checking/
        // attacking the side to move's king. It can also
        // be considered as the number of set bits in the
        // Checkers BitBoard.
        uint8_t CheckNum;

        // Default constructor for Position, no fields are
        // initialized by calling this method.
        constexpr inline Position() = default;

        // Insert safely inserts the given piece into the given empty square
        // updating all the relevant info so the Position stays consistent.
        constexpr inline void Insert(Square square, ColoredPiece piece) {
            // Assert that the square and the piece are valid, and that
            // the target square is empty so that a piece can be placed.
            assert(square != Square::None && piece != ColoredPiece::None);

            // Insert the given piece into the BitBoard representation.
            PieceBBs[static_cast<uint8_t>(piece.Piece())].Flip(square);
            ColorBBs[static_cast<uint8_t>(piece.Color())].Flip(square);

            // Add the given piece to the Zobrist hash of the Position.
            Hash += Keys::PieceOnSquare(piece, square);
        }

        // Remove safely removes the piece occupying the given square,
        // updating all the relevant info so the Position stays consistent.
        constexpr inline void Remove(Square square) {
            // Assert that the square is valid.
            assert(square != Square::None);

            // Fetch the piece present at the given square.
            const ColoredPiece piece = (*this)[square];

            // Assert that there is a piece to remove.
            assert(piece != ColoredPiece::None);

            // Remove the piece from the BitBoard representation.
            PieceBBs[static_cast<uint8_t>(piece.Piece())].Flip(square);
            ColorBBs[static_cast<uint8_t>(piece.Color())].Flip(square);

            // Remove the given piece from the Zobrist hash of the Position.
            Hash -= Keys::PieceOnSquare(piece, square);
        }

        // Indexing with a Piece return's that Piece's BitBoard.
        constexpr inline BitBoard operator [](const Piece piece) const {
            return PieceBBs[static_cast<uint8_t>(piece)];
        }

        // Indexing with a Color return's that Color's BitBoard.
        constexpr inline BitBoard operator [](const Color color) const {
            return ColorBBs[static_cast<uint8_t>(color)];
        }

        /****************************
         * Public Utility Functions *
         ****************************/

        // Attacked checks if the given square has been attacked by pieces of the given color, given
        // the provided blocker BitBoard on the target Position.
        template <Color BY>
        [[nodiscard]] constexpr inline bool Attacked(const Square square, const BitBoard blockers) const {
            const BitBoard attackers = (*this)[BY];

            // Check for pawn attackers.
            const BitBoard attackingPawns = (*this)[Piece::Pawn] & attackers;
            if (!attackingPawns.IsDisjoint(MoveTable::Pawn<!BY>(square))) return true;

            // Check for knight attackers.
            const BitBoard attackingKnights = (*this)[Piece::Knight] & attackers;
            if (!attackingKnights.IsDisjoint(MoveTable::Knight(square))) return true;

            const BitBoard attackingQueens = (*this)[Piece::Queen];

            // Check for bishop type attackers.
            const BitBoard attackingBishops = ((*this)[Piece::Bishop] | attackingQueens) & attackers;
            if (!attackingBishops.IsDisjoint(MoveTable::Bishop(square, blockers))) return true;

            // Check for rook type attackers.
            const BitBoard attackingRooks = ((*this)[Piece::Rook] | attackingQueens) & attackers;
            if (!attackingRooks.IsDisjoint(MoveTable::Rook(square, blockers))) return true;

            // Check for a king attacker.
            const BitBoard attackingKing = (*this)[Piece::King] & attackers;
            if (!attackingKing.IsDisjoint(MoveTable::King(square))) return true;

            // No attackers found.
            return false;
        }

        // An overload of the standard Attacked function which uses
        // the occupied BitBoard as its blocker set automatically.
        template <Color BY>
        [[maybe_unused]] [[nodiscard]] constexpr inline bool Attacked(const Square square) const {
            return Attacked<BY>(square, (*this)[BY] | (*this)[!BY]);
        }

        // An overload of the standard Attacked function which checks for attacks to
        // multiple squares. The function returns true if any of the squares is attacked.
        template<Color BY>
        [[nodiscard]] inline bool Attacked(const BitBoard targets, const BitBoard blockers) const {
            for (const auto target : targets)
                if (Attacked<BY>(target, blockers)) return true;
            return false;
        }

        // An overload of the standard Attacked function which is similar to the one which
        // operates on multiples squares, except that the blocker set is automatically set
        // to be the occupied BitBoard.
        template<Color BY>
        [[nodiscard]] inline bool Attacked(const BitBoard targets) const {
            return Attacked<BY>(targets, (*this)[BY] | (*this)[!BY]);
        }

        // Constructor of Position which operates on a raw FEN string.
        // NOLINTNEXTLINE(cppcoreguidelines-pro-type-member-init)
        explicit Position(const std::string& fenString) {
            *this = Position(FEN(fenString));
        }

        // Constructor of Position which operates on a parsed FEN string.
        // NOLINTNEXTLINE(cppcoreguidelines-pro-type-member-init)
        explicit Position(const FEN& fen) {
            Hash = Keys::None;

            // Copy the relevant fields.
            SideToMove = fen.SideToMove;     if (SideToMove !=    Color::White) Hash += Keys::SideToMove;
            EpTarget   = fen.EPTarget;       if (EpTarget   !=   Square::None ) Hash += Keys::EnPassantTarget(EpTarget);
            Rights     = fen.CastlingRights; if (Rights     != Castling::None ) Hash += Keys::CastlingRights(Rights);
            DrawClock  = fen.DrawClock;

            // Zero out the board representation.
            PieceBBs = {};
            ColorBBs = {};

            // Populate the board representation.
            for (uint8_t sq = 0; sq < Square::N; sq++)
                if (fen.Mailbox[sq] != ColoredPiece::None)
                    Insert(Square(sq), fen.Mailbox[sq]);

            GenerateCheckers();
        }

        // Indexing Position by Square returns the ColoredPiece at that Square.
        constexpr inline ColoredPiece operator[](const Square sq) const {
            const auto& position = *this;

            const Color color = position[Color::White][sq] ? Color::White :
                                position[Color::Black][sq] ? Color::Black : Color::None;

            if (color == Color::None) return ColoredPiece::None;

            const Piece piece = position[Piece::Pawn][sq] ? Piece::Pawn :
                                position[Piece::Knight][sq] ? Piece::Knight :
                                position[Piece::Bishop][sq] ? Piece::Bishop :
                                position[Piece::Rook][sq] ? Piece::Rook :
                                position[Piece::King][sq] ? Piece::King :
                                position[Piece::Queen][sq] ? Piece::Queen : Piece::None;

            return piece + color;
        }

        // ToString converts the position to a human-readable string representation.
        [[nodiscard]] constexpr std::string ToString() const {
            std::string board = "+---+---+---+---+---+---+---+---+\n";

            for (uint8_t rank = 7; rank != 255; rank--) {
                board += "| ";

                for (uint8_t file = 0; file < File::N; file++) {
                    board += (*this)[Square(rank * 8 + file)].ToString() + " | ";
                }

                board += Rank(rank).ToString();
                board += "\n+---+---+---+---+---+---+---+---+\n";
            }

            board += "  a   b   c   d   e   f   g   h\n";
            return board;
        }

        // GenerateCheckers generates the Checkers BitBoard.
        constexpr inline void GenerateCheckers() {
            const auto friends = (*this)[ SideToMove];
            const auto enemies = (*this)[!SideToMove];
            const auto occupied = friends | enemies;

            assert((*this)[Piece::King] != BitBoards::Empty);
            const auto king = ((*this)[Piece::King] & friends).LSB();
            assert(king != Square::None);

            // Get the Piece BitBoards.
            const auto p = (*this)[Piece::Pawn  ];
            const auto n = (*this)[Piece::Knight];
            const auto b = (*this)[Piece::Bishop];
            const auto r = (*this)[Piece::Rook  ];
            const auto q = (*this)[Piece::Queen ];

            // Treating the king as a super-piece, check for any pieces that fall
            // into its attack range with the same type of attack as the range.
            const auto checkingP = p & MoveTable::Pawn(SideToMove, king);
            const auto checkingN = n & MoveTable::Knight(king);
            const auto checkingD = (b | q) & MoveTable::Bishop(king, occupied);
            const auto checkingL = (r | q) & MoveTable::Rook  (king, occupied);

            // Cast out the friendly pieces from the BitBoard and store it.
            // Also store the number of checkers in the other variable.
            Checkers = (checkingP | checkingN | checkingD | checkingL) & enemies;
            CheckNum = Checkers.PopCount();
        }

        static constexpr Chess::Hash ZobristHash(const Position& position) {
            auto hash = Keys::None;

            if (position.SideToMove != Color::White) hash += Keys::SideToMove;
            if (position.EpTarget != Square::None) hash += Keys::EnPassantTarget(position.EpTarget);

            hash += Keys::CastlingRights(position.Rights);

            for (uint8_t square = 0; square < Square::N; square++)
                if (position[Square(square)] != ColoredPiece::None)
                    hash += Keys::PieceOnSquare(position[Square(square)], Square(square));

            return hash;
        }
    };

    constexpr inline std::ostream& operator<<(std::ostream& os, const Position& position) {
        os << position.ToString();
        return os;
    }
}

#endif //CHESS_POSITION
