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
#include <array>
#include <cstdint>
#include "Nyxptr/game/bits.h"

namespace nyx::game::lookups {
  struct MagicValues {
    uint64_t magic;
    uint8_t bits;
  };
  struct Magic {
    uint64_t mask;
    uint64_t magic;
    uint32_t shift;
    uint64_t* ptr;
  };

  inline Magic rookMagics[64];
  inline Magic bishopMagics[64];
  inline uint64_t sliderDatabase[107648];

  inline uint32_t getMagicIndex(const Magic& m, uint64_t occupancy) {
    return static_cast<uint32_t>(((occupancy & m.mask) * m.magic) >> m.shift);
  }

  void initSliderTables();

  constexpr std::array<uint64_t, 64> initKnightTable() {
    std::array<uint64_t, 64> table{};
    for (int i = 0; i < 64; ++i) {
      uint64_t b = 1ULL << i;
      uint64_t k = 0;
      if (b & ~0x0101010101010101ULL) {
        k |= (b << 15) | (b >> 17);
        if (b & ~0x0202020202020202ULL) {
          k |= (b << 6) | (b >> 10);
        }
      }
      if (b & ~0x8080808080808080ULL) {
        k |= (b << 17) | (b >> 15);
        if (b & ~0x4040404040404040ULL) {
          k |= (b << 10) | (b >> 6);
        }
      }
      table[i] = k;
    }
    return table;
  }

  constexpr std::array<uint64_t, 64> initKingTable() {
    std::array<uint64_t, 64> table{};
    for (int i = 0; i < 64; ++i) {
      uint64_t b = 1ULL << i;
      uint64_t kg = 0;
      kg |= (b << 8) | (b >> 8);
      if (b & ~0x0101010101010101ULL) {
        kg |= (b << 7) | (b >> 1) | (b >> 9);
      }
      if (b & ~0x8080808080808080ULL) {
        kg |= (b << 9) | (b << 1) | (b >> 7);
      }
      table[i] = kg;
    }
    return table;
  }

  inline constexpr std::array<uint64_t, 64> knightTable = initKnightTable();
  inline constexpr std::array<uint64_t, 64> kingTable = initKingTable();
}