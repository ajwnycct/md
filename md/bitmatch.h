/*!
    * md, a Max external
    * Copyright (c) 2026 Adam James Wilson. All rights reserved.
    * SPDX-License-Identifier: BSD 3-Clause Clear
    */

#ifndef BITMATCH_H
#define BITMATCH_H

#include <stdint.h>
#include "defs.h"

static const uint_fast8_t k_from_pairs[121] = {
	  0, 1, 2,
	  3, 3, 3,
	  4, 4, 4, 4,
	  5, 5, 5, 5, 5,
	  6, 6, 6, 6, 6, 6,
	  7, 7, 7, 7, 7, 7, 7,
	  8, 8, 8, 8, 8, 8, 8, 8,
	  9, 9, 9, 9, 9, 9, 9, 9, 9,
	  10,10,10,10,10,10,10,10,10,10,
	  11,11,11,11,11,11,11,11,11,11,11,
	  12,12,12,12,12,12,12,12,12,12,12,12,
	  13,13,13,13,13,13,13,13,13,13,13,13,13,
	  14,14,14,14,14,14,14,14,14,14,14,14,14,14,
	 15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,
	  16,
};

static inline int_fast8_t compare_graphs_64(t_md *x, uint_fast64_t a, uint_fast64_t b, uint_fast8_t min)
{
    uint64_t diff = (((uint64_t *)x->p)[a] ^ ((uint64_t *)x->p)[b])
                  | (((uint64_t *)x->t)[a] ^ ((uint64_t *)x->t)[b]);
    if (diff == 0) return x->N;
    int lz = __builtin_clzll(diff);
    int pack_bits = x->N * (x->N - 1);
    if (lz >= pack_bits) return x->N;
    uint_fast8_t k = k_from_pairs[lz >> 1];
    return (k < min) ? -1 : (int_fast8_t)k;
}

static inline int_fast8_t compare_graphs_128(t_md *x, uint_fast64_t a, uint_fast64_t b, uint_fast8_t min)
{
    uint128_t diff = (((uint128_t *)x->p)[a] ^ ((uint128_t *)x->p)[b])
                   | (((uint128_t *)x->t)[a] ^ ((uint128_t *)x->t)[b]);
    if (diff == 0) return x->N;
    uint64_t hi = (uint64_t)(diff >> 64);
    uint64_t lo = (uint64_t) diff;
    int lz = hi ? __builtin_clzll(hi) : 64 + __builtin_clzll(lo);
    int pack_bits = x->N * (x->N - 1);
    if (lz >= pack_bits) return x->N;
    uint_fast8_t k = k_from_pairs[lz >> 1];
    return (k < min) ? -1 : (int_fast8_t)k;
}

static inline int_fast8_t compare_graphs_192(t_md *x, uint_fast64_t a, uint_fast64_t b, uint_fast8_t min)
{
    uint192_t diff = (((uint192_t *)x->p)[a] ^ ((uint192_t *)x->p)[b])
                   | (((uint192_t *)x->t)[a] ^ ((uint192_t *)x->t)[b]);
    if (diff == 0) return x->N;
    uint64_t h2 = (uint64_t)(diff >> 128);
    uint64_t h1 = (uint64_t)(diff >>  64);
    uint64_t h0 = (uint64_t) diff;
    int lz;
    if      (h2) lz =       __builtin_clzll(h2);
    else if (h1) lz =  64 + __builtin_clzll(h1);
    else         lz = 128 + __builtin_clzll(h0);
    int pack_bits = x->N * (x->N - 1);
    if (lz >= pack_bits) return x->N;
    uint_fast8_t k = k_from_pairs[lz >> 1];
    return (k < min) ? -1 : (int_fast8_t)k;
}

static inline int_fast8_t compare_graphs_256(t_md *x, uint_fast64_t a, uint_fast64_t b, uint_fast8_t min)
{
    uint256_t diff = (((uint256_t *)x->p)[a] ^ ((uint256_t *)x->p)[b])
                   | (((uint256_t *)x->t)[a] ^ ((uint256_t *)x->t)[b]);
    if (diff == 0) return x->N;
    uint64_t h3 = (uint64_t)(diff >> 192);
    uint64_t h2 = (uint64_t)(diff >> 128);
    uint64_t h1 = (uint64_t)(diff >>  64);
    uint64_t h0 = (uint64_t) diff;
    int lz;
    if      (h3) lz =       __builtin_clzll(h3);
    else if (h2) lz =  64 + __builtin_clzll(h2);
    else if (h1) lz = 128 + __builtin_clzll(h1);
    else         lz = 192 + __builtin_clzll(h0);
    int pack_bits = x->N * (x->N - 1);
    if (lz >= pack_bits) return x->N;
    uint_fast8_t k = k_from_pairs[lz >> 1];
    return (k < min) ? -1 : (int_fast8_t)k;
}

static inline int_fast8_t compare_graphs(t_md *x, uint_fast64_t a, uint_fast64_t b, uint_fast8_t min)
{
		switch (x->N) {
				case 3  ... 8:  return compare_graphs_64 (x, a, b, min);
				case 9  ... 11: return compare_graphs_128(x, a, b, min);
				case 12 ... 14: return compare_graphs_192(x, a, b, min);
				case 15 ... 16: return compare_graphs_256(x, a, b, min);
				default: return -1;
		}
}

#endif
