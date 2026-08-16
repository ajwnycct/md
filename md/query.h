/*!
    * md, a Max external
    * Copyright (c) 2026 Adam James Wilson. All rights reserved.
    * SPDX-License-Identifier: BSD 3-Clause Clear
    */

#ifndef QUERY_H
#define QUERY_H

#include "defs.h"
#include "heap.h"

#define DEFINE_QUERY_TOP_M(WIDTH)                                              \
static inline int query_top_m_##WIDTH(t_md *x,                                 \
                                      catalog##WIDTH##_t cat,                  \
                                      int M,                                   \
                                      heap_cmp_mode_t mode,                    \
                                      heap_entry##WIDTH##_t *heap)             \
{                                                                              \
    (void)x;                                                                   \
    if (M <= 0) return 0;                                                      \
    if (M > HEAP_MAX_M) M = HEAP_MAX_M;                                        \
                                                                               \
    int n = 0;                                                                 \
    catalog##WIDTH##_it_t it;                                                  \
    for (catalog##WIDTH##_it(it, cat); !catalog##WIDTH##_end_p(it);            \
         catalog##WIDTH##_next(it))                                            \
    {                                                                          \
        catalog##WIDTH##_pair_ct *pair = catalog##WIDTH##_ref(it);             \
        size_t idx_n = idx_array_size(pair->value.indices);                    \
        int_fast64_t last = (idx_n > 0)                                        \
            ? *idx_array_cget(pair->value.indices, idx_n - 1)                  \
            : (int_fast64_t)-1;                                                \
                                                                               \
        heap_entry##WIDTH##_t e = {                                            \
            .count      = pair->value.count,                                   \
            .size       = pair->value.length,                                  \
            .last_index = last,                                                \
            .key        = &pair->key,                                          \
            .value      = &pair->value                                         \
        };                                                                     \
        if (n < M) {                                                           \
            heap##WIDTH##_push(heap, &n, e, mode);                             \
        } else if (heap##WIDTH##_less(heap[0], e, mode)) {                     \
            heap##WIDTH##_replace_root(heap, n, e, mode);                      \
        }                                                                      \
    }                                                                          \
                                                                               \
    heap##WIDTH##_sort_desc(heap, n, mode);                                    \
    return n;                                                                  \
}

DEFINE_QUERY_TOP_M(64)
DEFINE_QUERY_TOP_M(128)
DEFINE_QUERY_TOP_M(192)
DEFINE_QUERY_TOP_M(256)

#undef DEFINE_QUERY_TOP_M

#endif
