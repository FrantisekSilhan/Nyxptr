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

#pragma once
#include <cstdint>
#include "Nyxptr/game/types.h"
#include "Nyxptr/game/move.h"
#include <vector>

namespace nyx::game {
  struct UndoState {
    uint8_t castlingRights;
    Square enPassantSquare;
    int halfMoveClock;
    uint64_t zobristKey;
    Piece capturedPiece;
    Move lastMove;
  };

  class Board {
    private:
      uint64_t bitboards[2][6];
      uint64_t colorOccupancy[2];
      uint64_t combinedOccupancy;

      Piece boardArray[64];
      Color colorArray[64];

      Color sideToMove;
      uint8_t castlingRights;
      Square enPassantSquare;
      Square kingSquare[2];
      int halfMoveClock;
      int fullMoveNumber;

      uint64_t zobristKey;
      std::vector<UndoState> history;

      static uint64_t pieceKeys[2][6][64];
      static uint64_t sideKey;
      static uint64_t castlingKeys[16];
      static uint64_t enPassantKeys[64];

      void addPiece(Square sq, Piece p, Color c);
      void removePiece(Square sq, Piece p, Color c);

    public:
      Board();
      static void initZobrist();

      uint64_t getAttacksTo(Square sq, Color side) const;

      bool isSquareAttacked(Square sq, Color attackerColor) const;

      bool isCheck(Color side) const {
        return isSquareAttacked(kingSquare[to_i(side)], !side);
      }

      std::vector<float> getFullStateTensor() const;

      bool makeMove(Move m);
      void undoMove();

      inline Color getSideToMove() const { return sideToMove; }
      inline uint64_t getOccupancy(Color c) const { return colorOccupancy[static_cast<int>(c)]; }
      inline uint64_t getCombinedOccupancy() const { return combinedOccupancy; }
      inline uint64_t getBitboard(Color c, Piece p) const { return bitboards[static_cast<int>(c)][static_cast<int>(p)]; }
      inline Piece getPieceAt(Square sq) const { return boardArray[static_cast<int>(sq)]; }
      inline Color getColorAt(Square sq) const { return colorArray[static_cast<int>(sq)]; }
      inline uint8_t getCastlingRights() const { return castlingRights; }
      inline Square getEnPassantSquare() const { return enPassantSquare; }
  };
}