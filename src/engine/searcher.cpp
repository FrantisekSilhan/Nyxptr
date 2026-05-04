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
#include "Nyxptr/engine/syzygy.h"
#include "Nyxptr/game/movegen.h"
#include "Nyxptr/game/types.h"
#include <cmath>
#include <algorithm>
#include <torch/cuda.h>
#include <random>

using namespace nyx::game;

namespace nyx::engine {
  namespace {
    constexpr unsigned kTbResultFailed = 0xFFFFFFFFu;
    constexpr unsigned kTbLoss = 0;
    constexpr unsigned kTbBlessedLoss = 1;
    constexpr unsigned kTbDraw = 2;
    constexpr unsigned kTbCursedWin = 3;
    constexpr unsigned kTbWin = 4;

    float wdlToValue(unsigned wdl) {
      switch (wdl) {
        case kTbLoss: return -1.0f;
        case kTbBlessedLoss: return -0.9f;
        case kTbDraw: return 0.0f;
        case kTbCursedWin: return 0.9f;
        case kTbWin: return 1.0f;
        default: return 0.0f;
      }
    }

    void fillOneHotDistribution(std::vector<float>& distribution, const game::Move& move) {
      if (move.isNone()) {
        return;
      }

      const int from = game::to_i(move.getFrom());
      const int to = game::to_i(move.getTo());
      const uint16_t flags = move.getFlags();

      int idx = 0;
      if (!(flags & game::Move::Promotion)) {
        idx = from * 64 + to;
      } else {
        const uint16_t pieceType = flags & 0x3;
        switch (pieceType) {
          case 0: idx = 4096 + to; break;
          case 1: idx = 4096 + 64 + to; break;
          case 2: idx = 4096 + 128 + to; break;
          case 3: idx = 4096 + 192 + to; break;
          default: idx = from * 64 + to; break;
        }
      }

      if (idx >= 0 && static_cast<size_t>(idx) < distribution.size()) {
        distribution[idx] = 1.0f;
      }
    }
  }

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
      case 3: return 4096 + 192 + to;
      default: return from * 64 + to;
    }
  }

  game::Move Searcher::findBestMove(game::Board& board, int simulations) {
    if (Syzygy::canProbe(board)) {
      game::Move tbMove = Syzygy::probeDtz(board);
      if (!tbMove.isNone()) return tbMove;
    }

    auto root = std::make_unique<MCTSNode>(game::Move(), nullptr, 1.0f);
    std::vector<MCTSNode*> rootPath = {root.get()};
    expandAndEvaluate(root.get(), board, rootPath);

    int maxDepth = 0;
    auto startTime = std::chrono::high_resolution_clock::now();

    for (int i = 0; i < simulations; ++i) {
      game::Board tempBoard = board;
      std::vector<MCTSNode*> path;

      MCTSNode* curr = root.get();
      path.push_back(curr);
      bool playoutValid = true;

      while (!curr->children.empty() && !curr->isTerminal) {
        curr = select(curr);
        if (!tempBoard.makeMove(curr->move)) {
          playoutValid = false;
          break;
        }
        path.push_back(curr);
      }

      if (!playoutValid) {
        continue;
      }

      if (static_cast<int>(path.size()) > maxDepth) maxDepth = path.size();
      expandAndEvaluate(curr, tempBoard, path);

      if (i > 0 && i % 100 == 0) {
        auto now = std::chrono::high_resolution_clock::now();
        auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now - startTime).count();
        float nps = (ms > 0) ? (i * 1000.0f / ms) : 0.0f;

        auto bestSoFar = std::max_element(root->children.begin(), root->children.end(),
          [](const auto& a, const auto& b) {
            return a.second->visitCount < b.second->visitCount;
          });

        int cpScore = static_cast<int>(root->getQ() * 1000.0f);
        std::cout << "info depth " << maxDepth
                  << " nodes " << i
                  << " nps " << static_cast<int>(nps)
                  << " score cp " << cpScore
                  << " pv " << bestSoFar->second->move.toAlgebraic() << std::endl;
      }
    }

    if (root->children.empty()) return game::Move();

    MCTSNode* bestChild = nullptr;
    int maxVisits = -1;

    for (auto& [key, child] : root->children) {
      if (child->isTerminal && child->terminalValue > 0.99f) {
        return child->move;
      }

      if (child->visitCount > maxVisits) {
        maxVisits = child->visitCount;
        bestChild = child.get();
      }
    }

    return bestChild ? bestChild->move : game::Move();
  }

  game::Move Searcher::selectMoveProportionally(MCTSNode* root) {
    static std::mt19937 gen(std::random_device{}());

    std::vector<float> visits;
    std::vector<game::Move> moves;
    float totalVisits = 0.0f;

    for (const auto& [key, child] : root->children) {
      visits.push_back(static_cast<float>(child->visitCount));
      moves.push_back(child->move);
      totalVisits += child->visitCount;
    }

    if (totalVisits == 0.0f) return game::Move();

    std::discrete_distribution<> dist(visits.begin(), visits.end());
    return moves[dist(gen)];
  }

  std::pair<game::Move, std::vector<float>> Searcher::getBestMoveAndDistribution(game::Board& board, int simulations) {
    if (Syzygy::canProbe(board)) {
      game::Move tbMove = Syzygy::probeDtz(board);
      std::vector<float> distribution(4352, 0.0f);
      fillOneHotDistribution(distribution, tbMove);
      std::cout << "info string tb dtz root_move " << tbMove.toAlgebraic() << std::endl;
      return {tbMove, distribution};
    }

    auto root = std::make_unique<MCTSNode>(game::Move(), nullptr, 1.0f);
    std::vector<MCTSNode*> rootPath = {root.get()};
    expandAndEvaluate(root.get(), board, rootPath);

    if (!root->children.empty()) {
      float epsilon = 0.25f;
      float alpha = 0.3f;

      static std::mt19937 gen(std::random_device{}());
      std::gamma_distribution<float> dist(alpha, 1.0f);

      std::vector<float> noise(root->children.size());
      float noiseSum = 0.0f;
      for (float& n : noise) {
        n = dist(gen);
        noiseSum += n;
      }

      int i = 0;
      for (auto& [key, child] : root->children) {
        float noiseVal = noise[i++] / noiseSum;
        child->prior = (1.0f - epsilon) * child->prior + epsilon * noiseVal;
      }
    }

    for (int i = 0; i < simulations; ++i) {
      game::Board tempBoard = board;
      std::vector<MCTSNode*> path;

      MCTSNode* curr = root.get();
      path.push_back(curr);
      bool playoutValid = true;

      while (!curr->children.empty()) {
        curr = select(curr);
        if (!tempBoard.makeMove(curr->move)) {
          playoutValid = false;
          break;
        }
        path.push_back(curr);
      }

      if (!playoutValid) {
        continue;
      }

      expandAndEvaluate(curr, tempBoard, path);
    }

    std::vector<float> distribution(4352, 0.0f);
    float totalVisits = 0;

    for (const auto& [key, child] : root->children) {
      totalVisits += child->visitCount;
    }

    for (const auto& [key, child] : root->children) {
      int idx = moveToIndex(child->move);
      float prob = (totalVisits > 0) ? (child->visitCount / totalVisits) : 0.0f;
      distribution[idx] = prob;
    }

    game::Move selectedMove = selectMoveProportionally(root.get());

    return {selectedMove, distribution};
  }

  MCTSNode* Searcher::select(MCTSNode* node) {
    if (node->isTerminal) return nullptr;
    float bestScore = -std::numeric_limits<float>::infinity();
    MCTSNode* bestChild = nullptr;

    float totalVisits = 0;
    for (const auto& [key, child] : node->children)
      totalVisits += child->visitCount;

    float fpuValue = node->getQ() - fpuReduction;

    for (const auto& [key, child] : node->children) {
      float Q = (child->visitCount > 0) ? child->getQ() : fpuValue;
      float U = cPuct * child->prior * std::sqrt(totalVisits + 1.0f) / (1 + child->visitCount);

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
    if (Syzygy::canProbe(board)) {
      unsigned wdl = Syzygy::probeWdl(board);
      if (wdl != kTbResultFailed) {
        float value = wdlToValue(wdl);

        backpropagate(path, value);

        node->isTerminal = true;
        node->terminalValue = value;
        return;
      }
    }

    auto legalMoves = MoveGen::generateMoves(board);
    MoveGen::filterLegalMoves(board, legalMoves);

    if (legalMoves.empty()) {
      float result = 0.0f;
      if (board.isCheck(board.getSideToMove())) {
        result = -1.0f;
      }

      if (result == 0.0f) {
        result = drawPenalty;
      }

      backpropagate(path, result);
      return;
    }

    if (board.isDraw()) {
      backpropagate(path, drawPenalty);
      return;
    }

    uint64_t boardKey = board.getZobristKey();
    
    std::vector<float> policyData;
    float value;

    auto ttIt = tt.find(boardKey);
    if (ttIt != tt.end()) {
      const Evaluation& eval = ttIt->second;
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
      float rawProb = 1.0f;
      if (idx >= 0 && static_cast<size_t>(idx) < policyData.size()) {
        float logProb = policyData[idx];
        rawProb = std::exp(logProb);
      }
      float flattenedProb = std::pow(rawProb, 1.0f / policyTemp);
      node->children[m.getRaw()] = std::make_unique<MCTSNode>(m, node, flattenedProb);
      probSum += flattenedProb;
    }

    if (probSum > 0) {
      for (auto& [key, child] : node->children) {
        child->prior /= probSum;
      }
    }

    backpropagate(path, value);
  }
}