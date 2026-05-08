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

#include "Nyxptr/engine/recorder.h"
#include <iostream>
#include "Nyxptr/engine/searcher.h"

namespace nyx::engine {
  DataRecorder::DataRecorder(const std::string& filename) : outputPath(filename) {
    fileStream.open(outputPath, std::ios::binary | std::ios::app);
  }

  DataRecorder::~DataRecorder() {
    if (fileStream.is_open()) fileStream.close();
  }

  void DataRecorder::recordStep(const game::Board& board, const std::vector<float>& policyDistribution) {
    TrainingStep step;
    step.stateTensor = board.getFullStateTensor();
    step.policyTarget = policyDistribution;
    gameBuffer.push_back(step);
  }

  void DataRecorder::finishGame(float finalResult) {
    if (!fileStream.is_open()) return;

    float currentResult = finalResult;
    for (auto it = gameBuffer.rbegin(); it != gameBuffer.rend(); ++it) {
      writeToDisk(*it, currentResult);
      currentResult = -currentResult;
    }

    gameBuffer.clear();
    fileStream.flush();
  }

  void DataRecorder::writeToDisk(const TrainingStep& step, float result) {
    for (int p = 0; p < 13; ++p) {
      uint64_t packed = 0;
      for (int sq = 0; sq < 64; ++sq) {
        if (step.stateTensor[p * 64 + sq] > 0.5f) packed |= (1ULL << sq);
      }
      fileStream.write(reinterpret_cast<const char*>(&packed), sizeof(uint64_t));
    }
    fileStream.write(reinterpret_cast<const char*>(step.policyTarget.data()), step.policyTarget.size() * sizeof(float));
    fileStream.write(reinterpret_cast<const char*>(&result), sizeof(float));
  }

  PRecorder::PRecorder(const std::string& filename) : outputPath(filename) {
    fileStream.open(outputPath, std::ios::binary | std::ios::app);
  }

  PRecorder::~PRecorder() {
    if (fileStream.is_open()) fileStream.close();
  }

  void PRecorder::recordStep(const game::Board& board, game::Move bestMove) {
    PStep step;

    std::vector<float> floats = board.getFullStateTensor();

    for (int p = 0; p < 13; ++p) {
      uint64_t packed = 0;
      for (int sq = 0; sq < 64; ++sq) {
        if (floats[p *  64 + sq] > 0.5f) {
          packed |= (1ULL << sq);
        }
      }
      step.planes[p] = packed;
    }

    step.moveIndex = static_cast<uint16_t>(Searcher::moveToIndex(bestMove));

    gameBuffer.push_back(step);
  }

  void PRecorder::finishGame(float finalResult) {
    if (!fileStream.is_open()) return;

    float currentResult = finalResult;
    for (auto it = gameBuffer.rbegin(); it != gameBuffer.rend(); ++it) {
      writeToDisk(*it, currentResult);
      currentResult = -currentResult;
    }

    gameBuffer.clear();
    fileStream.flush();
  }

  void PRecorder::finishPuzzle(float result) {
    if (!fileStream.is_open()) return;

    for (const auto& step : gameBuffer) {
      writeToDisk(step, result);
    }
    
    gameBuffer.clear();
    fileStream.flush();
  }

  void PRecorder::writeToDisk(const PStep& step, float result) {
    fileStream.write(reinterpret_cast<const char*>(step.planes), 13 * sizeof(uint64_t));
    fileStream.write(reinterpret_cast<const char*>(&step.moveIndex), sizeof(uint16_t));
    int16_t pRes = static_cast<int16_t>(result * 100);
    fileStream.write(reinterpret_cast<const char*>(&pRes), sizeof(int16_t));
  }
}