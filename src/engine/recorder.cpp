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
    fileStream.write(reinterpret_cast<const char*>(step.stateTensor.data()), step.stateTensor.size() * sizeof(float));
    fileStream.write(reinterpret_cast<const char*>(step.policyTarget.data()), step.policyTarget.size() * sizeof(float));
    fileStream.write(reinterpret_cast<const char*>(&result), sizeof(float));
  }
}