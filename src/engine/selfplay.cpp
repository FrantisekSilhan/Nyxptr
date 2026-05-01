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

#include "Nyxptr/engine/selfplay.h"
#include "Nyxptr/engine/recorder.h"
#include "Nyxptr/game/board.h"
#include "Nyxptr/game/movegen.h"
#include <iostream>
#include <iomanip>
#include <atomic>

using namespace nyx::game;
using namespace nyx::engine;

extern std::atomic<bool> running;

namespace nyx::engine {
  void SelfPlay::runSelfPlay(Searcher& searcher, int numGames, int simsPerMove, const std::string& outputFile) {
    DataRecorder recorder(outputFile);
    searcher.setCPuct(4.0f);
    searcher.setFpuReduction(0.4f);
    searcher.setDrawPenalty(-0.1f);

    for (int g = 0; g < numGames && running; ++g) {
      Board board;
      int movesInGame = 0;
      std::string gameMoves = "";

      std::cout << "\nStarting Self-Play Game " << (g + 1) << "/" << numGames << std::endl;


      while (running) {
        std::vector<Move> legalMoves = MoveGen::generateMoves(board);
        MoveGen::filterLegalMoves(board, legalMoves);

        if (legalMoves.empty() || board.isDraw()) {
          float result = 0.0f;
          std::string resultStr = "Draw";
          if (legalMoves.empty() && board.isCheck(board.getSideToMove())) {
            result = -1.0f;
            resultStr = (board.getSideToMove() == Color::White) ? "Black Won" : "White Won";
          }

          recorder.finishGame(result);
          std::cout << "\rGame " << (g + 1) << " Finished. Total Moves: " << movesInGame 
                    << " Result: " << resultStr << "                      " << std::endl;
          // std::cout << "Move History: " << gameMoves << std::endl;
          break;
        }

        searcher.clearCache();
        auto [bestMove, distribution] = searcher.getBestMoveAndDistribution(board, simsPerMove);

        if (bestMove.isNone()) {
          recorder.finishGame(0.0f);
          break;
        }

        /*std::string moveStr = bestMove.toAlgebraic();
        gameMoves += moveStr + " ";

        std::cout << "\r> Move " << std::setw(3) << movesInGame + 1 
                  << " | Side: " << (board.getSideToMove() == Color::White ? "W" : "B")
                  << " | AI Choice: " << moveStr << std::flush;*/

        recorder.recordStep(board, distribution);
        board.makeMove(bestMove);
        movesInGame++;
      }
    }

    std::cout << "\nSelf-play session complete." << std::endl;
  };
}