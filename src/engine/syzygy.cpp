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
#include "Nyxptr/game/movegen.h"
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
    unsigned probe_wdl_internal(const game::Board& board) {
      return tb_probe_wdl_impl(
        board.getOccupancy(game::Color::White),
        board.getOccupancy(game::Color::Black),
        board.getBitboard(game::Color::White, game::Piece::King) | board.getBitboard(game::Color::Black, game::Piece::King),
        board.getBitboard(game::Color::White, game::Piece::Queen) | board.getBitboard(game::Color::Black, game::Piece::Queen),
        board.getBitboard(game::Color::White, game::Piece::Rook) | board.getBitboard(game::Color::Black, game::Piece::Rook),
        board.getBitboard(game::Color::White, game::Piece::Bishop) | board.getBitboard(game::Color::Black, game::Piece::Bishop),
        board.getBitboard(game::Color::White, game::Piece::Knight) | board.getBitboard(game::Color::Black, game::Piece::Knight),
        board.getBitboard(game::Color::White, game::Piece::Pawn) | board.getBitboard(game::Color::Black, game::Piece::Pawn),
        board.getEnPassantSquare() == game::Square::None ? 0 : static_cast<unsigned>(board.getEnPassantSquare()),
        board.getSideToMove() == game::Color::White
      );
    }

    game::Move convertTbMove(const game::Board& board, TbMove tbMove) {
      auto from = static_cast<game::Square>(TB_MOVE_FROM(tbMove));
      auto to = static_cast<game::Square>(TB_MOVE_TO(tbMove));
      auto prom = TB_MOVE_PROMOTES(tbMove);

      auto moves = game::MoveGen::generateMoves(board);
      game::MoveGen::filterLegalMoves(const_cast<game::Board&>(board), moves);

      for (const auto& m : moves) {
        if (static_cast<game::Square>(m.getFrom()) == from && static_cast<game::Square>(m.getTo()) == to) {
          if (prom != TB_PROMOTES_NONE) {
            uint16_t f = m.getFlags();
            if (prom == TB_PROMOTES_QUEEN  && (f & 0x3) == 3) return m;
            if (prom == TB_PROMOTES_ROOK   && (f & 0x3) == 2) return m;
            if (prom == TB_PROMOTES_BISHOP && (f & 0x3) == 1) return m;
            if (prom == TB_PROMOTES_KNIGHT && (f & 0x3) == 0) return m;
            continue;
          }
          return m;
        }
      }
      return game::Move();
    }
  }

  bool Syzygy::init(const std::string& path) {
    return tb_init(path.c_str());
  }

  bool Syzygy::canProbe(const game::Board& board) {
    if (TB_LARGEST == 0 || board.getCastlingRights() != 0) {
      return false;
    }

    int count = bitboard::countBits(board.getCombinedOccupancy());
    return count <= static_cast<int>(TB_LARGEST);
  }

  unsigned Syzygy::probeWdl(const game::Board& board) {
    if (!canProbe(board)) return TB_RESULT_FAILED;
    return probe_wdl_internal(board);
  }

  game::Move Syzygy::convertTbMoveToNyxptrMove(const game::Board& board, uint16_t tbMove) {
    return convertTbMove(board, tbMove);
  }

  game::Move Syzygy::probeDtz(const game::Board& board) {
    if (!canProbe(board)) return game::Move();

    TbRootMoves rm;

    int success = tb_probe_root_dtz(
      board.getOccupancy(game::Color::White),
      board.getOccupancy(game::Color::Black),
      board.getBitboard(game::Color::White, game::Piece::King) | board.getBitboard(game::Color::Black, game::Piece::King),
      board.getBitboard(game::Color::White, game::Piece::Queen) | board.getBitboard(game::Color::Black, game::Piece::Queen),
      board.getBitboard(game::Color::White, game::Piece::Rook) | board.getBitboard(game::Color::Black, game::Piece::Rook),
      board.getBitboard(game::Color::White, game::Piece::Bishop) | board.getBitboard(game::Color::Black, game::Piece::Bishop),
      board.getBitboard(game::Color::White, game::Piece::Knight) | board.getBitboard(game::Color::Black, game::Piece::Knight),
      board.getBitboard(game::Color::White, game::Piece::Pawn) | board.getBitboard(game::Color::Black, game::Piece::Pawn),
      board.getHalfMoveClock(),
      0, 
      board.getEnPassantSquare() == game::Square::None ? 0 : static_cast<unsigned>(board.getEnPassantSquare()),
      board.getSideToMove() == game::Color::White,
      board.hasRepeatedPosition(),
      true,
      &rm
    );

    if (!success || rm.size == 0) return game::Move();

    unsigned bestIdx = 0;
    for (unsigned i = 1; i < rm.size; ++i) {
      if (rm.moves[i].tbRank > rm.moves[bestIdx].tbRank) {
        bestIdx = i;
      }
    }

    return convertTbMoveToNyxptrMove(board, rm.moves[bestIdx].move);
  }
}