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

#include "Nyxptr/game/move.h"
#include <array>

namespace nyx::game {
  std::string Move::toAlgebraic() const {
    if (isNone()) return "none";

    static constexpr std::array<const char*, 64> squareNames = {
      "a1", "b1", "c1", "d1", "e1", "f1", "g1", "h1",
      "a2", "b2", "c2", "d2", "e2", "f2", "g2", "h2",
      "a3", "b3", "c3", "d3", "e3", "f3", "g3", "h3",
      "a4", "b4", "c4", "d4", "e4", "f4", "g4", "h4",
      "a5", "b5", "c5", "d5", "e5", "f5", "g5", "h5",
      "a6", "b6", "c6", "d6", "e6", "f6", "g6", "h6",
      "a7", "b7", "c7", "d7", "e7", "f7", "g7", "h7",
      "a8", "b8", "c8", "d8", "e8", "f8", "g8", "h8"
    };

    std::string moveStr = squareNames[getFrom()];
    moveStr += squareNames[getTo()];

    uint16_t flags = getFlags();
    if (flags & Promotion) {
      switch (flags) {
        case ProjKnight: case (ProjKnight | Capture): moveStr += 'n'; break;
        case ProjBishop: case (ProjBishop | Capture): moveStr += 'b'; break;
        case ProjRook:   case (ProjRook | Capture):   moveStr += 'r'; break;
        case ProjQueen:  case (ProjQueen | Capture):  moveStr += 'q'; break;
        default: break;
      }
    }

    return moveStr;
  }
}