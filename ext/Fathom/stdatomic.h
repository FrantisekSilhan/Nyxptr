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

#ifndef FORCED_STDATOMIC_H
#define FORCED_STDATOMIC_H

#include <stdbool.h>

#define _Atomic
typedef bool atomic_bool;
typedef int  atomic_int;

#define atomic_init(obj, val) (*obj = val)
#define atomic_load_explicit(obj, mem) (*obj)
#define atomic_store_explicit(obj, val, mem) (*obj = val)

#define memory_order_relaxed 0
#define memory_order_acquire 0
#define memory_order_release 0

#endif