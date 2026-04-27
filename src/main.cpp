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

#include <torch/script.h>
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
using namespace nyx::engine;

std::atomic<bool> running(true);

void signalHandler([[maybe_unused]] int signum) {
  std::cout << "\n[Interrupt] Gracefully shutting down..." << std::endl;
  running = false;
}

void printBoard(const Board& board) {
  std::cout << "\n  +---+---+---+---+---+---+---+---+\n";
  for (int r = 7; r >= 0; --r) {
    std::cout << r + 1 << " | ";
    for (int c = 0; c < 8; ++c) {
      Square sq = static_cast<Square>(r * 8 + c);
      Piece p = board.getPieceAt(sq);
      Color col = board.getColorAt(sq);

      if (p == Piece::None) {
        std::cout << "  ";
      } else {
        char pChar;
        switch (p) {
          case Piece::Pawn:   pChar = 'P'; break;
          case Piece::Knight: pChar = 'N'; break;
          case Piece::Bishop: pChar = 'B'; break;
          case Piece::Rook:   pChar = 'R'; break;
          case Piece::Queen:  pChar = 'Q'; break;
          case Piece::King:   pChar = 'K'; break;
          default:            pChar = '?'; break;
        }
        if (col == Color::Black) pChar = static_cast<char>(tolower(pChar));
        std::cout << pChar << " ";
      }
      std::cout << "| ";
    }
    std::cout << "\n  +---+---+---+---+---+---+---+---+\n";
  }
  std::cout << "    a   b   c   d   e   f   g   h\n\n";
  std::cout << "Side to move: " << (board.getSideToMove() == Color::White ? "White" : "Black") << "\n";
}

void runSelfPlay(Searcher& searcher, int numGames, int simsPerMove) {
  const std::string filename = "training_data.bin";
  DataRecorder recorder(filename);
  
  for (int g = 0; g < numGames; ++g) {
    Board board;
    int movesInGame = 0;

    std::cout << "Starting Game " << (g + 1) << "/" << numGames << std::endl;

    while (running) {
      auto legalMoves = MoveGen::generateMoves(board);
      MoveGen::filterLegalMoves(board, legalMoves);

      if (legalMoves.empty() || board.isDraw()) {
        float result = 0.0f;
        if (legalMoves.empty() && board.isCheck(board.getSideToMove())) {
          result = -1.0f;
        }

        recorder.finishGame(result);
        std::cout << "\rGame " << (g + 1) << " Finished. Moves: " << movesInGame
                  << " Result: " << result << "                      " << std::endl;
        break;
      }

      std::cout << "\r> Move " << std::setw(3) << movesInGame + 1 
                << " | Side: " << (board.getSideToMove() == Color::White ? "W" : "B")
                << " | Last Move: " << std::setw(5) << (movesInGame > 0 ? "..." : "None")
                << std::flush;

      searcher.clearCache();
      auto [bestMove, distribution] = searcher.getBestMoveAndDistribution(board, simsPerMove);

      recorder.recordStep(board, distribution);

      if (bestMove.isNone()) {
        std::cerr << "\nError: MCTS failed to find a move!" << std::endl;
        float res = 0.0f;
        if (board.isCheck(board.getSideToMove())) res = -1.0f;
        recorder.finishGame(res);
        std::cout << "\rGame " << (g + 1) << " Finished via search.                      " << std::endl;
        break;
      }

      if (board.makeMove(bestMove)) {
        movesInGame++;
      } else {
        std::cerr << "Error: AI tried to play illegal move: " << bestMove.toAlgebraic() << std::endl;
        recorder.finishGame(0.0f); // Treat as draw
        break; 
      }
    }
  }

  std::cout << "\nSelf-play session complete. Data saved to " << filename << std::endl;
}

int main(int argc, char* argv[]) {
  std::string MODEL_PATH = "model/nyxptr_v1.pt";
  std::signal(SIGINT, signalHandler);

  std::cout << "========================================" << std::endl;
  std::cout << "      Nyxptr Chess Engine v0.0.1        " << std::endl;
  std::cout << "========================================" << std::endl;

  std::cout << "[1/3] Initializing lookup tables..." << std::endl;
  lookups::initSliderTables();

  std::cout << "[2/3] Initializing Zobrist keys..." << std::endl;
  Board::initZobrist();

  if (argc > 1 && std::string(argv[1]) == "--convert") {
    if (argc < 4) {
      std::cout << "Usage: --convert <input.pgn> <output.bin>" << std::endl;
      return 1;
    }
    std::string pgnPath = argv[2];
    std::string binPath = argv[3];
    std::cout << "Converting PGN: " << pgnPath << " to Binary: " << binPath << std::endl;
    DataConverter::convertPGNToBinary(pgnPath, binPath);
    return 0;
  }

  std::cout << "[3/3] Loading Neural Network..." << std::endl;
  std::unique_ptr<Searcher> searcher;
  try {
    searcher = std::make_unique<Searcher>(MODEL_PATH);
    std::cout << "Neural Network loaded successfully!" << std::endl;
  } catch (const std::exception& e) {
    std::cerr << "CRITICAL ERROR: Could not load model: " << e.what() << std::endl;
    return 1;
  }

  if (argc > 1 && std::string(argv[1]) == "--selfplay") {
    int games = (argc > 2) ? std::stoi(argv[2]) : 10;
    int sims = (argc > 3) ? std::stoi(argv[3]) : 400;
    std::cout << "Entering Self-Play mode: " << games << " games @ " << sims << " sims." << std::endl;
    runSelfPlay(*searcher, games, sims);
    return 0;
  }

  Board board;
  int simulations = 400;

  std::cout << "\nInitialization Complete. Type 'go' to let the AI move, or enter a move (e.g., e2e4)." << std::endl;

  while (running) {
    printBoard(board);

    if (board.isDraw()) {
      std::cout << "DRAW by repetition or 50-move rule!" << std::endl;
      running = false;
      continue;
    }

    std::vector<Move> legalMoves = MoveGen::generateMoves(board);
    MoveGen::filterLegalMoves(board, legalMoves);

    if (legalMoves.empty()) {
      if (board.isCheck(board.getSideToMove())) {
        std::cout << "CHECKMATE! " << (board.getSideToMove() == Color::White ? "Black" : "White") << " wins." << std::endl;
      } else {
        std::cout << "STALEMATE! Game is a draw." << std::endl;
      }
      running = false;
      continue;
    }

    std::cout << "Enter move, 'go' for AI, 'u' for undo, or 'q' to quit: ";
    std::string input;
    std::cin >> input;

    if (!running) break;

    if (input == "q") {
      running = false;
      continue;
    }

    if (input == "u") {
      board.undoMove();
      continue;
    }

    if (input == "go") {
      searcher->clearCache();
      std::cout << "AI is thinking (" << simulations << " simulations)..." << std::endl;
      Move bestMove = searcher->findBestMove(board, simulations);
      if (!bestMove.isNone()) {
        std::cout << "AI played: " << bestMove.toAlgebraic() << std::endl;
        board.makeMove(bestMove);
      } else {
        std::cout << "AI found no moves!" << std::endl;
      }
      continue;
    }

    bool found = false;
    for (const auto& m : legalMoves) {
      if (m.toAlgebraic() == input) {
        board.makeMove(m);
        found = true;
        break;
      }
    }

    if (!found) {
      std::cout << "Invalid or illegal move!" << std::endl;
    }
  }

  return 0;
}