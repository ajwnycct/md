/*!
    * md, a Max external
    * Copyright (c) 2026 Adam James Wilson. All rights reserved.
    * SPDX-License-Identifier: BSD 3-Clause Clear
    */

#ifndef DEFS_H
#define DEFS_H

#include <stdint.h>
#include <stdbool.h>
#include "ext.h"
#include "m-dict.h"
#include "m-array.h"

#ifndef HEAP_MAX_M
#define HEAP_MAX_M 256
#endif

typedef struct _match
{
	uint_fast64_t motive_idx;
	uint_fast8_t subgraph_size;
	uint_fast64_t count;
} t_match;

DICT_DEF2(matches, int64_t, M_BASIC_OPLIST, t_match, M_POD_OPLIST);

ARRAY_DEF(idx_array, int_fast64_t, M_BASIC_OPLIST);
#define M_OPL_idx_array_t() ARRAY_OPLIST(idx_array, M_BASIC_OPLIST)

DICT_DEF2(prefix_index, uint64_t, M_BASIC_OPLIST, idx_array_t, M_OPL_idx_array_t());

typedef unsigned __int128 uint128_t;
typedef unsigned _BitInt(192) uint192_t;
typedef unsigned _BitInt(256) uint256_t;

typedef struct { uint_fast8_t size; uint64_t   pg; uint64_t   tg; } content_key64_t;
typedef struct { uint_fast8_t size; uint128_t  pg; uint128_t  tg; } content_key128_t;
typedef struct { uint_fast8_t size; uint192_t  pg; uint192_t  tg; } content_key192_t;
typedef struct { uint_fast8_t size; uint256_t  pg; uint256_t  tg; } content_key256_t;

#define FNV_MIX(h, x) ((h) * 0x9E3779B97F4A7C15ULL + (uint64_t)(x))

static inline size_t content_key64_hash(content_key64_t k)
{
    size_t h = k.size;
    h = FNV_MIX(h, k.pg);
    h = FNV_MIX(h, k.tg);
    return h;
}
static inline bool content_key64_equal(content_key64_t a, content_key64_t b)
{
    return a.size == b.size && a.pg == b.pg && a.tg == b.tg;
}

static inline size_t content_key128_hash(content_key128_t k)
{
    size_t h = k.size;
    h = FNV_MIX(h, (uint64_t)(k.pg >> 64));
    h = FNV_MIX(h, (uint64_t) k.pg);
    h = FNV_MIX(h, (uint64_t)(k.tg >> 64));
    h = FNV_MIX(h, (uint64_t) k.tg);
    return h;
}
static inline bool content_key128_equal(content_key128_t a, content_key128_t b)
{
    return a.size == b.size && a.pg == b.pg && a.tg == b.tg;
}

static inline size_t content_key192_hash(content_key192_t k)
{
    size_t h = k.size;
    h = FNV_MIX(h, (uint64_t)(k.pg >> 128));
    h = FNV_MIX(h, (uint64_t)(k.pg >>  64));
    h = FNV_MIX(h, (uint64_t) k.pg);
    h = FNV_MIX(h, (uint64_t)(k.tg >> 128));
    h = FNV_MIX(h, (uint64_t)(k.tg >>  64));
    h = FNV_MIX(h, (uint64_t) k.tg);
    return h;
}
static inline bool content_key192_equal(content_key192_t a, content_key192_t b)
{
    return a.size == b.size && a.pg == b.pg && a.tg == b.tg;
}

static inline size_t content_key256_hash(content_key256_t k)
{
    size_t h = k.size;
    h = FNV_MIX(h, (uint64_t)(k.pg >> 192));
    h = FNV_MIX(h, (uint64_t)(k.pg >> 128));
    h = FNV_MIX(h, (uint64_t)(k.pg >>  64));
    h = FNV_MIX(h, (uint64_t) k.pg);
    h = FNV_MIX(h, (uint64_t)(k.tg >> 192));
    h = FNV_MIX(h, (uint64_t)(k.tg >> 128));
    h = FNV_MIX(h, (uint64_t)(k.tg >>  64));
    h = FNV_MIX(h, (uint64_t) k.tg);
    return h;
}
static inline bool content_key256_equal(content_key256_t a, content_key256_t b)
{
    return a.size == b.size && a.pg == b.pg && a.tg == b.tg;
}

#define M_OPL_content_key64_t()  M_OPEXTEND(M_POD_OPLIST, \
    HASH(content_key64_hash),  EQUAL(content_key64_equal))
#define M_OPL_content_key128_t() M_OPEXTEND(M_POD_OPLIST, \
    HASH(content_key128_hash), EQUAL(content_key128_equal))
#define M_OPL_content_key192_t() M_OPEXTEND(M_POD_OPLIST, \
    HASH(content_key192_hash), EQUAL(content_key192_equal))
#define M_OPL_content_key256_t() M_OPEXTEND(M_POD_OPLIST, \
    HASH(content_key256_hash), EQUAL(content_key256_equal))

typedef struct {
    uint_fast64_t count;
    uint_fast8_t  length;
    idx_array_t   indices;
} cat_value_t;

static inline void cat_value_init(cat_value_t *v)
{
    v->count  = 0;
    v->length = 0;
    idx_array_init(v->indices);
}
static inline void cat_value_clear(cat_value_t *v)
{
    idx_array_clear(v->indices);
}
static inline void cat_value_init_set(cat_value_t *dst, const cat_value_t *src)
{
    dst->count  = src->count;
    dst->length = src->length;
    idx_array_init_set(dst->indices, src->indices);
}
static inline void cat_value_set(cat_value_t *dst, const cat_value_t *src)
{
    dst->count  = src->count;
    dst->length = src->length;
    idx_array_set(dst->indices, src->indices);
}

#define M_OPL_cat_value_t() (                              \
    TYPE(cat_value_t),                                     \
    INIT(API_2(cat_value_init)),                           \
    CLEAR(API_2(cat_value_clear)),                         \
    INIT_SET(API_6(cat_value_init_set)),                   \
    SET(API_6(cat_value_set)))

DICT_DEF2(catalog64,  content_key64_t,  M_OPL_content_key64_t(),  cat_value_t, M_OPL_cat_value_t())
DICT_DEF2(catalog128, content_key128_t, M_OPL_content_key128_t(), cat_value_t, M_OPL_cat_value_t())
DICT_DEF2(catalog192, content_key192_t, M_OPL_content_key192_t(), cat_value_t, M_OPL_cat_value_t())
DICT_DEF2(catalog256, content_key256_t, M_OPL_content_key256_t(), cat_value_t, M_OPL_cat_value_t())

DICT_SET_DEF(subsumed_set64,  content_key64_t,  M_OPL_content_key64_t())
DICT_SET_DEF(subsumed_set128, content_key128_t, M_OPL_content_key128_t())
DICT_SET_DEF(subsumed_set192, content_key192_t, M_OPL_content_key192_t())
DICT_SET_DEF(subsumed_set256, content_key256_t, M_OPL_content_key256_t())

typedef struct {
    uint_fast64_t                 count;
    uint_fast8_t                  size;
    int_fast64_t                  last_index;
    const content_key64_t        *key;
    const cat_value_t            *value;
} heap_entry64_t;

typedef struct {
    uint_fast64_t                 count;
    uint_fast8_t                  size;
    int_fast64_t                  last_index;
    const content_key128_t       *key;
    const cat_value_t            *value;
} heap_entry128_t;

typedef struct {
    uint_fast64_t                 count;
    uint_fast8_t                  size;
    int_fast64_t                  last_index;
    const content_key192_t       *key;
    const cat_value_t            *value;
} heap_entry192_t;

typedef struct {
    uint_fast64_t                 count;
    uint_fast8_t                  size;
    int_fast64_t                  last_index;
    const content_key256_t       *key;
    const cat_value_t            *value;
} heap_entry256_t;

typedef struct _md
{
	t_object ob;

	void *m_outlet;

	void *m_outlet_test;

	uint_fast8_t N;
  uint_fast8_t n;
  uint_fast8_t b;
  uint_fast8_t C;

  uint_fast16_t unsigned_integer_length;

	uint64_t note_count;
	int_fast64_t motive_index;

	double *pd;
	double *td;
	double previous_time;

	long maximum_notes;
	long maximum_maximum_length_motives;
	void *p;
	void *t;

	matches_t matches;

	uint_fast8_t min_subgraph_size;

	double thresholds[2];

	prefix_index_t prefix_index;

	catalog64_t      cat64;    subsumed_set64_t  sub64;
	catalog128_t     cat128;   subsumed_set128_t sub128;
	catalog192_t     cat192;   subsumed_set192_t sub192;
	catalog256_t     cat256;   subsumed_set256_t sub256;

	heap_entry64_t   heap64 [HEAP_MAX_M];
	heap_entry128_t  heap128[HEAP_MAX_M];
	heap_entry192_t  heap192[HEAP_MAX_M];
	heap_entry256_t  heap256[HEAP_MAX_M];

} t_md;

void (*translate_initial_to_bits)(t_md *x, double *notes, void *arr);
void (*translate_next_to_bits)(t_md *x, double *notes, void *arr);

#endif
