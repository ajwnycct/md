/*!
    * md, a Max external
    * Copyright (c) 2026 Adam James Wilson. All rights reserved.
    * SPDX-License-Identifier: BSD 3-Clause Clear
    */

#ifndef MD_H
#define MD_H

#if defined(UNIT)
  #include "printbits.h"
  #include "unit.h"
#endif

#include <stdint.h>
#include "ext_time.h"
#include "ext_critical.h"
#include <math.h>
#include <limits.h>

#include "ext.h"
#include "ext_obex.h"

#include "bitpack.h"
#include "bitmatch.h"

#include "snap.h"
#include "prefix.h"

#include "defs.h"
#include "register.h"

#include "heap.h"
#include "query.h"
#include "hex.h"

void *md_new(t_symbol *s, long argc, t_atom *argv);
void md_free(t_md *x);
void md_assist(t_md *x, void *b, long m, long a, char *s);
void md_float(t_md *x, double cents);
void md_int(t_md *x, long cents);
void md_clear(t_md *x);
t_max_err md_max_length_set(t_md *x, t_object *attr, long argc, t_atom *argv);
t_max_err md_max_length_get(t_md *x, t_object *attr, long *argc, t_atom **argv);
t_max_err md_thresholds_set(t_md *x, t_object *attr, long argc, t_atom *argv);
t_max_err md_thresholds_get(t_md *x, t_object *attr, long *argc, t_atom **argv);

void md_maxcounts(t_md *x, long n);
void md_maxgraphs(t_md *x, long n);

void preprocess(t_md *x, long maximum_notes_per_motive);
void shift_left(double *input_data, long input_data_length);
void build_initial_words(t_md *x);
void build_next_words(t_md *x);
void find_repeats(t_md *x);
#ifdef UNIT
void md_run_tests(t_md *x);
#endif

static inline void prefix_index_register(t_md *x, int_fast64_t pos);

void *md_class;

#endif
