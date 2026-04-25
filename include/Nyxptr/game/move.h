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
#include <string>
#include "Nyxptr/game/types.h"

namespace nyx::game {
  /**
   * Move bits:
   * 0-5:   From square (0-63)
   * 6-11:  To square (0-63)
   * 12-15: Flags (Promotion, Capture, Special)
   */

  class Move {
    public:
      using MoveData = uint16_t;

      enum Flags : uint16_t {
        Quiet           = 0,
        DoublePawnPush  = 1,
        KingCastle      = 2,
        QueenCastle     = 3,
        EnPassant       = 5,
        
        Capture         = 4,
        PawnCapture     = 4, 

        Promotion       = 8,
        ProjKnight      = 8,
        ProjBishop      = 9,
        ProjRook        = 10,
        ProjQueen       = 11,
        
        CapturePromoN   = 12,
        CapturePromoB   = 13,
        CapturePromoR   = 14,
        CapturePromoQ   = 15
      };

      constexpr Move() : data(0) {}
      constexpr Move(Square from, Square to, Flags flags) : data(static_cast<uint16_t>(static_cast<uint8_t>(from) | (static_cast<uint8_t>(to) << 6) | (static_cast<uint16_t>(flags) << 12))) {}

      constexpr uint8_t getFrom() const { return data & 0x3F; }
      constexpr uint8_t getTo() const { return (data >> 6) & 0x3F; }
      constexpr uint16_t getFlags() const { return (data >> 12); }

      constexpr bool isNone() const { return data == 0; }
      constexpr bool operator==(const Move& other) const { return data == other.data; }

      std::string toAlgebraic() const;

    private:
      MoveData data;
  };

  inline constexpr Move::Flags operator|(Move::Flags a, Move::Flags b) {
    return static_cast<Move::Flags>(static_cast<uint16_t>(a) | static_cast<uint16_t>(b));
  };

  inline constexpr Move::Flags operator|=(Move::Flags& a, Move::Flags b) {
    return a = a | b;
  };
}