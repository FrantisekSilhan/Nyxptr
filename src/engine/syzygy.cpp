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

#include "Nyxptr/engine/syzygy.h"
#include "Nyxptr/game/bits.h"
#include <algorithm>
#include <array>
#include <cstdlib>

extern "C" {
  #ifndef TB_NO_THREADS
    #define TB_NO_THREADS
  #endif
  #include "tbprobe.h"
}

namespace nyx::engine {
  namespace {
    int countPieces(const game::Board& board) {
      return bitboard::countBits(board.getCombinedOccupancy());
    }

    struct TbPosition {
      uint64_t white;
      uint64_t black;
      uint64_t kings;
      uint64_t queens;
      uint64_t rooks;
      uint64_t bishops;
      uint64_t knights;
      uint64_t pawns;
      unsigned rule50;
      unsigned ep;
      unsigned castling;
      bool turn;
    };

    TbPosition toTbPosition(const game::Board& board) {
      TbPosition pos{};
      pos.white = board.getOccupancy(game::Color::White);
      pos.black = board.getOccupancy(game::Color::Black);
      pos.kings = board.getBitboard(game::Color::White, game::Piece::King) | board.getBitboard(game::Color::Black, game::Piece::King);
      pos.queens = board.getBitboard(game::Color::White, game::Piece::Queen) | board.getBitboard(game::Color::Black, game::Piece::Queen);
      pos.rooks = board.getBitboard(game::Color::White, game::Piece::Rook) | board.getBitboard(game::Color::Black, game::Piece::Rook);
      pos.bishops = board.getBitboard(game::Color::White, game::Piece::Bishop) | board.getBitboard(game::Color::Black, game::Piece::Bishop);
      pos.knights = board.getBitboard(game::Color::White, game::Piece::Knight) | board.getBitboard(game::Color::Black, game::Piece::Knight);
      pos.pawns = board.getBitboard(game::Color::White, game::Piece::Pawn) | board.getBitboard(game::Color::Black, game::Piece::Pawn);
      pos.rule50 = static_cast<unsigned>(board.getHalfMoveClock());
      pos.ep = (board.getEnPassantSquare() == game::Square::None) ? 0u : static_cast<unsigned>(board.getEnPassantSquare());
      pos.castling = board.getCastlingRights();
      pos.turn = board.getSideToMove() == game::Color::White;
      return pos;
    }

    unsigned castlingToTbMask(uint8_t castlingRights) {
      unsigned tbCastling = 0;
      if (castlingRights & game::WhiteKingside) tbCastling |= TB_CASTLING_K;
      if (castlingRights & game::WhiteQueenside) tbCastling |= TB_CASTLING_Q;
      if (castlingRights & game::BlackKingside) tbCastling |= TB_CASTLING_k;
      if (castlingRights & game::BlackQueenside) tbCastling |= TB_CASTLING_q;
      return tbCastling;
    }

    bool rootMoveBetter(const TbRootMove& lhs, const TbRootMove& rhs) {
      if (lhs.tbRank != rhs.tbRank) return lhs.tbRank > rhs.tbRank;
      if (lhs.tbScore != rhs.tbScore) return lhs.tbScore > rhs.tbScore;
      return lhs.move < rhs.move;
    }

    bool isCastleMove(const game::Square to) {
      return to == game::Square::G1 || to == game::Square::C1 || to == game::Square::G8 || to == game::Square::C8;
    }

    game::Move convertTbMove(const game::Board& board, TbMove tbMove) {
      const auto from = static_cast<game::Square>(TB_MOVE_FROM(tbMove));
      const auto to = static_cast<game::Square>(TB_MOVE_TO(tbMove));
      const auto movingPiece = board.getPieceAt(from);
      const auto targetPiece = board.getPieceAt(to);
      const auto promote = TB_MOVE_PROMOTES(tbMove);

      if (movingPiece == game::Piece::King && isCastleMove(to)) {
        if (to == game::Square::G1 || to == game::Square::G8) {
          return game::Move(from, to, game::Move::KingCastle);
        }
        if (to == game::Square::C1 || to == game::Square::C8) {
          return game::Move(from, to, game::Move::QueenCastle);
        }
      }

      if (movingPiece == game::Piece::Pawn && board.getEnPassantSquare() == to && targetPiece == game::Piece::None) {
        return game::Move(from, to, game::Move::EnPassant);
      }

      if (movingPiece == game::Piece::Pawn && targetPiece == game::Piece::None &&
          game::to_i(from) % 8 == game::to_i(to) % 8 && std::abs(static_cast<int>(to) - static_cast<int>(from)) == 16) {
        return game::Move(from, to, game::Move::DoublePawnPush);
      }

      if (promote != TB_PROMOTES_NONE) {
        game::Move::Flags promotionFlag = game::Move::ProjQueen;
        switch (promote) {
          case TB_PROMOTES_QUEEN: promotionFlag = game::Move::ProjQueen; break;
          case TB_PROMOTES_ROOK: promotionFlag = game::Move::ProjRook; break;
          case TB_PROMOTES_BISHOP: promotionFlag = game::Move::ProjBishop; break;
          case TB_PROMOTES_KNIGHT: promotionFlag = game::Move::ProjKnight; break;
          default: break;
        }

        if (targetPiece != game::Piece::None) {
          return game::Move(from, to, static_cast<game::Move::Flags>(game::Move::Promotion | game::Move::Capture | promotionFlag));
        }
        return game::Move(from, to, static_cast<game::Move::Flags>(game::Move::Promotion | promotionFlag));
      }

      if (targetPiece != game::Piece::None) {
        return game::Move(from, to, game::Move::Capture);
      }

      return game::Move(from, to, game::Move::Quiet);
    }

    bool probeRoot(const TbPosition& pos, bool useRule50, bool hasRepeated, TbRootMoves& rootMoves, bool& usedDtz) {
      rootMoves.size = 0;
      usedDtz = false;

      const unsigned tbCastling = castlingToTbMask(static_cast<uint8_t>(pos.castling));
      if (tbCastling != 0) {
        return false;
      }

      int ok = tb_probe_root_dtz(pos.white, pos.black, pos.kings, pos.queens, pos.rooks, pos.bishops, pos.knights, pos.pawns,
                                 pos.rule50, tbCastling, pos.ep, pos.turn, hasRepeated, useRule50, &rootMoves);
      if (ok) {
        usedDtz = true;
        return true;
      }

      ok = tb_probe_root_wdl(pos.white, pos.black, pos.kings, pos.queens, pos.rooks, pos.bishops, pos.knights, pos.pawns,
                             pos.rule50, tbCastling, pos.ep, pos.turn, useRule50, &rootMoves);
      return ok != 0;
    }
  }

  bool Syzygy::init(const std::string& path) {
    return tb_init(path.c_str());
  }

  bool Syzygy::canProbe(const game::Board& board) {
    if (TB_LARGEST == 0) {
      return false;
    }

    if (board.getCastlingRights() != 0) {
      return false;
    }

    return countPieces(board) <= static_cast<int>(TB_LARGEST);
  }

  game::Move Syzygy::convertTbMoveToNyxptrMove(const game::Board& board, uint16_t tbMove) {
    return convertTbMove(board, tbMove);
  }

  unsigned Syzygy::probeWdl(const game::Board& board) {
    if (!canProbe(board)) {
      return TB_RESULT_FAILED;
    }

    const TbPosition pos = toTbPosition(board);
    const unsigned tbCastling = castlingToTbMask(static_cast<uint8_t>(pos.castling));
    if (tbCastling != 0) {
      return TB_RESULT_FAILED;
    }

    return tb_probe_wdl_impl(pos.white, pos.black, pos.kings, pos.queens, pos.rooks, pos.bishops, pos.knights, pos.pawns, pos.ep, pos.turn);
  }

  game::Move Syzygy::probeDtz(const game::Board& board) {
    if (!canProbe(board)) {
      return game::Move();
    }

    const TbPosition pos = toTbPosition(board);
    TbRootMoves rootMoves{};
    bool usedDtz = false;

    if (!probeRoot(pos, true, board.hasRepeatedPosition(), rootMoves, usedDtz) || rootMoves.size == 0) {
      return game::Move();
    }

    const TbRootMove* best = nullptr;
    for (unsigned i = 0; i < rootMoves.size; ++i) {
      const TbRootMove& candidate = rootMoves.moves[i];
      if (candidate.move == 0) continue;
      if (best == nullptr || rootMoveBetter(candidate, *best)) {
        best = &candidate;
      }
    }

    if (best == nullptr) {
      return game::Move();
    }

    return convertTbMoveToNyxptrMove(board, best->move);
  }
}