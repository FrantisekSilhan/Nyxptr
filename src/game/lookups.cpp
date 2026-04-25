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

#include "Nyxptr/game/lookups.h"

namespace nyx::game::lookups {
  const MagicValues ROOK_TABLE[64] = { // Generated using <https://github.com/Tearth/Fast-Magic-Bitboards>
    {0x80004000803022ULL, 12}, {0x4000200290004cULL, 11}, {0x80082000803002ULL, 11}, {0x20008c020102e00ULL, 11},
    {0x4a00102002004804ULL, 11}, {0x4900080a84002500ULL, 11}, {0x2080020000806300ULL, 11}, {0x80004020800100ULL, 12},
    {0x20800020400091ULL, 11}, {0x808020008a4000ULL, 10}, {0xa004010820020ULL, 10}, {0x8081002230000901ULL, 10},
    {0x2800401480080ULL, 10}, {0x16100180c010012ULL, 10}, {0x100800900800200ULL, 10}, {0x809000202806100ULL, 11},
    {0x2288009400480ULL, 11}, {0x430004020054002ULL, 10}, {0x1488130020004500ULL, 10}, {0x200c420008102200ULL, 10},
    {0x20080100110044c8ULL, 10}, {0x808004000600ULL, 10}, {0x2010100020004ULL, 10}, {0x80002000042a114ULL, 11},
    {0x4400080008820ULL, 11}, {0x600880400081ULL, 10}, {0x2411500200100ULL, 10}, {0x22100080080380ULL, 10},
    {0x1041080080040080ULL, 10}, {0x2022004a00043108ULL, 10}, {0x2020028400111008ULL, 10}, {0x10088200084104ULL, 11},
    {0x4000400080800028ULL, 11}, {0x9030112000400040ULL, 10}, {0x1000200041001100ULL, 10}, {0x40d82301001000ULL, 10},
    {0x800800800400ULL, 10}, {0x89220080800400ULL, 10}, {0x10085084000102ULL, 10}, {0x10051042002084ULL, 11},
    {0x401020808004ULL, 11}, {0x24500020004006ULL, 10}, {0x20008050008024ULL, 10}, {0x100201001001dULL, 10},
    {0xc000880011010004ULL, 10}, {0x8802009028020034ULL, 10}, {0x5040021008040009ULL, 10}, {0x24581020004ULL, 11},
    {0x440218201510200ULL, 11}, {0x40004311812300ULL, 10}, {0x3006001409500ULL, 10}, {0x926220100a00c200ULL, 10},
    {0x8204100805002ULL, 10}, {0x1040804400120180ULL, 10}, {0x100080610038400ULL, 10}, {0x8810212194044200ULL, 11},
    {0x1000530040238001ULL, 12}, {0x3004005201089ULL, 11}, {0x800306200488042ULL, 11}, {0x802004010082006ULL, 11},
    {0x4020028a1041022ULL, 11}, {0x4081000204000801ULL, 11}, {0x5010801089a100cULL, 11}, {0x4145040050810122ULL, 12}
  };

  const MagicValues BISHOP_TABLE[64] = { // Generated using <https://github.com/Tearth/Fast-Magic-Bitboards>
    {0x4402101208910208ULL, 6}, {0x108080820464000ULL, 5}, {0x21041c1400401002ULL, 5}, {0x108048500860408ULL, 5},
    {0x2061003002000ULL, 5}, {0x4002080208002001ULL, 5}, {0xc008820160208008ULL, 5}, {0xa800140404242400ULL, 6},
    {0x2082008008100ULL, 5}, {0x12500208812201ULL, 5}, {0x4020904082004010ULL, 5}, {0x80888a02000040ULL, 5},
    {0x1030040504008000ULL, 5}, {0x2000050120302040ULL, 5}, {0x20a10040400ULL, 5}, {0x4008022120a2084ULL, 5},
    {0x8040181202c20c03ULL, 5}, {0x20440008281c8402ULL, 5}, {0x8040108180a5190ULL, 7}, {0xc000802400820ULL, 7},
    {0x2041004820084108ULL, 7}, {0x8008080900821000ULL, 7}, {0x8202910500905000ULL, 5}, {0x810400c0421080ULL, 5},
    {0x104048421200400ULL, 5}, {0x4010100008420081ULL, 5}, {0x208080401004900ULL, 7}, {0x20080021004048ULL, 9},
    {0x10008c0000812008ULL, 9}, {0x82130a0340480400ULL, 7}, {0x2008008482048442ULL, 5}, {0x2504a0089010120ULL, 5},
    {0x808024144180800ULL, 5}, {0x1082300212040801ULL, 5}, {0x2084020800030042ULL, 7}, {0xe210020080180081ULL, 9},
    {0x41900500400c0ULL, 9}, {0x10100840408048ULL, 7}, {0x3003010104ec00ULL, 5}, {0x401204aa00002200ULL, 5},
    {0x18c08e0840002100ULL, 5}, {0x100a220411000ULL, 5}, {0x100840141007800ULL, 7}, {0x2004010400200ULL, 7},
    {0x6020100210168200ULL, 7}, {0x14008800400200ULL, 7}, {0xd0240080809400ULL, 5}, {0x8005040100400a00ULL, 5},
    {0x8002008220304000ULL, 5}, {0x202602102c00b8ULL, 5}, {0x494010401246001ULL, 5}, {0x2030800084040000ULL, 5},
    {0x40200a4104400d0ULL, 5}, {0x6208082118088060ULL, 5}, {0x1060021082048228ULL, 5}, {0x710100083004010ULL, 5},
    {0x1000420084200290ULL, 6}, {0xc300204230842001ULL, 5}, {0x48010200422208ULL, 5}, {0x2080000a208804ULL, 5},
    {0x4020440182200ULL, 5}, {0xc030b10830500080ULL, 5}, {0x8800042008050506ULL, 5}, {0x20010101040080ULL, 6}
  };
  
  namespace {
    uint64_t generateMask(int sq, bool isRook) {
      uint64_t mask = 0;
      int r = sq / 8, c = sq % 8;

      if (isRook) {
        for (int i = r + 1; i <= 6; ++i) mask |= (1ULL << (i * 8 + c));
        for (int i = r - 1; i >= 1; --i) mask |= (1ULL << (i * 8 + c));
        for (int i = c + 1; i <= 6; ++i) mask |= (1ULL << (r * 8 + i));
        for (int i = c - 1; i >= 1; --i) mask |= (1ULL << (r * 8 + i));
      } else {
        int dr[] = {1, 1, -1, -1};
        int dc[] = {1, -1, 1, -1};
        for (int i = 0; i < 4; ++i) {
          for (int step = 1; step < 8; ++step) {
            int nr = r + dr[i] * step;
            int nc = c + dc[i] * step;
            if (nr >= 1 && nr <= 6 && nc >= 1 && nc <= 6) mask |= (1ULL << (nr * 8 + nc));
          }
        }
      }
      return mask;
    }

    uint64_t generateAttacks(int sq, uint64_t occ, bool isRook) {
      uint64_t attacks = 0;
      int r = sq / 8, c = sq % 8;
      int dr[] = {1, -1, 0, 0, 1, 1, -1, -1};
      int dc[] = {0, 0, 1, -1, 1, -1, 1, -1};
      int start = isRook ? 0 : 4, end = isRook ? 4 : 8;

      for (int i = start; i < end; ++i) {
        for (int step = 1; step < 8; ++step) {
          int nr = r + dr[i] * step, nc = c + dc[i] * step;
          if (nr < 0 || nr > 7 || nc < 0 || nc > 7) break;
          attacks |= (1ULL << (nr * 8 + nc));
          if (occ & (1ULL << (nr * 8 + nc))) break;
        }
      }
      return attacks;
    }

    uint64_t mapIndexToOccupancy(int index, int bits, uint64_t mask) {
      uint64_t occupancy = 0;
      for (int i = 0; i < bits; ++i) {
        int sq = bitboard::popLSB(mask);
        if (index & (1 << i)) occupancy |= (1ULL << sq);
      }
      return occupancy;
    }
  }

  void initSliderTables() {
    uint64_t* currentPtr = sliderDatabase;

    for (int i = 0; i < 64; ++i) {
      rookMagics[i].mask = generateMask(i, true);
      rookMagics[i].magic = ROOK_TABLE[i].magic;
      rookMagics[i].shift = 64 - ROOK_TABLE[i].bits;
      rookMagics[i].ptr = currentPtr;

      int variations = 1 << ROOK_TABLE[i].bits;
      for (int j = 0; j < variations; ++j) {
        uint64_t occ = mapIndexToOccupancy(j, ROOK_TABLE[i].bits, rookMagics[i].mask);
        rookMagics[i].ptr[getMagicIndex(rookMagics[i], occ)] = generateAttacks(i, occ, true);
      }
      currentPtr += variations;

      bishopMagics[i].mask = generateMask(i, false);
      bishopMagics[i].magic = BISHOP_TABLE[i].magic;
      bishopMagics[i].shift = 64 - BISHOP_TABLE[i].bits;
      bishopMagics[i].ptr = currentPtr;

      variations = 1 << BISHOP_TABLE[i].bits;
      for (int j = 0; j < variations; ++j) {
        uint64_t occ = mapIndexToOccupancy(j, BISHOP_TABLE[i].bits, bishopMagics[i].mask);
        bishopMagics[i].ptr[getMagicIndex(bishopMagics[i], occ)] = generateAttacks(i, occ, false);
      }
      currentPtr += variations;
    }
  }
}