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
#include "Nyxptr/engine/selfplay.h"
#include "Nyxptr/engine/searcher.h"
#include "Nyxptr/game/lookups.h"
#include "Nyxptr/game/board.h"
#include "Nyxptr/engine/converter.h"
#include <string>
#include <atomic>
#include <filesystem>

using namespace nyx::engine;

std::atomic<bool> running(true);

int main(int argc, char* argv[]) {
  nyx::game::lookups::initSliderTables();
  nyx::game::Board::initZobrist();

  std::string MODEL_PATH = "model/nyxptr_v3_epoch3.pt";
  Searcher searcher(MODEL_PATH);
  int simulations = 10000;

  if (argc > 1) {
    std::string mode = argv[1];
    if (mode == "--convert") {
      if (argc < 4) {
        std::cerr << "Usage: " << argv[0] << " --convert pgn_directory bin_directory" << std::endl;
        return 1;
      }
      std::string pgnDir = argv[2];
      std::string binDir = argv[3];
      for (const auto& entry : std::filesystem::directory_iterator(pgnDir)) {
        if (entry.path().extension() == ".pgn") {
          std::filesystem::path pgnPath = entry.path();
          std::filesystem::path binPath = binDir / pgnPath.stem();
          binPath.replace_extension(".bin");
          std::cout << "Converting " << pgnPath.string() << " to " << binPath.string() << std::endl;
          DataConverter::convertPGNToBinary(pgnPath.string(), binPath.string());
        }
      }
       return 0;
    }
    if (mode == "--selfplay") {
      int games = (argc > 2) ? std::stoi(argv[2]) : 50;
      int sims = (argc > 3) ? std::stoi(argv[3]) : 800;
      std::string outputFile = (argc > 4) ? argv[4] : "selfplay_data.bin";
      SelfPlay::runSelfPlay(searcher, games, sims, outputFile);
    } else if (mode == "--uci") {
      UCI::loop(searcher, simulations);
    } else {
      std::cerr << "Args: " << argv[0] << " [--selfplay numGames simsPerMove] | [--uci]" << std::endl;
      return 1;
    }
  } else {
    UCI::loop(searcher, simulations);
  }
}