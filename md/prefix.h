/*!
    * md, a Max external
    * Copyright (c) 2026 Adam James Wilson. All rights reserved.
    * SPDX-License-Identifier: BSD 3-Clause Clear
    */

#ifndef PREFIX_H
#define PREFIX_H

#include "defs.h"

static inline uint64_t prefix3_fingerprint(t_md *x, int_fast64_t pos) {
    uint64_t top_pg = 0, top_tg = 0;
    switch (x->N) {
        case 3 ... 8:   top_pg = ((uint64_t   *)x->p)[pos];
                        top_tg = ((uint64_t   *)x->t)[pos]; break;
        case 9 ... 11:  top_pg = (uint64_t)(((uint128_t *)x->p)[pos] >> 64);
                        top_tg = (uint64_t)(((uint128_t *)x->t)[pos] >> 64); break;
        case 12 ... 14: top_pg = (uint64_t)(((uint192_t *)x->p)[pos] >> 128);
                        top_tg = (uint64_t)(((uint192_t *)x->t)[pos] >> 128); break;
        case 15 ... 16: top_pg = (uint64_t)(((uint256_t *)x->p)[pos] >> 192);
                        top_tg = (uint64_t)(((uint256_t *)x->t)[pos] >> 192); break;
    }
    return ((top_pg >> 58) << 6) | (top_tg >> 58);
}

static inline void prefix_index_register(t_md *x, int_fast64_t pos)
{
    uint64_t key = prefix3_fingerprint(x, pos);
    idx_array_t *b = prefix_index_get(x->prefix_index, key);
    if (b != NULL) {
        idx_array_push_back(*b, pos);
    } else {
        idx_array_t fresh;
        idx_array_init(fresh);
        idx_array_push_back(fresh, pos);
        prefix_index_set_at(x->prefix_index, key, fresh);
        idx_array_clear(fresh);
    }
}

#endif
