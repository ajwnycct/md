/*!
    * md, a Max external
    * Copyright (c) 2026 Adam James Wilson. All rights reserved.
    * SPDX-License-Identifier: BSD 3-Clause Clear
    */

#ifndef BITPACK_H
#define BITPACK_H

#include <stdint.h>
#include "ext_post.h"
#include "defs.h"

#define DEFINE_INITIAL(WIDTH_BITS, TYPE, SUFFIX, ONE)                          \
static void translate_initial_to_##SUFFIX##_bits(                              \
        t_md *x, double *notes, void *arr)                                     \
{                                                                              \
    TYPE output = 0;                                                           \
    int pair_idx = 0;                                                          \
    for (int j = 1; j <= x->n; j++) {                                          \
        for (int i = 0; i < j; i++) {                                          \
            double d = notes[j] - notes[i];                                    \
            int sgn = (d > 0) - (d < 0);                                       \
            unsigned leftpos  = (WIDTH_BITS - 1) - 2u * (unsigned)pair_idx;    \
            unsigned rightpos = leftpos - 1u;                                  \
            switch (sgn) {                                                     \
                case  1: output |= ((TYPE)(ONE) << rightpos); break;           \
                case -1: output |= ((TYPE)(ONE) << leftpos);  break;           \
                case  0: output |= ((TYPE)(ONE) << leftpos)                    \
                                 | ((TYPE)(ONE) << rightpos); break;           \
            }                                                                  \
            pair_idx++;                                                        \
        }                                                                      \
    }                                                                          \
    ((TYPE *)arr)[0] = output;                                                 \
}

DEFINE_INITIAL(64,  uint64_t,  64,  1)
DEFINE_INITIAL(128, uint128_t, 128, 1)
DEFINE_INITIAL(192, uint192_t, 192, 1)
DEFINE_INITIAL(256, uint256_t, 256, 1)

#define DEFINE_NEXT(WIDTH_BITS, TYPE, SUFFIX, ONE)                             \
static void translate_next_to_##SUFFIX##_bits(                                 \
        t_md *x, double *notes, void *arr)                                     \
{                                                                              \
    TYPE old = ((TYPE *)arr)[x->motive_index];                                 \
    TYPE shifted = 0;                                                          \
    for (int k = 1; k <= x->n - 1; k++) {                                      \
        unsigned low_bit = (unsigned)(WIDTH_BITS - (k + 1) * (k + 2));         \
        TYPE mask = (((TYPE)(ONE) << (2u * (unsigned)k)) - (TYPE)(ONE))        \
                    << low_bit;                                                \
        shifted |= (old & mask) << (2u * (unsigned)(k + 1));                   \
    }                                                                          \
    int start_pair = x->n * (x->n - 1) / 2;                                    \
    for (int i = 0; i < x->n; i++) {                                           \
        double d = notes[x->n] - notes[i];                                     \
        int sgn = (d > 0) - (d < 0);                                           \
        int pair_idx = start_pair + i;                                         \
        unsigned leftpos  = (WIDTH_BITS - 1) - 2u * (unsigned)pair_idx;        \
        unsigned rightpos = leftpos - 1u;                                      \
        switch (sgn) {                                                         \
            case  1: shifted |= ((TYPE)(ONE) << rightpos); break;              \
            case -1: shifted |= ((TYPE)(ONE) << leftpos);  break;              \
            case  0: shifted |= ((TYPE)(ONE) << leftpos)                       \
                              | ((TYPE)(ONE) << rightpos); break;              \
        }                                                                      \
    }                                                                          \
    ((TYPE *)arr)[x->motive_index + 1] = shifted;                              \
}

DEFINE_NEXT(64,  uint64_t,  64,  1)
DEFINE_NEXT(128, uint128_t, 128, 1)
DEFINE_NEXT(192, uint192_t, 192, 1)
DEFINE_NEXT(256, uint256_t, 256, 1)

#endif
