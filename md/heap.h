/*!
    * md, a Max external
    * Copyright (c) 2026 Adam James Wilson. All rights reserved.
    * SPDX-License-Identifier: BSD 3-Clause Clear
    */

#ifndef HEAP_H
#define HEAP_H

#include "defs.h"

#ifndef HEAP_MAX_M
#define HEAP_MAX_M 256
#endif

typedef enum {
    HEAP_CMP_COUNT = 0,
    HEAP_CMP_SIZE  = 1
} heap_cmp_mode_t;

#define DEFINE_HEAP_OPS(WIDTH)                                                 \
static inline int heap##WIDTH##_less(heap_entry##WIDTH##_t a,                  \
                                     heap_entry##WIDTH##_t b,                  \
                                     heap_cmp_mode_t mode)                     \
{                                                                              \
    if (mode == HEAP_CMP_COUNT) {                                              \
        if (a.count != b.count)           return a.count < b.count;            \
        if (a.last_index != b.last_index) return a.last_index < b.last_index;  \
        return a.size < b.size;                                                \
    } else {                                                                   \
        if (a.size != b.size)             return a.size < b.size;              \
        if (a.last_index != b.last_index) return a.last_index < b.last_index;  \
        return a.count < b.count;                                              \
    }                                                                          \
}                                                                              \
                                                                               \
static inline void heap##WIDTH##_sift_up(heap_entry##WIDTH##_t *h, int i,      \
                                         heap_cmp_mode_t mode)                 \
{                                                                              \
    heap_entry##WIDTH##_t v = h[i];                                            \
    while (i > 0) {                                                            \
        int parent = (i - 1) >> 1;                                             \
        if (!heap##WIDTH##_less(v, h[parent], mode)) break;                    \
        h[i] = h[parent];                                                      \
        i = parent;                                                            \
    }                                                                          \
    h[i] = v;                                                                  \
}                                                                              \
                                                                               \
static inline void heap##WIDTH##_sift_down(heap_entry##WIDTH##_t *h, int n,    \
                                           int i, heap_cmp_mode_t mode)        \
{                                                                              \
    heap_entry##WIDTH##_t v = h[i];                                            \
    for (;;) {                                                                 \
        int left  = 2 * i + 1;                                                 \
        int right = left + 1;                                                  \
        int min   = i;                                                         \
        heap_entry##WIDTH##_t best = v;                                        \
        if (left  < n && heap##WIDTH##_less(h[left],  best, mode)) {           \
            min = left;  best = h[left];                                       \
        }                                                                      \
        if (right < n && heap##WIDTH##_less(h[right], best, mode)) {           \
            min = right; best = h[right];                                      \
        }                                                                      \
        if (min == i) break;                                                   \
        h[i] = h[min];                                                         \
        i    = min;                                                            \
    }                                                                          \
    h[i] = v;                                                                  \
}                                                                              \
                                                                               \
static inline void heap##WIDTH##_push(heap_entry##WIDTH##_t *h, int *n,        \
                                      heap_entry##WIDTH##_t e,                 \
                                      heap_cmp_mode_t mode)                    \
{                                                                              \
    h[*n] = e;                                                                 \
    heap##WIDTH##_sift_up(h, *n, mode);                                        \
    (*n)++;                                                                    \
}                                                                              \
                                                                               \
static inline void heap##WIDTH##_replace_root(heap_entry##WIDTH##_t *h, int n, \
                                              heap_entry##WIDTH##_t e,         \
                                              heap_cmp_mode_t mode)            \
{                                                                              \
    h[0] = e;                                                                  \
    heap##WIDTH##_sift_down(h, n, 0, mode);                                    \
}                                                                              \
                                                                               \
static inline void heap##WIDTH##_sort_desc(heap_entry##WIDTH##_t *h, int n,    \
                                           heap_cmp_mode_t mode)               \
{                                                                              \
    for (int i = n - 1; i > 0; i--) {                                          \
        heap_entry##WIDTH##_t tmp = h[0];                                      \
        h[0] = h[i];                                                           \
        h[i] = tmp;                                                            \
        heap##WIDTH##_sift_down(h, i, 0, mode);                                \
    }                                                                          \
}

DEFINE_HEAP_OPS(64)
DEFINE_HEAP_OPS(128)
DEFINE_HEAP_OPS(192)
DEFINE_HEAP_OPS(256)

#undef DEFINE_HEAP_OPS

#endif
