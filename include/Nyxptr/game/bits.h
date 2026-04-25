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
#include <bit>
#include <cstdint>
#include "Nyxptr/game/types.h"

namespace nyx::bitboard {
  inline int getLSB(uint64_t bb) {
    return std::countr_zero(bb);
  }

  inline game::Square getLSBSquare(uint64_t bb) {
    return static_cast<game::Square>(getLSB(bb));
  }

  inline int getMSB(uint64_t bb) {
    return 63 - std::countl_zero(bb);
  }

  inline int popLSB(uint64_t& bb) {
    int lsb = getLSB(bb);
    bb &= bb - 1;
    return lsb;
  }

  inline game::Square popLSBSquare(uint64_t& bb) {
    return static_cast<game::Square>(popLSB(bb));
  }

  inline int countBits(uint64_t bb) {
    return std::popcount(bb);
  }
}