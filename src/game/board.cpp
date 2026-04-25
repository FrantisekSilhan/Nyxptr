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

#include "Nyxptr/game/board.h"
#include "Nyxptr/game/lookups.h"
#include <random>

namespace nyx::game {
  Board::Board() {
    for (int c = 0; c < 2; ++c) {
      colorOccupancy[c] = 0ULL;
      kingSquare[c] = Square::None;
      for (int p = 0; p < 6; ++p) bitboards[c][p] = 0ULL;
    }
    combinedOccupancy = 0ULL;
    for (int i = 0; i < 64; ++i) {
      boardArray[i] = Piece::None;
      colorArray[i] = Color::None;
    }

    sideToMove = Color::White;
    castlingRights = WhiteKingside | WhiteQueenside | BlackKingside | BlackQueenside;
    enPassantSquare = Square::None;
    halfMoveClock = 0;
    fullMoveNumber = 1;
    zobristKey = 0ULL;

    auto setupPiece = [&](Square sq, Piece p, Color c) {
      addPiece(sq, p, c);
      if (p == Piece::King) kingSquare[to_i(c)] = sq;
    };

    for (int i = 0; i < 8; ++i) {
      setupPiece(static_cast<Square>(to_i(Square::A2) + i), Piece::Pawn, Color::White);
      setupPiece(static_cast<Square>(to_i(Square::A7) + i), Piece::Pawn, Color::Black);
    }

    setupPiece(Square::A1, Piece::Rook, Color::White); setupPiece(Square::H1, Piece::Rook, Color::White);
    setupPiece(Square::A8, Piece::Rook, Color::Black); setupPiece(Square::H8, Piece::Rook, Color::Black);

    setupPiece(Square::B1, Piece::Knight, Color::White); setupPiece(Square::G1, Piece::Knight, Color::White);
    setupPiece(Square::B8, Piece::Knight, Color::Black); setupPiece(Square::G8, Piece::Knight, Color::Black);

    setupPiece(Square::C1, Piece::Bishop, Color::White); setupPiece(Square::F1, Piece::Bishop, Color::White);
    setupPiece(Square::C8, Piece::Bishop, Color::Black); setupPiece(Square::F8, Piece::Bishop, Color::Black);

    setupPiece(Square::D1, Piece::Queen, Color::White);
    setupPiece(Square::D8, Piece::Queen, Color::Black);

    setupPiece(Square::E1, Piece::King, Color::White);
    setupPiece(Square::E8, Piece::King, Color::Black);

    zobristKey ^= sideKey;
    zobristKey ^= castlingKeys[castlingRights];
  }

  static constexpr uint64_t NYX_SEED = 0x4E7978707472ULL;
  uint64_t Board::pieceKeys[2][6][64];
  uint64_t Board::sideKey;
  uint64_t Board::castlingKeys[16];
  uint64_t Board::enPassantKeys[64];

  void Board::initZobrist() {
    std::mt19937_64 rng(NYX_SEED);
    for (int c = 0; c < 2; c++) // You see what I did there?
      for (int p = 0; p < 6; ++p)
        for (int s = 0; s < 64; ++s)
          pieceKeys[c][p][s] = rng();

    sideKey = rng();
    for (int i = 0; i < 16; ++i) castlingKeys[i] = rng();
    for (int s = 0; s < 64; ++s) enPassantKeys[s] = rng();
  }

  bool Board::isDraw() const {
    if (halfMoveClock >= 100) return true;

    int repetitions = 0;
    for (const auto& state : history) {
      if (state.zobristKey == zobristKey) {
        repetitions++;
      }
    }

    if (repetitions >= 2) return true;

    // TODO: Implement insufficient material checks

    return false;
  }

  uint64_t Board::getAttacksTo(Square sq, Color side) const {
    uint64_t occ = combinedOccupancy;
    uint64_t allAttackers = 0;
    int sIdx = to_i(side);
    int sqIdx = to_i(sq);
    
    allAttackers |= lookups::bishopMagics[sqIdx].ptr[getMagicIndex(lookups::bishopMagics[sqIdx], occ)]
    & (bitboards[sIdx][to_i(Piece::Bishop)] | bitboards[sIdx][to_i(Piece::Queen)]);
    
    allAttackers |= lookups::rookMagics[sqIdx].ptr[getMagicIndex(lookups::rookMagics[sqIdx], occ)]
    & (bitboards[sIdx][to_i(Piece::Rook)] | bitboards[sIdx][to_i(Piece::Queen)]);
    
    allAttackers |= lookups::knightTable[sqIdx] & bitboards[sIdx][to_i(Piece::Knight)];
    allAttackers |= lookups::kingTable[sqIdx] & bitboards[sIdx][to_i(Piece::King)];

    uint64_t kingMask = (1ULL << sqIdx);

    if (side == Color::White) {
      allAttackers |= ((kingMask >> 9) & ~0x80808080808080ULL | (kingMask >> 7) & ~0x0101010101010101ULL) & bitboards[sIdx][to_i(Piece::Pawn)];
    } else {
      allAttackers |= ((kingMask << 7) & ~0x80808080808080ULL | (kingMask << 9) & ~0x0101010101010101ULL) & bitboards[sIdx][to_i(Piece::Pawn)];
    }

    return allAttackers;
  }

  bool Board::isSquareAttacked(Square sq, Color attackerColor) const {
    const int sIdx = to_i(attackerColor);
    const int sqIdx = to_i(sq);
    const uint64_t occ = combinedOccupancy;

    if (lookups::knightTable[sqIdx] & bitboards[sIdx][to_i(Piece::Knight)]) return true;
    if (lookups::bishopMagics[sqIdx].ptr[getMagicIndex(lookups::bishopMagics[sqIdx], occ)]
        & (bitboards[sIdx][to_i(Piece::Bishop)] | bitboards[sIdx][to_i(Piece::Queen)])) return true;
    if (lookups::rookMagics[sqIdx].ptr[getMagicIndex(lookups::rookMagics[sqIdx], occ)]
        & (bitboards[sIdx][to_i(Piece::Rook)] | bitboards[sIdx][to_i(Piece::Queen)])) return true;

    const uint64_t sqMask = (1ULL << sqIdx);
    if (attackerColor == Color::White) {
      if ((sqMask >> 9) & ~0x80808080808080ULL & bitboards[sIdx][to_i(Piece::Pawn)]) return true;
      if ((sqMask >> 7) & ~0x0101010101010101ULL & bitboards[sIdx][to_i(Piece::Pawn)]) return true;
    } else {
      if ((sqMask << 7) & ~0x80808080808080ULL & bitboards[sIdx][to_i(Piece::Pawn)]) return true;
      if ((sqMask << 9) & ~0x0101010101010101ULL & bitboards[sIdx][to_i(Piece::Pawn)]) return true;
    }

    if (lookups::kingTable[sqIdx] & bitboards[sIdx][to_i(Piece::King)]) return true;

    return false;
  }

  std::vector<float> Board::getFullStateTensor() const {
    std::vector<float> tensor(832, 0.0f);

    for (int p = 0; p < 6; ++p) {
      for (int c = 0; c < 2; ++c) {
        uint64_t bb = bitboards[c][p];
        int planeOffset = (c * 6 + p) * 64;
        while (bb) {
          int sq = bitboard::popLSB(bb);
          tensor[planeOffset + sq] = 1.0f;
        }
      }
    }

    for (int i = 0; i < 64; ++i) {
      tensor[12 * 64 + i] = (sideToMove == Color::White) ? 1.0f : 0.0f;
    }

    return tensor;
  }

  void Board::addPiece(Square sq, Piece p, Color c) {
    uint64_t mask = 1ULL << static_cast<int>(sq);
    int cIdx = static_cast<int>(c);
    int pIdx = static_cast<int>(p);

    bitboards[cIdx][pIdx] |= mask;
    colorOccupancy[cIdx] |= mask;
    combinedOccupancy |= mask;
    boardArray[static_cast<int>(sq)] = p;
    colorArray[static_cast<int>(sq)] = c;

    zobristKey ^= pieceKeys[cIdx][pIdx][static_cast<int>(sq)];
  }

  void Board::removePiece(Square sq, Piece p, Color c) {
    uint64_t mask = 1ULL << static_cast<int>(sq);
    int cIdx = static_cast<int>(c);
    int pIdx = static_cast<int>(p);

    bitboards[cIdx][pIdx] &= ~mask;
    colorOccupancy[cIdx] &= ~mask;
    combinedOccupancy &= ~mask;
    boardArray[static_cast<int>(sq)] = Piece::None;
    colorArray[static_cast<int>(sq)] = Color::None;

    zobristKey ^= pieceKeys[cIdx][pIdx][static_cast<int>(sq)];
  }

  bool Board::makeMove(Move m) {
    UndoState state;
    state.castlingRights = castlingRights;
    state.enPassantSquare = enPassantSquare;
    state.halfMoveClock = halfMoveClock;
    state.zobristKey = zobristKey;
    state.lastMove = m;

    const Square from = static_cast<Square>(m.getFrom());
    const Square to = static_cast<Square>(m.getTo());
    const uint16_t flags = m.getFlags();
    const Piece movingPiece = getPieceAt(from);
    const Color us = sideToMove;

    state.capturedPiece = Piece::None;
    if (flags == Move::EnPassant) {
      Square epPawnSq = static_cast<Square>(to_i(to) + (us == Color::White ? -8 : 8));
      state.capturedPiece = Piece::Pawn;
      removePiece(epPawnSq, Piece::Pawn, !us);
    } else {
      state.capturedPiece = getPieceAt(to);
      if (state.capturedPiece != Piece::None) {
        removePiece(to, state.capturedPiece, !us);
      }
    }

    removePiece(from, movingPiece, us);
    if (!(flags & Move::Promotion)) {
      addPiece(to, movingPiece, us);
    } else {
      Piece promoPiece = static_cast<Piece>(to_i(Piece::Knight) + (flags & 0x3));
      addPiece(to, promoPiece, us);
    }

    if (flags == Move::KingCastle) {
      Square rFrom = (us == Color::White) ? Square::H1 : Square::H8;
      Square rTo = (us == Color::White) ? Square::F1 : Square::F8;
      removePiece(rFrom, Piece::Rook, us);
      addPiece(rTo, Piece::Rook, us);
    } else if (flags == Move::QueenCastle) {
      Square rFrom = (us == Color::White) ? Square::A1 : Square::A8;
      Square rTo = (us == Color::White) ? Square::D1 : Square::D8;
      removePiece(rFrom, Piece::Rook, us);
      addPiece(rTo, Piece::Rook, us);
    }

    if (movingPiece == Piece::King) kingSquare[to_i(us)] = to;

    if (enPassantSquare != Square::None) zobristKey ^= enPassantKeys[to_i(enPassantSquare)];
    zobristKey ^= castlingKeys[castlingRights];

    if (movingPiece == Piece::King) {
      castlingRights &= (us == Color::White) ? ~0x3 : ~0xC;
    }
    if (from == Square::A1 || to == Square::A1) castlingRights &= ~WhiteQueenside;
    if (from == Square::H1 || to == Square::H1) castlingRights &= ~WhiteKingside;
    if (from == Square::A8 || to == Square::A8) castlingRights &= ~BlackQueenside;
    if (from == Square::H8 || to == Square::H8) castlingRights &= ~BlackKingside;

    if (flags == Move::DoublePawnPush) {
      enPassantSquare = static_cast<Square>(to_i(to) + (us == Color::White ? -8 : 8));
    } else {
      enPassantSquare = Square::None;
    }

    if (enPassantSquare != Square::None) zobristKey ^= enPassantKeys[to_i(enPassantSquare)];
    zobristKey ^= castlingKeys[castlingRights];

    if (movingPiece == Piece::Pawn || state.capturedPiece != Piece::None) {
      halfMoveClock = 0;
    } else {
      halfMoveClock++;
    }

    if (sideToMove == Color::Black) fullMoveNumber++;
    sideToMove = !sideToMove;
    zobristKey ^= sideKey;

    if (isSquareAttacked(kingSquare[to_i(us)], !us)) {
      history.push_back(state);
      undoMove();
      return false;
    }

    history.push_back(state);
    return true;
  }

  void Board::undoMove() {
    if (history.empty()) return;

    UndoState state = history.back();
    history.pop_back();

    Move m = state.lastMove;
    Square from = static_cast<Square>(m.getFrom());
    Square to = static_cast<Square>(m.getTo());
    uint16_t flags = m.getFlags();

    if (sideToMove == Color::White) fullMoveNumber--;
    sideToMove = !sideToMove;
    
    Color us = sideToMove;
    Piece movedPiece = boardArray[to_i(to)];

    if (flags & Move::Promotion) {
      removePiece(to, movedPiece, us);
      addPiece(from, Piece::Pawn, us);
    } else {
      removePiece(to, movedPiece, us);
      addPiece(from, movedPiece, us);
    }

    if (flags == Move::EnPassant) {
      Square epPawnSq = static_cast<Square>(to_i(to) + (us == Color::White ? -8 : 8));
      addPiece(epPawnSq, Piece::Pawn, !us);
    } else if (state.capturedPiece != Piece::None) {
      addPiece(to, state.capturedPiece, !us);
    }

    if (flags == Move::KingCastle) {
      Square rFrom = (us == Color::White) ? Square::H1 : Square::H8;
      Square rTo = (us == Color::White) ? Square::F1 : Square::F8;
      removePiece(rTo, Piece::Rook, us);
      addPiece(rFrom, Piece::Rook, us);
    } else if (flags == Move::QueenCastle) {
      Square rFrom = (us == Color::White) ? Square::A1 : Square::A8;
      Square rTo = (us == Color::White) ? Square::D1 : Square::D8;
      removePiece(rTo, Piece::Rook, us);
      addPiece(rFrom, Piece::Rook, us);
    }

    castlingRights = state.castlingRights;
    enPassantSquare = state.enPassantSquare;
    halfMoveClock = state.halfMoveClock;
    zobristKey = state.zobristKey;

    if (movedPiece == Piece::King || (flags & Move::Promotion && Piece::Pawn == Piece::King)) {
      kingSquare[to_i(us)] = from;
    }
    if (boardArray[to_i(from)] == Piece::King) {
      kingSquare[to_i(us)] = from;
    }
  }
}