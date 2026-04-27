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
#include <string>
#include <vector>
#include "Nyxptr/game/board.h"
#include "Nyxptr/engine/recorder.h"

namespace nyx::engine {
  class DataConverter {
    public:
      static void convertPGNToBinary(const std::string& pgnPath, const std::string& binPath);

    private:
      static game::Move findMoveInPGN(game::Board& board, std::string san);
      static char getPieceChar(uint8_t piece);
  };
}