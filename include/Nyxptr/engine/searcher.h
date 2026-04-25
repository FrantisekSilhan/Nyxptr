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
#include <torch/script.h>
#include <memory>
#include <vector>
#include <string>
#include "Nyxptr/game/board.h"
#include "Nyxptr/game/move.h"
#include "Nyxptr/engine/node.h"

namespace nyx::engine {
  struct Evaluation {
    std::vector<float> policy;
    float value;
  };
  inline std::unordered_map<uint64_t, Evaluation> tt;
  class Searcher {
    public:
      explicit Searcher(const std::string& modelPath);
      game::Move findBestMove(game::Board& board, int simulations);
      void clearCache() { tt.clear(); }
    private:
      MCTSNode* select(MCTSNode* node);
      void expandAndEvaluate(MCTSNode* node, game::Board& board, std::vector<MCTSNode*> path);
      void backpropagate(const std::vector<MCTSNode*>& path, float value);

      int moveToIndex(const game::Move& m);

      torch::jit::script::Module model;
      torch::Device device;

      const float c_puct = 2.0f;
  };
}