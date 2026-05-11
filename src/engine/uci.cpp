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

#include "Nyxptr/engine/uci.h"
#include <iostream>
#include <iomanip>
#include <vector>
#include <csignal>
#include "Nyxptr/game/board.h"
#include "Nyxptr/game/lookups.h"
#include "Nyxptr/game/movegen.h"
#include "Nyxptr/engine/searcher.h"
#include "Nyxptr/engine/recorder.h"
#include "Nyxptr/engine/converter.h"

using namespace nyx::game;

std::vector<std::string> split(const std::string& s) {
  std::vector<std::string> tokens;
  std::string token;
  std::istringstream tokenSteam(s);
  while (std::getline(tokenSteam, token, ' ')) {
    if (!token.empty()) tokens.push_back(token);
  }
  return tokens;
};

namespace nyx::engine {
  void UCI::loop(Searcher& searcher, int simulations) {
    Board board;
    std::string line;

    while (std::getline(std::cin, line)) {
      std::vector<std::string> tokens = split(line);
      if (tokens.empty()) continue;

      std::string cmd = tokens[0];
      
      if (cmd == "uci") {
        std::cout << "id name Nyxptr v0.0.6" << std::endl;
        std::cout << "id author https://github.com/FrantisekSilhan/Nyxptr/blob/main/AUTHORS" << std::endl;
        std::cout << "uciok" << std::endl;
      } else if (cmd == "isready") {
        std::cout << "readyok" << std::endl;
      } else if (cmd == "ucinewgame") {
        board = Board();
        searcher.clearCache();
      } else if (cmd == "position") {
        searcher.clearCache();
        if (tokens.size() <= 1) continue;
        std::string option = tokens[1];
        if (option == "startpos") {
          board = Board();
          size_t mIdx = 0;
          for (size_t i = 2; i < tokens.size(); ++i) {
            if (tokens[i] == "moves") {
              mIdx = i + 1;
              break;
            }
          }
          for (size_t i = mIdx; i < tokens.size(); ++i) {
            std::vector<Move> moves = MoveGen::generateMoves(board);
            MoveGen::filterLegalMoves(board, moves);
            bool moveApplied = false;
            for (const Move& m : moves) {
              if (m.toAlgebraic() == tokens[i]) {
                if (board.makeMove(m)) {
                  moveApplied = true;
                }
                break;
              }
            }

            if (!moveApplied) {
              std::cout << "info string invalid position move: " << tokens[i] << std::endl;
              break;
            }
          }
        } else if (option == "fen") {
          board = Board();
          if (tokens.size() <= 7) continue;
          std::string fen = tokens[2] + " " + tokens[3] + " " + tokens[4] + " " + tokens[5] + " " + tokens[6] + " " + tokens[7];
          board.loadFEN(fen);
          size_t mIdx = 0;
          for (size_t i = 8; i < tokens.size(); ++i) {
            if (tokens[i] == "moves") {
              mIdx = i + 1;
              break;
            }
          }
          for (size_t i = mIdx; i < tokens.size(); ++i) {
            std::vector<Move> moves = MoveGen::generateMoves(board);
            MoveGen::filterLegalMoves(board, moves);
            bool moveApplied = false;
            for (const Move& m : moves) {
              if (m.toAlgebraic() == tokens[i]) {
                if (board.makeMove(m)) {
                  moveApplied = true;
                }
                break;
              }
            }

            if (!moveApplied) {
              std::cout << "info string invalid position move: " << tokens[i] << std::endl;
              break;
            }
          }
        }
      } else if (cmd == "go") {
        searcher.clearCache();
        Move bestMove = searcher.findBestMove(board, simulations);
        std::cout << "bestmove " << (bestMove.isNone() ? "0000" : bestMove.toAlgebraic()) << std::endl;
      } else if (cmd == "quit") {
        break;
      }
    }
  }
}