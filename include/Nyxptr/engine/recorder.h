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
#include <string>
#include <fstream>
#include "Nyxptr/game/board.h"
#include "Nyxptr/game/move.h"

namespace nyx::engine {
  struct TrainingStep {
    std::vector<float> stateTensor;
    std::vector<float> policyTarget;
  };

  class DataRecorder {
    public:
      DataRecorder(const std::string& filename);
      ~DataRecorder();

      void recordStep(const game::Board& board, const std::vector<float>& visitCounts);
      void finishGame(float finalResult);

    private:
      std::string outputPath;
      std::vector<TrainingStep> gameBuffer;
      std::ofstream fileStream;

      void writeToDisk(const TrainingStep& step, float result);
  };

  class PRecorder {
    public:
      PRecorder(const std::string& filename);
      ~PRecorder();

      void recordStep(const game::Board& board, game::Move bestMove);
      void finishGame(float finalResult);
      void finishPuzzle(float result);

    private:
      struct PStep {
        uint64_t planes[13];
        uint16_t moveIndex;
      };

      std::string outputPath;
      std::ofstream fileStream;
      std::vector<PStep> gameBuffer;

      void writeToDisk(const PStep& step, float result);
  };
}