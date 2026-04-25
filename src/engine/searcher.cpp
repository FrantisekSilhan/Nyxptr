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

#include "Nyxptr/engine/searcher.h"
#include "Nyxptr/game/movegen.h"
#include "Nyxptr/game/types.h"
#include <cmath>
#include <algorithm>
#include <torch/cuda.h>

using namespace nyx::game;

namespace nyx::engine {
  Searcher::Searcher(const std::string& modelPath) : device(torch::cuda::is_available() ? torch::kCUDA : torch::kCPU) {
    model = torch::jit::load(modelPath);
    model.to(device);
    model.eval();
  }

  int Searcher::moveToIndex(const game::Move& m) {
    int from = to_i(m.getFrom());
    int to = to_i(m.getTo());
    uint16_t flags = m.getFlags();

    if (!(flags & game::Move::Promotion)) {
      return from * 64 + to;
    }

    // (ProjKnight (8) || CapturePromoN (12)) & 0x3 = 0
    // (ProjBishop (9) || CapturePromoB (13)) & 0x3 = 1
    // (ProjRook (10) || CapturePromoR (14)) & 0x3 = 2
    // (ProjQueen (11) || CapturePromoQ (15)) & 0x3 = 3
    uint16_t pieceType = flags & 0x3;

    switch (pieceType) {
      case 0: return 4096 + to;
      case 1: return 4096 + 64 + to;
      case 2: return 4096 + 128 + to;
      case 3: default: return from * 64 + to;
    }
  }

  game::Move Searcher::findBestMove(game::Board& board, int simulations) {
    auto root = std::make_unique<MCTSNode>(game::Move(), nullptr, 1.0f);

    std::vector<MCTSNode*> rootPath = {root.get()};
    expandAndEvaluate(root.get(), board, rootPath);

    for (int i = 0; i < simulations; ++i) {
      game::Board tempBoard = board;
      std::vector<MCTSNode*> path;

      MCTSNode* curr = root.get();
      path.push_back(curr);

      while (!curr->children.empty()) {
        curr = select(curr);
        tempBoard.makeMove(curr->move);
        path.push_back(curr);
      }

      expandAndEvaluate(curr, tempBoard, path);
    }

    if (root->children.empty()) return game::Move();

    auto bestIter = std::max_element(root->children.begin(), root->children.end(),
      [](const auto& a, const auto& b) {
        return a.second->visitCount < b.second->visitCount;
      });

    return bestIter->second->move;
  }

  MCTSNode* Searcher::select(MCTSNode* node) {
    float bestScore = -std::numeric_limits<float>::infinity();
    MCTSNode* bestChild = nullptr;

    float totalVisits = 0;
    for (const auto& [key, child] : node->children)
      totalVisits += child->visitCount;

    for (const auto& [key, child] : node->children) {
      float Q = child->getQ();
      float U = c_puct * child->prior * std::sqrt(totalVisits) / (1 + child->visitCount);

      float score = Q + U;
      if (score > bestScore) {
        bestScore = score;
        bestChild = child.get();
      }
    }

    return bestChild;
  }

  void Searcher::backpropagate(const std::vector<MCTSNode*>& path, float value) {
    float v = value;
    for (auto it = path.rbegin(); it != path.rend(); ++it) {
      (*it)->visitCount++;
      (*it)->valueSum += v;
      v = -v;
    }
  }

  void Searcher::expandAndEvaluate(MCTSNode* node, game::Board& board, std::vector<MCTSNode*> path) {
    auto legalMoves = MoveGen::generateMoves(board);
    MoveGen::filterLegalMoves(board, legalMoves);

    if (legalMoves.empty()) {
      float result = 0.0f;
      if (board.isCheck(board.getSideToMove())) {
        result = -1.0f;
      }

      backpropagate(path, result);
      return;
    }

    if (board.isDraw()) {
      backpropagate(path, 0.0f);
      return;
    }

    uint64_t boardKey = board.getZobristKey();
    
    std::vector<float> policyData;
    float value;

    auto ttIt = tt.find(boardKey);
    if (ttIt != tt.end()) {
      const Evaluation& eval = tt[boardKey];
      policyData = eval.policy;
      value = eval.value;
    } else {
      std::vector<float> tensorData = board.getFullStateTensor();
      torch::Tensor input = torch::from_blob(tensorData.data(), {1, 13, 8, 8}).to(device);

      std::vector<torch::jit::IValue> inputs;
      inputs.push_back(input);

      auto outputs = model.forward(inputs).toTuple();
      torch::Tensor policyTensor = outputs->elements()[0].toTensor().to(torch::kCPU);
      torch::Tensor valueTensor = outputs->elements()[1].toTensor().to(torch::kCPU);

      value = valueTensor.item<float>();
      
      policyData.assign(policyTensor.data_ptr<float>(), policyTensor.data_ptr<float>() + policyTensor.numel());

      tt[boardKey] = { policyData, value };
    }

    float probSum = 0.0f;
    for (const auto& m : legalMoves) {
      int idx = moveToIndex(m);
      float prob = policyData[idx];
      node->children[m.getRaw()] = std::make_unique<MCTSNode>(m, node, prob);
      probSum += prob;
    }

    if (probSum > 0) {
      for (auto& [key, child] : node->children) {
        child->prior /= probSum;
      }
    }

    backpropagate(path, value);
  }
}