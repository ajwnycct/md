/*!
    * md, a Max external
    * Copyright (c) 2026 Adam James Wilson. All rights reserved.
    * SPDX-License-Identifier: BSD 3-Clause Clear
    */

#ifdef UNIT
#ifndef UNIT_H
#define UNIT_H

typedef struct _md t_md;

#include <time.h>
#include "md.h"

#define UNIT_MATCH_CAPTURE_MAX 1024
typedef struct {
    int_fast64_t i;
    int_fast64_t mi;
    int_fast8_t  sg;
} unit_captured_match_t;
extern unit_captured_match_t unit_captured_matches[UNIT_MATCH_CAPTURE_MAX];
extern int                   unit_captured_count;

void run_all_tests(t_md *x);

#endif
#endif
