/*!
    * md, a Max external
    * Copyright (c) 2026 Adam James Wilson. All rights reserved.
    * SPDX-License-Identifier: BSD 3-Clause Clear
    */

#ifndef REGISTER_H
#define REGISTER_H

static inline void extract_content64(t_md *x, int_fast64_t pos, uint_fast8_t k,
                                     content_key64_t *out)
{
    uint_fast16_t drop = 64 - (uint_fast16_t)k * (k - 1);
    out->size = k;
    out->pg   = ((uint64_t *)x->p)[pos] >> drop;
    out->tg   = ((uint64_t *)x->t)[pos] >> drop;
}
static inline void extract_content128(t_md *x, int_fast64_t pos, uint_fast8_t k,
                                      content_key128_t *out)
{
    uint_fast16_t drop = 128 - (uint_fast16_t)k * (k - 1);
    out->size = k;
    out->pg   = ((uint128_t *)x->p)[pos] >> drop;
    out->tg   = ((uint128_t *)x->t)[pos] >> drop;
}
static inline void extract_content192(t_md *x, int_fast64_t pos, uint_fast8_t k,
                                      content_key192_t *out)
{
    uint_fast16_t drop = 192 - (uint_fast16_t)k * (k - 1);
    out->size = k;
    out->pg   = ((uint192_t *)x->p)[pos] >> drop;
    out->tg   = ((uint192_t *)x->t)[pos] >> drop;
}
static inline void extract_content256(t_md *x, int_fast64_t pos, uint_fast8_t k,
                                      content_key256_t *out)
{
    uint_fast16_t drop = 256 - (uint_fast16_t)k * (k - 1);
    out->size = k;
    out->pg   = ((uint256_t *)x->p)[pos] >> drop;
    out->tg   = ((uint256_t *)x->t)[pos] >> drop;
}

#define DEFINE_REGISTER_MATCH(WIDTH)                                          \
static void register_match_##WIDTH(t_md *x, uint_fast64_t match_index,        \
                                   uint_fast8_t sg)                           \
{                                                                             \
                                                                              \
    content_key##WIDTH##_t key;                                               \
    extract_content##WIDTH(x, x->motive_index, sg, &key);                     \
                                                                              \
    if (subsumed_set##WIDTH##_get(x->sub##WIDTH, key) != NULL) {              \
        return;                                                               \
    }                                                                         \
                                                                              \
    cat_value_t *v = catalog##WIDTH##_get(x->cat##WIDTH, key);                \
    if (v != NULL) {                                                          \
        v->count++;                                                           \
        idx_array_push_back(v->indices, x->motive_index);                     \
        return;                                                               \
    }                                                                         \
                                                                              \
    cat_value_t fresh;                                                        \
    cat_value_init(&fresh);                                                   \
    fresh.count  = 2;                                                         \
    fresh.length = sg;                                                        \
    idx_array_push_back(fresh.indices, (int_fast64_t)match_index);            \
    idx_array_push_back(fresh.indices, x->motive_index);                      \
    catalog##WIDTH##_set_at(x->cat##WIDTH, key, fresh);                       \
    cat_value_clear(&fresh);                                                  \
                                                                              \
                                                                              \
    for (uint_fast8_t k = 3; k < sg; k++) {                                   \
        for (uint_fast8_t o = 0; o + k <= sg; o++) {                          \
            int_fast64_t src = (int_fast64_t)match_index + (int_fast64_t)o;   \
            if (src >= x->motive_index) continue;                             \
            content_key##WIDTH##_t sub;                                       \
            extract_content##WIDTH(x, src, k, &sub);                          \
            subsumed_set##WIDTH##_push(x->sub##WIDTH, sub);                   \
            catalog##WIDTH##_erase(x->cat##WIDTH, sub);                       \
        }                                                                     \
    }                                                                         \
}

DEFINE_REGISTER_MATCH(64)
DEFINE_REGISTER_MATCH(128)
DEFINE_REGISTER_MATCH(192)
DEFINE_REGISTER_MATCH(256)

static inline void register_match(t_md *x, uint_fast64_t match_index,
                                  uint_fast8_t sg)
{
    switch (x->N) {
        case 3  ... 8:  register_match_64 (x, match_index, sg); break;
        case 9  ... 11: register_match_128(x, match_index, sg); break;
        case 12 ... 14: register_match_192(x, match_index, sg); break;
        case 15 ... 16: register_match_256(x, match_index, sg); break;
    }
}

#endif
