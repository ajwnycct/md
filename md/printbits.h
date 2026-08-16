/*!
    * md, a Max external
    * Copyright (c) 2026 Adam James Wilson. All rights reserved.
    * SPDX-License-Identifier: BSD 3-Clause Clear
    */

#if defined(UNIT)

#ifndef PRINTBITS_H
#define PRINTBITS_H

#include <stdint.h>
#include "ext_post.h"
#include "defs.h"

static inline void print_bit_pairs(const uint64_t *words, int total_bits)
{
    char buf[384 + 1];
    char *out = buf;
    int total_pairs = total_bits / 2;

    for (int i = 0; i < total_pairs; i++) {
        int bit_hi_from_top = 2 * i;
        int bit_lo_from_top = 2 * i + 1;

        int word_idx = bit_hi_from_top / 64;
        int shift_hi = 63 - (bit_hi_from_top % 64);
        int shift_lo = 63 - (bit_lo_from_top % 64);

        int b_hi = (int)((words[word_idx] >> shift_hi) & 1ULL);
        int b_lo = (int)((words[word_idx] >> shift_lo) & 1ULL);

        *out++ = b_hi ? '1' : '0';
        *out++ = b_lo ? '1' : '0';
        if (i < total_pairs - 1) *out++ = '-';
    }
    *out = '\0';

    post("%s", buf);
}

static inline void print_bits(uint64_t num)
{
    uint64_t words[1] = { num };
    print_bit_pairs(words, 64);
}

static inline void print_bits128(uint128_t num)
{
    uint64_t words[2] = {
        (uint64_t)(num >> 64),
        (uint64_t)(num),
    };
    print_bit_pairs(words, 128);
}

static inline void print_bits192(uint192_t num)
{
    uint64_t words[3] = {
        (uint64_t)(num >> 128),
        (uint64_t)(num >>  64),
        (uint64_t)(num),
    };
    print_bit_pairs(words, 192);
}

static inline void print_bits256(uint256_t num)
{
    uint64_t words[4] = {
        (uint64_t)(num >> 192),
        (uint64_t)(num >> 128),
        (uint64_t)(num >>  64),
        (uint64_t)(num),
    };
    print_bit_pairs(words, 256);
}

#endif
#endif
