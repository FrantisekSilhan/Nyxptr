/*
  Nyxptr, a chess engine written in C++20
  Copyright (C) 2026 The Nyxptr developers (see AUTHORS file)

  Nyxptr is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  Nyxptr is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <https://www.gnu.org/licenses/>
*/

#include "Nyxptr/game/movegen.h"
#include "Nyxptr/game/bits.h"
#include "Nyxptr/game/lookups.h"
#include <array>
#include <cstdint>

using namespace nyx::bitboard;

namespace nyx::game {
  namespace {
    void generateSlider(const Board& board, std::vector<Move>& moves, Piece p, bool isRook) {
      Color us = board.getSideToMove();
      uint64_t sliders = board.getBitboard(us, p);
      uint64_t occupancy = board.getCombinedOccupancy();
      uint64_t enemyOrEmpty = ~board.getOccupancy(us);

      while (sliders) {
        Square from = popLSBSquare(sliders);
        const lookups::Magic& m = isRook ? lookups::rookMagics[static_cast<int>(from)] : lookups::bishopMagics[static_cast<int>(from)];

        uint64_t attacks = m.ptr[getMagicIndex(m, occupancy)] & enemyOrEmpty;

        while (attacks) {
          Square to = popLSBSquare(attacks);
          moves.emplace_back(from, to, static_cast<Move::Flags>(board.getPieceAt(to) != Piece::None ? Move::Capture : Move::Quiet));
        }
      }
    }

    void generatePawnMoves(const Board& board, std::vector<Move>& moves) {
      const Color us = board.getSideToMove();
      const uint64_t pawns = board.getBitboard(us, Piece::Pawn);
      const uint64_t empty = ~board.getCombinedOccupancy();
      const uint64_t enemies = board.getOccupancy(!us);
      const uint64_t enPassantBB = (board.getEnPassantSquare() == Square::None) 
                                  ? 0 
                                  : (1ULL << static_cast<int>(board.getEnPassantSquare()));

      const uint64_t notAFile = 0xFEFEFEFEFEFEFEFEULL;
      const uint64_t notHFile = 0x7F7F7F7F7F7F7F7FULL;

      if (us == Color::White) {
        uint64_t singlePush = (pawns << 8) & empty;
        uint64_t doublePush = ((singlePush & 0x0000000000FF0000ULL) << 8) & empty;

        uint64_t pushPromos = singlePush & 0xFF00000000000000ULL;
        uint64_t pushQuiets = singlePush & ~0xFF00000000000000ULL;

        while (pushQuiets) {
          Square to = popLSBSquare(pushQuiets);
          moves.emplace_back(static_cast<Square>(static_cast<int>(to) - 8), to, Move::Quiet);
        }
        while (pushPromos) {
          Square to = popLSBSquare(pushPromos);
          Square from = static_cast<Square>(static_cast<int>(to) - 8);
          moves.emplace_back(from, to, Move::Promotion | Move::ProjQueen);
          moves.emplace_back(from, to, Move::Promotion | Move::ProjRook);
          moves.emplace_back(from, to, Move::Promotion | Move::ProjBishop);
          moves.emplace_back(from, to, Move::Promotion | Move::ProjKnight);
        }
        while (doublePush) {
          Square to = popLSBSquare(doublePush);
          moves.emplace_back(static_cast<Square>(static_cast<int>(to) - 16), to, Move::DoublePawnPush);
        }
      } else {
        uint64_t singlePush = (pawns >> 8) & empty;
        uint64_t doublePush = ((singlePush & 0x0000FF0000000000ULL) >> 8) & empty;

        uint64_t pushPromos = singlePush & 0x00000000000000FFULL;
        uint64_t pushQuiets = singlePush & ~0x00000000000000FFULL;

        while (pushQuiets) {
          Square to = popLSBSquare(pushQuiets);
          moves.emplace_back(static_cast<Square>(static_cast<int>(to) + 8), to, Move::Quiet);
        }
        while (pushPromos) {
          Square to = popLSBSquare(pushPromos);
          Square from = static_cast<Square>(static_cast<int>(to) + 8);
          moves.emplace_back(from, to, Move::Promotion | Move::ProjQueen);
          moves.emplace_back(from, to, Move::Promotion | Move::ProjRook);
          moves.emplace_back(from, to, Move::Promotion | Move::ProjBishop);
          moves.emplace_back(from, to, Move::Promotion | Move::ProjKnight);
        }
        while (doublePush) {
          Square to = popLSBSquare(doublePush);
          moves.emplace_back(static_cast<Square>(static_cast<int>(to) + 16), to, Move::DoublePawnPush);
        }
      }

      auto addCaptures = [&](uint64_t targets, int offset) {
        while (targets) {
          Square to = popLSBSquare(targets);
          Square from = static_cast<Square>(static_cast<int>(to) - offset);
          if ((1ULL << static_cast<int>(to)) & (us == Color::White ? 0xFF00000000000000ULL : 0x00000000000000FFULL)) {
            moves.emplace_back(from, to, Move::Promotion | Move::Capture | Move::ProjQueen);
            moves.emplace_back(from, to, Move::Promotion | Move::Capture | Move::ProjRook);
            moves.emplace_back(from, to, Move::Promotion | Move::Capture | Move::ProjBishop);
            moves.emplace_back(from, to, Move::Promotion | Move::Capture | Move::ProjKnight);
          } else {
            moves.emplace_back(from, to, Move::Capture);
          }
        }
      };

      if (us == Color::White) {
        addCaptures((pawns & notAFile) << 7 & enemies, 7);
        addCaptures((pawns & notHFile) << 9 & enemies, 9);
        
        uint64_t epLeft = ((pawns & notAFile) << 7) & enPassantBB;
        uint64_t epRight = ((pawns & notHFile) << 9) & enPassantBB;
        if (epLeft) moves.emplace_back(static_cast<Square>(static_cast<int>(getLSBSquare(epLeft)) - 7), getLSBSquare(epLeft), Move::EnPassant);
        if (epRight) moves.emplace_back(static_cast<Square>(static_cast<int>(getLSBSquare(epRight)) - 9), getLSBSquare(epRight), Move::EnPassant);
      } else {
        addCaptures((pawns & notHFile) >> 7 & enemies, -7);
        addCaptures((pawns & notAFile) >> 9 & enemies, -9);

        uint64_t epRight = ((pawns & notAFile) >> 9) & enPassantBB;
        uint64_t epLeft = ((pawns & notHFile) >> 7) & enPassantBB;
        if (epLeft) moves.emplace_back(static_cast<Square>(static_cast<int>(getLSBSquare(epLeft)) + 9), getLSBSquare(epLeft), Move::EnPassant);
        if (epRight) moves.emplace_back(static_cast<Square>(static_cast<int>(getLSBSquare(epRight)) + 7), getLSBSquare(epRight), Move::EnPassant);
      }
    }

    void generateKnightMoves(const Board& board, std::vector<Move>& moves) {
      Color us = board.getSideToMove();
      uint64_t knights = board.getBitboard(us, Piece::Knight);
      uint64_t ourPieces = board.getOccupancy(us);

      while (knights) {
        Square from = popLSBSquare(knights);
        uint64_t targets = lookups::knightTable[static_cast<int>(from)] & ~ourPieces;

        while (targets) {
          Square to = popLSBSquare(targets);
          moves.emplace_back(from, to, static_cast<Move::Flags>(board.getPieceAt(to) != Piece::None ? Move::Capture : Move::Quiet));
        }
      }
    }

    void generateBishopMoves(const Board& board, std::vector<Move>& moves) {
      generateSlider(board, moves, Piece::Bishop, false);
    }

    void generateRookMoves(const Board& board, std::vector<Move>& moves) {
      generateSlider(board, moves, Piece::Rook, true);
    }

    void generateQueenMoves(const Board& board, std::vector<Move>& moves) {
      generateSlider(board, moves, Piece::Queen, false);
      generateSlider(board, moves, Piece::Queen, true);
    }

    void generateKingMoves(const Board& board, std::vector<Move>& moves) {
      Color us = board.getSideToMove();
      uint64_t king = board.getBitboard(us, Piece::King);
      if (!king) return;

      Square from = getLSBSquare(king);
      uint64_t targets = lookups::kingTable[static_cast<int>(from)] & ~board.getOccupancy(us);

      while (targets) {
        Square to = popLSBSquare(targets);
        moves.emplace_back(from, to, static_cast<Move::Flags>(board.getPieceAt(to) != Piece::None ? Move::Capture : Move::Quiet));
      }

      uint8_t rights = board.getCastlingRights();
      uint64_t combined = board.getCombinedOccupancy();

      if (us == Color::White) {
        if ((rights & WhiteKingside) && !(combined & (1ULL << 5 | 1ULL << 6))) {
          moves.emplace_back(from, Square::G1, Move::KingCastle);
        }
        if ((rights & WhiteQueenside) && !(combined & (1ULL << 1 | 1ULL << 2 | 1ULL << 3))) {
          moves.emplace_back(from, Square::C1, Move::QueenCastle);
        }
      } else {
        if ((rights & BlackKingside) && !(combined & (1ULL << 61 | 1ULL << 62))) {
          moves.emplace_back(from, Square::G8, Move::KingCastle);
        }
        if ((rights & BlackQueenside) && !(combined & (1ULL << 57 | 1ULL << 58 | 1ULL << 59))) {
          moves.emplace_back(from, Square::C8, Move::QueenCastle);
        }
      }
    }
  }
  
  std::vector<Move> MoveGen::generateMoves(const Board& board) {
    std::vector<Move> moves;
    moves.reserve(256);

    generatePawnMoves(board, moves);

    generateKnightMoves(board, moves);

    generateBishopMoves(board, moves);
    generateRookMoves(board, moves);
    generateQueenMoves(board, moves);

    generateKingMoves(board, moves);

    return moves;
  }

  void MoveGen::filterLegalMoves(Board& board, std::vector<Move>& pseudoMoves) {
    std::erase_if(pseudoMoves, [&](const Move& m) {
      if (!board.makeMove(m)) return true;
      board.undoMove();
      return false;
    });
  }
}