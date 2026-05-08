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

#include "Nyxptr/engine/converter.h"
#include "Nyxptr/engine/searcher.h"
#include "Nyxptr/game/movegen.h"
#include <fstream>
#include <sstream>
#include <iostream>
#include <cctype>

namespace nyx::engine {
  void DataConverter::convertPGNToBinary(const std::string& pgnPath, const std::string& binPath) {
    std::ifstream pgnFile(pgnPath);
    if (!pgnFile.is_open()) {
      std::cerr << "Error: Could not open PGN file: " << pgnPath << std::endl;
      return;
    }
   
    PRecorder recorder(binPath);
    std::string line;

    int gamesConverted = 0;
    int gamesSkipped = 0;
    game::Board board;
    bool gameActive = false;
    bool sawMove = false;
    bool gameInvalid = false;
    std::string taggedResult;

    auto isResultToken = [](const std::string& token) {
      return token == "1-0" || token == "0-1" || token == "1/2-1/2" || token == "*";
    };

    auto applyResult = [&](const std::string& token) {
      if (token == "*") {
        gameActive = false;
        sawMove = false;
        gameInvalid = false;
        taggedResult.clear();
        return;
      }

      if (gameInvalid || !sawMove) {
        gamesSkipped++;
        gameActive = false;
        sawMove = false;
        gameInvalid = false;
        taggedResult.clear();
        return;
      }

      float result = 0.0f;
      if (token == "1-0") result = (board.getSideToMove() == game::Color::White) ? 1.0f : -1.0f;
      else if (token == "0-1") result = (board.getSideToMove() == game::Color::Black) ? 1.0f : -1.0f;

      recorder.finishGame(result);
      gamesConverted++;
      gameActive = false;
      sawMove = false;
      gameInvalid = false;
      taggedResult.clear();

      if (gamesConverted % 1000 == 0) {
        std::cout << "Converted " << gamesConverted << " games..." << std::endl;
      }
    };

    auto finalizePendingGame = [&]() {
      if (!gameActive) return;

      if (isResultToken(taggedResult) && taggedResult != "*") {
        applyResult(taggedResult);
      } else if (sawMove) {
        gamesSkipped++;
        gameActive = false;
        sawMove = false;
        gameInvalid = false;
        taggedResult.clear();
      }
    };

    while (std::getline(pgnFile, line)) {
      if (line.empty()) continue;

      if (line.rfind("[Event ", 0) == 0) {
        finalizePendingGame();
        board = game::Board();
        gameActive = true;
        sawMove = false;
        gameInvalid = false;
        taggedResult.clear();
      }

      if (line[0] == '[') {
        if (line.rfind("[Result ", 0) == 0) {
          const std::size_t firstQuote = line.find('"');
          const std::size_t lastQuote = line.rfind('"');
          if (firstQuote != std::string::npos && lastQuote != std::string::npos && lastQuote > firstQuote) {
            taggedResult = line.substr(firstQuote + 1, lastQuote - firstQuote - 1);
          }
        }
        continue;
      }

      if (!gameActive || gameInvalid) continue;

      std::stringstream ss(line);
      std::string token;

      while (ss >> token) {
        if (!token.empty() && token.back() == '\r') token.pop_back();

        if (isResultToken(token)) {
          applyResult(token);
          break;
        }

        if (!token.empty() && std::isdigit(static_cast<unsigned char>(token[0]))) continue;

        game::Move move = findMoveInPGN(board, token);
        if (move.isNone()) {
          gameInvalid = true;
          break;
        }
        
        recorder.recordStep(board, move);
        board.makeMove(move);
        sawMove = true;
      }
    }

    finalizePendingGame();

    std::cout << "Conversion complete. Converted: " << gamesConverted
              << ", skipped: " << gamesSkipped << std::endl;
  }

  char DataConverter::getPieceChar(uint8_t piece) {
    switch (piece) {
      case static_cast<uint8_t>(game::Piece::Pawn): return 'P';
      case static_cast<uint8_t>(game::Piece::Knight): return 'N';
      case static_cast<uint8_t>(game::Piece::Bishop): return 'B';
      case static_cast<uint8_t>(game::Piece::Rook): return 'R';
      case static_cast<uint8_t>(game::Piece::Queen): return 'Q';
      case static_cast<uint8_t>(game::Piece::King): return 'K';
      default: return '?';
    }
  }

  game::Move DataConverter::findMoveInPGN(game::Board& board, std::string san) {
    std::vector<game::Move> moves = game::MoveGen::generateMoves(board);
    game::MoveGen::filterLegalMoves(board, moves);

    if (san.empty()) return game::Move();
    if (san.back() == '+' || san.back() == '#') san.pop_back();

    if (san == "O-O" || san == "O-O-O") {
      bool kingSide = (san == "O-O");
      for (const auto& m : moves) {
        if (kingSide && (m.getFlags() == game::Move::KingCastle)) return m;
        if (!kingSide && (m.getFlags() == game::Move::QueenCastle)) return m;
      }
      return game::Move();
    }

    uint16_t promoFlag = 0;
    if (san.find('=') != std::string::npos) {
      char p = san.back();
      if (p == 'Q') promoFlag = game::Move::ProjQueen;
      else if (p == 'R') promoFlag = game::Move::ProjRook;
      else if (p == 'B') promoFlag = game::Move::ProjBishop;
      else if (p == 'N') promoFlag = game::Move::ProjKnight;

      san = san.substr(0, san.find('='));
    }

    char pieceChar = 'P';
    if (isupper(san[0])) {
      pieceChar = san[0];
      san = san.substr(1);
    }

    if (san.length() < 2) return game::Move();
    std::string targetStr = san.substr(san.length() - 2);
    int targetFile = targetStr[0] - 'a';
    int targetRank = targetStr[1] - '1';
    int targetSq = targetRank * 8 + targetFile;

    san = san.substr(0, san.length() - 2);
    if (!san.empty() && san.back() == 'x') san.pop_back();

    for (const auto& m : moves) {
      if (m.getTo() != targetSq) continue;

      uint8_t movingPiece = static_cast<uint8_t>(board.getPieceAt(static_cast<game::Square>(m.getFrom())));
      if (getPieceChar(movingPiece) != pieceChar) continue;

      if (promoFlag != 0) {
        if (!(m.getFlags() & game::Move::Promotion)) continue;
        if ((m.getFlags() & 0x3) != (promoFlag & 0x3)) continue;
      }

      if (!san.empty()) {
        int fromFile = m.getFrom() % 8;
        int fromRank = m.getFrom() / 8;

        if (san.length() == 1) {
          if (isdigit(san[0])) {
            if (fromRank != (san[0] - '1')) continue;
          } else {
            if (fromFile != (san[0] - 'a')) continue;
          }
        } else if (san.length() == 2) {
          if (fromFile != (san[0] - 'a')) continue;
          if (fromRank != (san[1] - '1')) continue;
        }
      }

      return m;
    }

    return game::Move();
  }

  void DataConverter::convertLichessPuzzles(const std::string& csvPath, const std::string& binPathBase) {
    std::ifstream file(csvPath);
    std::ofstream errorLog("puzzle_errors.txt");
    if (!file.is_open()) return;
    std::string line;
    std::getline(file, line); // Skip header

    int count = 0, skippedCount = 0;
    int fileIndex = 1;
    const int PUZZLES_PER_FILE = 500000;

    std::unique_ptr<PRecorder> recorder = std::make_unique<PRecorder>(binPathBase + "_01.bin");

    while (std::getline(file, line)) {
      std::vector<std::string> cols;
      std::stringstream ss(line);
      std::string cell;
      while (std::getline(ss, cell, ',')) cols.push_back(cell);
      
      if (cols.size() < 3) {
        skippedCount++;
        continue;
      }
      
      const std::string& fen = cols[1];
      const std::string& movesStr = cols[2];
      const std::string& themes = (cols.size() > 7) ? cols[7] : "";
      
      game::Board board;
      try {
        board.loadFEN(fen);
      } catch (...) {
        errorLog << "FEN_PARSE_ERROR | " << fen << std::endl;
        skippedCount++;
        continue;
      }
      
      std::stringstream mss(movesStr);
      std::string uci;
      
      if (mss >> uci) {
        auto legal = game::MoveGen::generateMoves(board);
        game::MoveGen::filterLegalMoves(board, legal);
        bool moved = false;
        for (const auto& m : legal) {
          if (m.toAlgebraic() == uci) {
            if (board.makeMove(m)) {
              moved = true;
              break;
            }
          }
        }
        if (!moved) {
          errorLog << "ILLEGAL_PREMOVE | move: " << uci << " | fen: " << fen << std::endl;
          skippedCount++;
          continue;
        }
      }
      
      bool playerTurn = true;
      bool puzzleValid = true;
      while (mss >> uci) {
        auto legal = game::MoveGen::generateMoves(board);
        game::MoveGen::filterLegalMoves(board, legal);
        
        game::Move bestMove;
        for (const auto& m : legal) {
          if (m.toAlgebraic() == uci) {
            bestMove = m;
            break;
          }
        }
        
        if (bestMove.isNone()) {
          errorLog << "MOVE_NOT_FOUND | move: " << uci << " | turn: " << (playerTurn ? "Player" : "Opponent") << " | fen: " << fen << std::endl;
          puzzleValid = false;
          break;
        }
        
        if (playerTurn) recorder->recordStep(board, bestMove);
        board.makeMove(bestMove);
        playerTurn = !playerTurn;
      }

      if (!puzzleValid) {
        skippedCount++;
        continue;
      }

      float result = 0.0f;
      if (themes.find("mate") != std::string::npos) result = 1.0f;
      else if (themes.find("crushing") != std::string::npos) result = 0.95f;
      else if (themes.find("advantage") != std::string::npos) result = 0.8f;
      else if (themes.find("equality") != std::string::npos) result = 0.0f;
      else result = 0.5f; // Unknown result, treat as a slight advantage (shouldn't happen at all)
      
      recorder->finishPuzzle(result);
      count++;

      if (count > 0 && count % PUZZLES_PER_FILE == 0) {
        fileIndex++;
        std::string paddedIndex = (fileIndex < 10) ? "0" + std::to_string(fileIndex) : std::to_string(fileIndex);
        std::string newPath = binPathBase + "_" + paddedIndex + ".bin";
        
        std::cout << "\n[Split] Rotating to new file: " << newPath << std::endl;
        recorder = std::make_unique<PRecorder>(newPath);
      }
      if (count % 50000 == 0) std::cout << "Converted " << count << " puzzles..." << std::endl;
    }
    std::cout << "\nDone. Created " << fileIndex << " puzzle shards." << std::endl;
    std::cout << "Skipped " << skippedCount << " puzzles due to errors or invalid data." << std::endl;
  }
}