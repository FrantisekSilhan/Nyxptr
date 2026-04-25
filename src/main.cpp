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

#include <iostream>
#include <iomanip>
#include <vector>
#include "Nyxptr/game/board.h"
#include "Nyxptr/game/lookups.h"
#include "Nyxptr/game/movegen.h"

using namespace nyx::game;

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

int main() {
  std::cout << "Initializing Engine..." << std::endl;
  lookups::initSliderTables();
  Board::initZobrist();
  std::cout << "Initialization Complete.\n" << std::endl;

  Board board;

  bool running = true;
  while (running) {
    printBoard(board);

    std::vector<Move> moves = MoveGen::generateMoves(board);
    
    std::cout << "Available moves (" << moves.size() << "): ";
    for (const auto& m : moves) {
      std::cout << m.toAlgebraic() << " ";
    }
    std::cout << "\n\nEnter move (or 'q' to quit, 'u' to undo): ";
    
    std::string input;
    std::cin >> input;

    if (input == "q") {
      running = false;
      continue;
    }

    if (input == "u") {
      board.undoMove();
      continue;
    }

    bool found = false;
    for (const auto& m : moves) {
      if (m.toAlgebraic() == input) {
        if (board.makeMove(m)) {
          std::cout << "Played: " << input << std::endl;
          found = true;
        } else {
          std::cout << "Move resulted in an illegal position (King left in check)!" << std::endl;
        }
        break;
      }
    }

    if (!found) {
      std::cout << "Invalid move! Please use algebraic notation (e.g., e2e4)." << std::endl;
    }
  }

  return 0;
}