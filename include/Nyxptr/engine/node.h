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
#include <vector>
#include <map>
#include <memory>
#include "Nyxptr/game/move.h"

namespace nyx::engine {
  struct MCTSNode {
    game::Move move;
    MCTSNode* parent;
    std::map<uint64_t, std::unique_ptr<MCTSNode>> children;

    int visitCount = 0;
    float valueSum = 0.0f;
    float prior = 0.0f;
    int virtualLoss = 0;

    bool isTerminal = false;
    float terminalValue = 0.0f;

    MCTSNode(game::Move m, MCTSNode* p, float pr) : move(m), parent(p), prior(pr) {}

    float getQ() const {
      int effectiveVisits = visitCount + virtualLoss;
      if (effectiveVisits == 0) return 0.0f;
      return (valueSum - virtualLoss) / effectiveVisits;
    }
  };
}