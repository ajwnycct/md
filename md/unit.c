/*!
    * md, a Max external
    * Copyright (c) 2026 Adam James Wilson. All rights reserved.
    * SPDX-License-Identifier: BSD 3-Clause Clear
    */

#ifdef UNIT

#include "unit.h"

unit_captured_match_t unit_captured_matches[UNIT_MATCH_CAPTURE_MAX];
int unit_captured_count = 0;

#include <stdlib.h>
#include <string.h>
#include "ext_obex.h"

#define PERF_NOTES        18000
#define PERF_ITERS         1000
#define PERF_WARMUP         100
#define PERF_RAND_SEED       42

#define PERF_SNAP_PD       0.25
#define PERF_SNAP_TD        5.0

static void perf_process_next_note(t_md *x, double cents, double ioi_ms)
{
    x->pd[x->N] = cents;
    x->td[x->n] = ioi_ms;
    snap(x->pd, x->N + 1, PERF_SNAP_PD);
    snap(x->td, x->N,     PERF_SNAP_TD);
    build_next_words(x);
    shift_left(x->pd, x->N + 1);
    shift_left(x->td, x->N);
    find_repeats(x);
    prefix_index_register(x, x->motive_index);
}

static void perf_bootstrap(t_md *x, double *pitches, double *iois)
{
    for (uint_fast8_t i = 0; i < x->N; i++) {
        x->pd[i] = pitches[i];
        if (i > 0) x->td[i - 1] = iois[i - 1];
    }
    x->pd[x->N] = pitches[x->N];
    x->td[x->N - 1] = iois[x->N - 1];
    snap(x->pd, x->N + 1, PERF_SNAP_PD);
    snap(x->td, x->N,     PERF_SNAP_TD);
    build_initial_words(x);
    shift_left(x->pd, x->N + 1);
    shift_left(x->td, x->N);
    prefix_index_register(x, 0);
}

static double bench_last_note(t_md *x, uint_fast8_t N, uint64_t iters,
                              size_t *out_cat, size_t *out_sub, double *out_fill_ms)
{
    preprocess(x, N);

    double *pitches = (double *)sysmem_newptr(sizeof(double) * PERF_NOTES);
    double *iois    = (double *)sysmem_newptr(sizeof(double) * PERF_NOTES);
    srand(PERF_RAND_SEED + N);
    for (int i = 0; i < PERF_NOTES; i++) {
        pitches[i] = (double)(rand() % 128) * 100.0;
        iois[i]    = (double)(rand() % 4000);
    }

    double t_fill_start = systimer_gettime();
    perf_bootstrap(x, pitches, iois);

    for (int i = N + 1; i < PERF_NOTES - 1; i++) {
        perf_process_next_note(x, pitches[i], iois[i]);
    }
    double fill_ms = systimer_gettime() - t_fill_start;

    double pd_snap[17], td_snap[17];
    memcpy(pd_snap, x->pd, sizeof(double) * (N + 1));
    memcpy(td_snap, x->td, sizeof(double) * N);
    int_fast64_t saved_motive_index = x->motive_index;
    uint_fast8_t saved_min_sg       = x->min_subgraph_size;

    double last_cents = pitches[PERF_NOTES - 1];
    double last_ioi   = iois[PERF_NOTES - 1];

    for (int w = 0; w < PERF_WARMUP; w++) {
        memcpy(x->pd, pd_snap, sizeof(double) * (N + 1));
        memcpy(x->td, td_snap, sizeof(double) * N);
        x->pd[N]   = last_cents;
        x->td[x->n] = last_ioi;
        snap(x->pd, N + 1, PERF_SNAP_PD);
        snap(x->td, N,     PERF_SNAP_TD);
        build_next_words(x);
        find_repeats(x);
        x->motive_index      = saved_motive_index;
        x->min_subgraph_size = saved_min_sg;
    }

    double t_bench_start = systimer_gettime();
    for (uint64_t i = 0; i < iters; i++) {
        memcpy(x->pd, pd_snap, sizeof(double) * (N + 1));
        memcpy(x->td, td_snap, sizeof(double) * N);
        x->pd[N]    = last_cents;
        x->td[x->n] = last_ioi;
        snap(x->pd, N + 1, PERF_SNAP_PD);
        snap(x->td, N,     PERF_SNAP_TD);
        build_next_words(x);
        find_repeats(x);
        x->motive_index      = saved_motive_index;
        x->min_subgraph_size = saved_min_sg;
    }
    double bench_ms = systimer_gettime() - t_bench_start;
    double per_iter_us = (bench_ms / (double)iters) * 1000.0;

    size_t cat = 0, sub = 0;
    switch (x->unsigned_integer_length) {
        case 64:  cat = catalog64_size(x->cat64);   sub = subsumed_set64_size(x->sub64);   break;
        case 128: cat = catalog128_size(x->cat128); sub = subsumed_set128_size(x->sub128); break;
        case 192: cat = catalog192_size(x->cat192); sub = subsumed_set192_size(x->sub192); break;
        case 256: cat = catalog256_size(x->cat256); sub = subsumed_set256_size(x->sub256); break;
    }

    sysmem_freeptr(pitches);
    sysmem_freeptr(iois);

    *out_cat     = cat;
    *out_sub     = sub;
    *out_fill_ms = fill_ms;
    return per_iter_us;
}

static double bench_query(t_md *x, int M, heap_cmp_mode_t mode,
                          uint64_t iters, int *out_rows)
{
    int rows = 0;

#define MD_RUN_QUERY()                                                        \
    do {                                                                      \
        switch (x->unsigned_integer_length) {                                 \
            case 64:  rows = query_top_m_64 (x, x->cat64,  M, mode,           \
                                             x->heap64);  break;              \
            case 128: rows = query_top_m_128(x, x->cat128, M, mode,           \
                                             x->heap128); break;              \
            case 192: rows = query_top_m_192(x, x->cat192, M, mode,           \
                                             x->heap192); break;              \
            case 256: rows = query_top_m_256(x, x->cat256, M, mode,           \
                                             x->heap256); break;              \
            default:  rows = 0; break;                                        \
        }                                                                     \
    } while (0)

    for (int w = 0; w < PERF_WARMUP; w++) {
        MD_RUN_QUERY();
    }

    double t_start = systimer_gettime();
    for (uint64_t i = 0; i < iters; i++) {
        MD_RUN_QUERY();
    }
    double bench_ms = systimer_gettime() - t_start;

#undef MD_RUN_QUERY

    if (out_rows) *out_rows = rows;
    return (bench_ms / (double)iters) * 1000.0;
}

#define PERF_QUERY_M 10

#define PERF_EMIT_ITERS   100
#define PERF_EMIT_WARMUP   10

static double bench_query_full(t_md *x, int M, heap_cmp_mode_t mode,
                               uint64_t iters)
{
    for (uint64_t w = 0; w < PERF_EMIT_WARMUP; w++) {
        if (mode == HEAP_CMP_COUNT) md_maxcounts(x, (long)M);
        else                        md_maxgraphs(x, (long)M);
    }

    double t_start = systimer_gettime();
    for (uint64_t i = 0; i < iters; i++) {
        if (mode == HEAP_CMP_COUNT) md_maxcounts(x, (long)M);
        else                        md_maxgraphs(x, (long)M);
    }
    double bench_ms = systimer_gettime() - t_start;

    return (bench_ms / (double)iters) * 1000.0;
}

static void test_perf_find_repeats(t_md *x, uint64_t iters)
{
    post("---- perf: 18000th-note pipeline avg (snap+pack+find_repeats+register_match) ----");
    post("  query columns (avg us, maxcounts %d / maxgraphs %d):", PERF_QUERY_M, PERF_QUERY_M);
    post("    q* = selection only (catalog walk + min-heap)");
    post("    e* = end-to-end handler, incl. hex formatting + clear/store emission");
    post("  N | fill (ms) | per-iter (us) | qcount | qgraph | ecount | egraph | catalog | subsumed");

    for (uint_fast8_t N = 3; N <= 16; N++) {
        size_t cat, sub;
        double fill_ms;
        double per_iter_us = bench_last_note(x, N, iters, &cat, &sub, &fill_ms);
        int rows_c = 0, rows_g = 0;
        double q_count_us = bench_query(x, PERF_QUERY_M, HEAP_CMP_COUNT,
                                        iters, &rows_c);
        double q_graph_us = bench_query(x, PERF_QUERY_M, HEAP_CMP_SIZE,
                                        iters, &rows_g);
        double e_count_us = bench_query_full(x, PERF_QUERY_M, HEAP_CMP_COUNT,
                                            PERF_EMIT_ITERS);
        double e_graph_us = bench_query_full(x, PERF_QUERY_M, HEAP_CMP_SIZE,
                                            PERF_EMIT_ITERS);
        post("  %2u | %9.2f | %13.3f | %6.3f | %6.3f | %6.3f | %6.3f | %7zu | %8zu",
             (unsigned)N, fill_ms, per_iter_us, q_count_us, q_graph_us,
             e_count_us, e_graph_us, cat, sub);
        if (rows_c != rows_g) {
            post("    note: maxcounts returned %d rows, maxgraphs %d",
                 rows_c, rows_g);
        }

        t_atom av[10];
        atom_setsym (av + 0, gensym("perf_last_note"));
        atom_setlong(av + 1, (t_atom_long)N);
        atom_setfloat(av + 2, fill_ms);
        atom_setfloat(av + 3, per_iter_us);
        atom_setlong(av + 4, (t_atom_long)cat);
        atom_setlong(av + 5, (t_atom_long)sub);
        atom_setfloat(av + 6, q_count_us);
        atom_setfloat(av + 7, q_graph_us);
        atom_setfloat(av + 8, e_count_us);
        atom_setfloat(av + 9, e_graph_us);
        outlet_list(x->m_outlet_test, NULL, 10, av);

    }
}

#include "bach.h"

static int test_pd_td_fill_shift(t_md *x)
{
    post("---- test_pd_td_fill_shift ----");
    int failures = 0;

    double saved_thr[2] = { x->thresholds[0], x->thresholds[1] };
    x->thresholds[0] = 0.25;
    x->thresholds[1] = 5.0;

    for (uint_fast8_t N = 3; N <= 16; N++) {
        preprocess(x, N);
        int n_notes = N + 3;
        double base = 6000.0;
        double step = 100.0;
        int per_N_failures = 0;

        struct timespec sleep_100ms = { .tv_sec = 0, .tv_nsec = 100L * 1000L * 1000L };

        for (int k = 0; k < n_notes; k++) {
            double cents = base + (double)k * step;
            md_float(x, cents);
            if (k < n_notes - 1) nanosleep(&sleep_100ms, NULL);

            if (k < N) {
                if (x->pd[k] != cents) {
                    post("  N=%u k=%d: pd[%d]=%f expected %f",
                         (unsigned)N, k, k, x->pd[k], cents);
                    per_N_failures++;
                }
                if (k >= 1 && !(x->td[k-1] > 0.0)) {
                    post("  N=%u k=%d: td[%d]=%f expected > 0",
                         (unsigned)N, k, k-1, x->td[k-1]);
                    per_N_failures++;
                }
            } else {
                for (int i = 0; i < N; i++) {
                    double expected = base + (double)(k - N + 1 + i) * step;
                    if (x->pd[i] != expected) {
                        post("  N=%u k=%d: pd[%d]=%f expected %f",
                             (unsigned)N, k, i, x->pd[i], expected);
                        per_N_failures++;
                    }
                }
                double expected_last = base + (double)k * step;
                if (x->pd[N] != expected_last) {
                    post("  N=%u k=%d: pd[N=%d]=%f expected %f (stale duplicate of pd[N-1])",
                         (unsigned)N, k, N, x->pd[N], expected_last);
                    per_N_failures++;
                }
                for (int i = 0; i < N; i++) {
                    if (!(x->td[i] > 0.0)) {
                        post("  N=%u k=%d: td[%d]=%f expected > 0",
                             (unsigned)N, k, i, x->td[i]);
                        per_N_failures++;
                    }
                }
            }
        }

        post("  N=%2u: %s (%d failure%s)",
             (unsigned)N,
             per_N_failures == 0 ? "PASS" : "FAIL",
             per_N_failures,
             per_N_failures == 1 ? "" : "s");
        failures += per_N_failures;

        t_atom av[3];
        atom_setsym (av + 0, gensym("pd_td_fill_shift"));
        atom_setlong(av + 1, (t_atom_long)N);
        atom_setlong(av + 2, (t_atom_long)per_N_failures);
        outlet_list(x->m_outlet_test, NULL, 3, av);
    }

    x->thresholds[0] = saved_thr[0];
    x->thresholds[1] = saved_thr[1];

    post("---- test_pd_td_fill_shift: %s (total=%d) ----",
         failures == 0 ? "PASSED" : "FAILED", failures);
    return failures;
}

static const double test_cents_arr[18] = {
    400.0, 600.0, 400.0, 600.0, 400.0, 600.0, 400.0, 600.0, 400.0,
    600.0, 400.0, 600.0, 400.0, 600.0, 400.0, 600.0, 400.0, 600.0,
};

static const uint64_t expected_initial_64[9] = {
    [ 3] = 0x7800000000000000ULL,
    [ 4] = 0x79d0000000000000ULL,
    [ 5] = 0x79dee00000000000ULL,
    [ 6] = 0x79dee77400000000ULL,
    [ 7] = 0x79dee777bb800000ULL,
    [ 8] = 0x79dee777bb9ddd00ULL,
};

static const uint128_t expected_initial_128[12] = {
    [ 9] = 0x79dee777bb9dddeeee00000000000000UWB,
    [10] = 0x79dee777bb9dddeeee77774000000000UWB,
    [11] = 0x79dee777bb9dddeeee77777bbbb80000UWB,
};

static const uint192_t expected_initial_192[15] = {
    [12] = 0x79dee777bb9dddeeee77777bbbb9ddddd000000000000000UWB,
    [13] = 0x79dee777bb9dddeeee77777bbbb9dddddeeeeee000000000UWB,
    [14] = 0x79dee777bb9dddeeee77777bbbb9dddddeeeeee777777400UWB,
};

static const uint256_t expected_initial_256[17] = {
    [15] = 0x79dee777bb9dddeeee77777bbbb9dddddeeeeee7777777bbbbbb800000000000UWB,
    [16] = 0x79dee777bb9dddeeee77777bbbb9dddddeeeeee7777777bbbbbb9ddddddd0000UWB,
};

static const uint64_t expected_next_64[9] = {
    [ 3] = 0xb400000000000000ULL,
    [ 4] = 0xb6e0000000000000ULL,
    [ 5] = 0xb6edd00000000000ULL,
    [ 6] = 0xb6eddbb800000000ULL,
    [ 7] = 0xb6eddbbb77400000ULL,
    [ 8] = 0xb6eddbbb776eee00ULL,
};

static const uint128_t expected_next_128[12] = {
    [ 9] = 0xb6eddbbb776eeedddd00000000000000UWB,
    [10] = 0xb6eddbbb776eeeddddbbbb8000000000UWB,
    [11] = 0xb6eddbbb776eeeddddbbbbb777740000UWB,
};

static const uint192_t expected_next_192[15] = {
    [12] = 0xb6eddbbb776eeeddddbbbbb77776eeeee000000000000000UWB,
    [13] = 0xb6eddbbb776eeeddddbbbbb77776eeeeedddddd000000000UWB,
    [14] = 0xb6eddbbb776eeeddddbbbbb77776eeeeeddddddbbbbbb800UWB,
};

static const uint256_t expected_next_256[17] = {
    [15] = 0xb6eddbbb776eeeddddbbbbb77776eeeeeddddddbbbbbbb777777400000000000UWB,
    [16] = 0xb6eddbbb776eeeddddbbbbb77776eeeeeddddddbbbbbbb7777776eeeeeee0000UWB,
};

static int test_translate_bits(t_md *x)
{
    post("---- test_translate_bits ----");
    int failures = 0;

    double saved_thr[2] = { x->thresholds[0], x->thresholds[1] };
    x->thresholds[0] = 0.25;
    x->thresholds[1] = 5.0;

    for (uint_fast8_t N = 3; N <= 16; N++) {
        preprocess(x, N);
        int n_notes = N + 2;
        for (int k = 0; k < n_notes; k++) {
            md_float(x, test_cents_arr[k]);
        }

        int per_N_failures = 0;
        switch (x->unsigned_integer_length) {
            case 64: {
                uint64_t a0 = ((uint64_t *)x->p)[0];
                uint64_t a1 = ((uint64_t *)x->p)[1];
                post("  N=%u translate_initial:", (unsigned)N);
                post("    expected:"); print_bits(expected_initial_64[N]);
                post("    actual:  "); print_bits(a0);
                post("  N=%u translate_next:", (unsigned)N);
                post("    expected:"); print_bits(expected_next_64[N]);
                post("    actual:  "); print_bits(a1);
                if (a0 != expected_initial_64[N]) per_N_failures++;
                if (a1 != expected_next_64[N])    per_N_failures++;
                break;
            }
            case 128: {
                uint128_t a0 = ((uint128_t *)x->p)[0];
                uint128_t a1 = ((uint128_t *)x->p)[1];
                post("  N=%u translate_initial:", (unsigned)N);
                post("    expected:"); print_bits128(expected_initial_128[N]);
                post("    actual:  "); print_bits128(a0);
                post("  N=%u translate_next:", (unsigned)N);
                post("    expected:"); print_bits128(expected_next_128[N]);
                post("    actual:  "); print_bits128(a1);
                if (a0 != expected_initial_128[N]) per_N_failures++;
                if (a1 != expected_next_128[N])    per_N_failures++;
                break;
            }
            case 192: {
                uint192_t a0 = ((uint192_t *)x->p)[0];
                uint192_t a1 = ((uint192_t *)x->p)[1];
                post("  N=%u translate_initial:", (unsigned)N);
                post("    expected:"); print_bits192(expected_initial_192[N]);
                post("    actual:  "); print_bits192(a0);
                post("  N=%u translate_next:", (unsigned)N);
                post("    expected:"); print_bits192(expected_next_192[N]);
                post("    actual:  "); print_bits192(a1);
                if (a0 != expected_initial_192[N]) per_N_failures++;
                if (a1 != expected_next_192[N])    per_N_failures++;
                break;
            }
            case 256: {
                uint256_t a0 = ((uint256_t *)x->p)[0];
                uint256_t a1 = ((uint256_t *)x->p)[1];
                post("  N=%u translate_initial:", (unsigned)N);
                post("    expected:"); print_bits256(expected_initial_256[N]);
                post("    actual:  "); print_bits256(a0);
                post("  N=%u translate_next:", (unsigned)N);
                post("    expected:"); print_bits256(expected_next_256[N]);
                post("    actual:  "); print_bits256(a1);
                if (a0 != expected_initial_256[N]) per_N_failures++;
                if (a1 != expected_next_256[N])    per_N_failures++;
                break;
            }
        }

        post("  N=%2u: %s (%d failure%s)",
             (unsigned)N,
             per_N_failures == 0 ? "PASS" : "FAIL",
             per_N_failures,
             per_N_failures == 1 ? "" : "s");
        failures += per_N_failures;

        t_atom av[3];
        atom_setsym (av + 0, gensym("translate_bits"));
        atom_setlong(av + 1, (t_atom_long)N);
        atom_setlong(av + 2, (t_atom_long)per_N_failures);
        outlet_list(x->m_outlet_test, NULL, 3, av);
    }

    x->thresholds[0] = saved_thr[0];
    x->thresholds[1] = saved_thr[1];

    post("---- test_translate_bits: %s (total=%d) ----",
         failures == 0 ? "PASSED" : "FAILED", failures);
    return failures;
}

static const double seq1_giant_steps_16bars_pitches[26] = {
    78.0, 74.0, 71.0, 67.0, 70.0, 71.0, 69.0, 74.0, 70.0, 67.0, 63.0, 66.0, 67.0, 65.0, 70.0, 71.0, 69.0, 74.0, 75.0, 75.0, 78.0, 79.0, 79.0, 82.0, 78.0, 78.0
};
static const double seq1_giant_steps_16bars_times[26] = {
    2.0, 2.0, 2.0, 1.5, 4.5, 1.5, 2.5, 2.0, 2.0, 2.0, 1.5, 4.5, 2.0, 1.5, 4.5, 2.0, 1.5, 4.5, 2.0, 1.5, 4.5, 2.0, 1.5, 4.5, 1.5, 2.5
};

static const double seq2_giant_steps_32bars_pitches[52] = {
    78.0, 74.0, 71.0, 67.0, 70.0, 71.0, 69.0, 74.0, 70.0, 67.0, 63.0, 66.0, 67.0, 65.0, 70.0, 71.0, 69.0, 74.0, 75.0, 75.0, 78.0, 79.0, 79.0, 82.0, 78.0, 78.0, 78.0, 74.0, 71.0, 67.0, 70.0, 71.0, 69.0, 74.0, 70.0, 67.0, 63.0, 66.0, 67.0, 65.0, 70.0, 71.0, 69.0, 74.0, 75.0, 75.0, 78.0, 79.0, 79.0, 82.0, 78.0, 78.0
};
static const double seq2_giant_steps_32bars_times[52] = {
    2.0, 2.0, 2.0, 1.5, 4.5, 1.5, 2.5, 2.0, 2.0, 2.0, 1.5, 4.5, 2.0, 1.5, 4.5, 2.0, 1.5, 4.5, 2.0, 1.5, 4.5, 2.0, 1.5, 4.5, 1.5, 2.5, 2.0, 2.0, 2.0, 1.5, 4.5, 1.5, 2.5, 2.0, 2.0, 2.0, 1.5, 4.5, 2.0, 1.5, 4.5, 2.0, 1.5, 4.5, 2.0, 1.5, 4.5, 2.0, 1.5, 4.5, 1.5, 2.5
};

static const double seq3_case1_test_pitches[21] = {
    65.0, 69.0, 65.0, 69.0, 62.0, 62.0, 62.0, 65.0, 69.0, 65.0, 69.0, 62.0, 74.0, 74.0, 65.0, 69.0, 62.0, 77.0, 76.0, 76.0, 76.0
};
static const double seq3_case1_test_times[21] = {
    2.0, 2.0, 2.0, 1.0, 2.0, 2.0, 3.0, 2.0, 2.0, 2.0, 1.0, 2.0, 3.0, 2.0, 2.0, 1.0, 2.0, 1.0, 1.0, 3.0, 4.0
};

static const double seq4_supergraph_erase_test_pitches[21] = {
    62.0, 62.0, 65.0, 69.0, 65.0, 69.0, 62.0, 64.0, 64.0, 65.0, 69.0, 62.0, 74.0, 74.0, 76.0, 64.0, 65.0, 69.0, 62.0, 77.0, 76.0
};
static const double seq4_supergraph_erase_test_times[21] = {
    2.0, 3.0, 2.0, 2.0, 2.0, 1.0, 2.0, 2.0, 2.0, 2.0, 1.0, 2.0, 3.0, 2.0, 4.0, 2.0, 2.0, 1.0, 2.0, 1.0, 2.0
};

typedef struct {
    int16_t i;
    int16_t mi;
    int8_t  sg;
    int8_t  case_type;
    int8_t  dest;
    int16_t final_count;
} unit_expected_match_t;

static const unit_expected_match_t expected_seq1_N3[10] = {
    {   0,   7,  3, 0, 0,  2 },
    {   1,   8,  3, 0, 0,  2 },
    {   2,   9,  3, 0, 0,  2 },
    {  10,  13,  3, 0, 0,  4 },
    {  11,  14,  3, 0, 0,  2 },
    {  12,  15,  3, 0, 0,  2 },
    {  13,  16,  3, 1, 0,  4 },
    {  16,  19,  3, 1, 0,  4 },
    {  17,  20,  3, 0, 0,  2 },
    {  18,  21,  3, 0, 0,  2 },
};
#define EXPECTED_SEQ1_N3_LEN 10
#define EXPECTED_SEQ1_N3_CATALOG_SIZE 8

static const unit_expected_match_t expected_seq1_N4[7] = {
    {   0,   7,  4, 0, 0,  2 },
    {   1,   8,  4, 0, 0,  2 },
    {  10,  13,  4, 0, 0,  2 },
    {  11,  14,  4, 0, 0,  2 },
    {  12,  15,  4, 0, 0,  2 },
    {  16,  19,  4, 0, 0,  2 },
    {  17,  20,  4, 0, 0,  2 },
};
#define EXPECTED_SEQ1_N4_LEN 7
#define EXPECTED_SEQ1_N4_CATALOG_SIZE 7

static const unit_expected_match_t expected_seq1_N5[4] = {
    {   0,   7,  5, 0, 0,  2 },
    {  10,  13,  5, 0, 0,  2 },
    {  11,  14,  5, 0, 0,  2 },
    {  16,  19,  5, 0, 0,  2 },
};
#define EXPECTED_SEQ1_N5_LEN 4
#define EXPECTED_SEQ1_N5_CATALOG_SIZE 4

static const unit_expected_match_t expected_seq1_N6[3] = {
    {   0,   7,  5, 0, 0,  2 },
    {  10,  13,  6, 0, 0,  2 },
    {  16,  19,  5, 0, 0,  2 },
};
#define EXPECTED_SEQ1_N6_LEN 3
#define EXPECTED_SEQ1_N6_CATALOG_SIZE 3

static const unit_expected_match_t expected_seq2_N7[23] = {
    {   0,   7,  5, 0, 1,  3 },
    {  10,  13,  6, 0, 1,  2 },
    {  16,  19,  5, 0, 1,  2 },
    {   7,  26,  5, 1, 1,  3 },
    {   0,  26,  7, 0, 0,  2 },
    {   1,  27,  7, 0, 0,  2 },
    {   2,  28,  7, 0, 0,  2 },
    {   3,  29,  7, 0, 0,  2 },
    {   4,  30,  7, 0, 0,  2 },
    {   5,  31,  7, 0, 0,  2 },
    {   6,  32,  7, 0, 0,  2 },
    {   7,  33,  7, 0, 0,  2 },
    {   8,  34,  7, 0, 0,  2 },
    {   9,  35,  7, 0, 0,  2 },
    {  10,  36,  7, 0, 0,  2 },
    {  11,  37,  7, 0, 0,  2 },
    {  12,  38,  7, 0, 0,  2 },
    {  13,  39,  7, 0, 0,  2 },
    {  14,  40,  7, 0, 0,  2 },
    {  15,  41,  7, 0, 0,  2 },
    {  16,  42,  7, 0, 0,  2 },
    {  17,  43,  7, 0, 0,  2 },
    {  18,  44,  7, 0, 0,  2 },
};
#define EXPECTED_SEQ2_N7_LEN 23
#define EXPECTED_SEQ2_N7_CATALOG_SIZE 19

static const unit_expected_match_t expected_seq2_N8[22] = {
    {   0,   7,  5, 0, 1,  3 },
    {  10,  13,  6, 0, 1,  2 },
    {  16,  19,  5, 0, 1,  2 },
    {   7,  26,  5, 1, 1,  3 },
    {   0,  26,  8, 0, 0,  2 },
    {   1,  27,  8, 0, 0,  2 },
    {   2,  28,  8, 0, 0,  2 },
    {   3,  29,  8, 0, 0,  2 },
    {   4,  30,  8, 0, 0,  2 },
    {   5,  31,  8, 0, 0,  2 },
    {   6,  32,  8, 0, 0,  2 },
    {   7,  33,  8, 0, 0,  2 },
    {   8,  34,  8, 0, 0,  2 },
    {   9,  35,  8, 0, 0,  2 },
    {  10,  36,  8, 0, 0,  2 },
    {  11,  37,  8, 0, 0,  2 },
    {  12,  38,  8, 0, 0,  2 },
    {  13,  39,  8, 0, 0,  2 },
    {  14,  40,  8, 0, 0,  2 },
    {  15,  41,  8, 0, 0,  2 },
    {  16,  42,  8, 0, 0,  2 },
    {  17,  43,  8, 0, 0,  2 },
};
#define EXPECTED_SEQ2_N8_LEN 22
#define EXPECTED_SEQ2_N8_CATALOG_SIZE 18

static const unit_expected_match_t expected_seq2_N9[21] = {
    {   0,   7,  5, 0, 1,  3 },
    {  10,  13,  6, 0, 1,  2 },
    {  16,  19,  5, 0, 1,  2 },
    {   7,  26,  5, 1, 1,  3 },
    {   0,  26,  9, 0, 0,  2 },
    {   1,  27,  9, 0, 0,  2 },
    {   2,  28,  9, 0, 0,  2 },
    {   3,  29,  9, 0, 0,  2 },
    {   4,  30,  9, 0, 0,  2 },
    {   5,  31,  9, 0, 0,  2 },
    {   6,  32,  9, 0, 0,  2 },
    {   7,  33,  9, 0, 0,  2 },
    {   8,  34,  9, 0, 0,  2 },
    {   9,  35,  9, 0, 0,  2 },
    {  10,  36,  9, 0, 0,  2 },
    {  11,  37,  9, 0, 0,  2 },
    {  12,  38,  9, 0, 0,  2 },
    {  13,  39,  9, 0, 0,  2 },
    {  14,  40,  9, 0, 0,  2 },
    {  15,  41,  9, 0, 0,  2 },
    {  16,  42,  9, 0, 0,  2 },
};
#define EXPECTED_SEQ2_N9_LEN 21
#define EXPECTED_SEQ2_N9_CATALOG_SIZE 17

static const unit_expected_match_t expected_seq2_N10[20] = {
    {   0,   7,  5, 0, 1,  3 },
    {  10,  13,  6, 0, 1,  2 },
    {  16,  19,  5, 0, 1,  2 },
    {   7,  26,  5, 1, 1,  3 },
    {   0,  26, 10, 0, 0,  2 },
    {   1,  27, 10, 0, 0,  2 },
    {   2,  28, 10, 0, 0,  2 },
    {   3,  29, 10, 0, 0,  2 },
    {   4,  30, 10, 0, 0,  2 },
    {   5,  31, 10, 0, 0,  2 },
    {   6,  32, 10, 0, 0,  2 },
    {   7,  33, 10, 0, 0,  2 },
    {   8,  34, 10, 0, 0,  2 },
    {   9,  35, 10, 0, 0,  2 },
    {  10,  36, 10, 0, 0,  2 },
    {  11,  37, 10, 0, 0,  2 },
    {  12,  38, 10, 0, 0,  2 },
    {  13,  39, 10, 0, 0,  2 },
    {  14,  40, 10, 0, 0,  2 },
    {  15,  41, 10, 0, 0,  2 },
};
#define EXPECTED_SEQ2_N10_LEN 20
#define EXPECTED_SEQ2_N10_CATALOG_SIZE 16

static const unit_expected_match_t expected_seq2_N11[19] = {
    {   0,   7,  5, 0, 1,  3 },
    {  10,  13,  6, 0, 1,  2 },
    {  16,  19,  5, 0, 1,  2 },
    {   7,  26,  5, 1, 1,  3 },
    {   0,  26, 11, 0, 0,  2 },
    {   1,  27, 11, 0, 0,  2 },
    {   2,  28, 11, 0, 0,  2 },
    {   3,  29, 11, 0, 0,  2 },
    {   4,  30, 11, 0, 0,  2 },
    {   5,  31, 11, 0, 0,  2 },
    {   6,  32, 11, 0, 0,  2 },
    {   7,  33, 11, 0, 0,  2 },
    {   8,  34, 11, 0, 0,  2 },
    {   9,  35, 11, 0, 0,  2 },
    {  10,  36, 11, 0, 0,  2 },
    {  11,  37, 11, 0, 0,  2 },
    {  12,  38, 11, 0, 0,  2 },
    {  13,  39, 11, 0, 0,  2 },
    {  14,  40, 11, 0, 0,  2 },
};
#define EXPECTED_SEQ2_N11_LEN 19
#define EXPECTED_SEQ2_N11_CATALOG_SIZE 15

static const unit_expected_match_t expected_seq2_N12[18] = {
    {   0,   7,  5, 0, 1,  3 },
    {  10,  13,  6, 0, 1,  2 },
    {  16,  19,  5, 0, 1,  2 },
    {   7,  26,  5, 1, 1,  3 },
    {   0,  26, 12, 0, 0,  2 },
    {   1,  27, 12, 0, 0,  2 },
    {   2,  28, 12, 0, 0,  2 },
    {   3,  29, 12, 0, 0,  2 },
    {   4,  30, 12, 0, 0,  2 },
    {   5,  31, 12, 0, 0,  2 },
    {   6,  32, 12, 0, 0,  2 },
    {   7,  33, 12, 0, 0,  2 },
    {   8,  34, 12, 0, 0,  2 },
    {   9,  35, 12, 0, 0,  2 },
    {  10,  36, 12, 0, 0,  2 },
    {  11,  37, 12, 0, 0,  2 },
    {  12,  38, 12, 0, 0,  2 },
    {  13,  39, 12, 0, 0,  2 },
};
#define EXPECTED_SEQ2_N12_LEN 18
#define EXPECTED_SEQ2_N12_CATALOG_SIZE 14

static const unit_expected_match_t expected_seq2_N13[17] = {
    {   0,   7,  5, 0, 1,  3 },
    {  10,  13,  6, 0, 1,  2 },
    {  16,  19,  5, 0, 1,  2 },
    {   7,  26,  5, 1, 1,  3 },
    {   0,  26, 13, 0, 0,  2 },
    {   1,  27, 13, 0, 0,  2 },
    {   2,  28, 13, 0, 0,  2 },
    {   3,  29, 13, 0, 0,  2 },
    {   4,  30, 13, 0, 0,  2 },
    {   5,  31, 13, 0, 0,  2 },
    {   6,  32, 13, 0, 0,  2 },
    {   7,  33, 13, 0, 0,  2 },
    {   8,  34, 13, 0, 0,  2 },
    {   9,  35, 13, 0, 0,  2 },
    {  10,  36, 13, 0, 0,  2 },
    {  11,  37, 13, 0, 0,  2 },
    {  12,  38, 13, 0, 0,  2 },
};
#define EXPECTED_SEQ2_N13_LEN 17
#define EXPECTED_SEQ2_N13_CATALOG_SIZE 13

static const unit_expected_match_t expected_seq2_N14[16] = {
    {   0,   7,  5, 0, 1,  3 },
    {  10,  13,  6, 0, 1,  2 },
    {  16,  19,  5, 0, 1,  2 },
    {   7,  26,  5, 1, 1,  3 },
    {   0,  26, 14, 0, 0,  2 },
    {   1,  27, 14, 0, 0,  2 },
    {   2,  28, 14, 0, 0,  2 },
    {   3,  29, 14, 0, 0,  2 },
    {   4,  30, 14, 0, 0,  2 },
    {   5,  31, 14, 0, 0,  2 },
    {   6,  32, 14, 0, 0,  2 },
    {   7,  33, 14, 0, 0,  2 },
    {   8,  34, 14, 0, 0,  2 },
    {   9,  35, 14, 0, 0,  2 },
    {  10,  36, 14, 0, 0,  2 },
    {  11,  37, 14, 0, 0,  2 },
};
#define EXPECTED_SEQ2_N14_LEN 16
#define EXPECTED_SEQ2_N14_CATALOG_SIZE 12

static const unit_expected_match_t expected_seq2_N15[15] = {
    {   0,   7,  5, 0, 1,  3 },
    {  10,  13,  6, 0, 1,  2 },
    {  16,  19,  5, 0, 1,  2 },
    {   7,  26,  5, 1, 1,  3 },
    {   0,  26, 15, 0, 0,  2 },
    {   1,  27, 15, 0, 0,  2 },
    {   2,  28, 15, 0, 0,  2 },
    {   3,  29, 15, 0, 0,  2 },
    {   4,  30, 15, 0, 0,  2 },
    {   5,  31, 15, 0, 0,  2 },
    {   6,  32, 15, 0, 0,  2 },
    {   7,  33, 15, 0, 0,  2 },
    {   8,  34, 15, 0, 0,  2 },
    {   9,  35, 15, 0, 0,  2 },
    {  10,  36, 15, 0, 0,  2 },
};
#define EXPECTED_SEQ2_N15_LEN 15
#define EXPECTED_SEQ2_N15_CATALOG_SIZE 11

static const unit_expected_match_t expected_seq2_N16[14] = {
    {   0,   7,  5, 0, 1,  3 },
    {  10,  13,  6, 0, 1,  2 },
    {  16,  19,  5, 0, 1,  2 },
    {   7,  26,  5, 1, 1,  3 },
    {   0,  26, 16, 0, 0,  2 },
    {   1,  27, 16, 0, 0,  2 },
    {   2,  28, 16, 0, 0,  2 },
    {   3,  29, 16, 0, 0,  2 },
    {   4,  30, 16, 0, 0,  2 },
    {   5,  31, 16, 0, 0,  2 },
    {   6,  32, 16, 0, 0,  2 },
    {   7,  33, 16, 0, 0,  2 },
    {   8,  34, 16, 0, 0,  2 },
    {   9,  35, 16, 0, 0,  2 },
};
#define EXPECTED_SEQ2_N16_LEN 14
#define EXPECTED_SEQ2_N16_CATALOG_SIZE 10

static const unit_expected_match_t expected_seq3_N5[2] = {
    {   0,   7,  5, 0, 0,  2 },
    {   9,  14,  3, 2, 1,  0 },
};
#define EXPECTED_SEQ3_N5_LEN 2
#define EXPECTED_SEQ3_N5_CATALOG_SIZE 1

static const unit_expected_match_t expected_seq4_N5[2] = {
    {   4,   9,  3, 0, 1,  2 },
    {   8,  15,  4, 0, 0,  2 },
};
#define EXPECTED_SEQ4_N5_LEN 2
#define EXPECTED_SEQ4_N5_CATALOG_SIZE 1

typedef struct {
    int seq_id;
    int N;
    const double *pitches;
    const double *times;
    int n_notes;
    const unit_expected_match_t *expected;
    int n_expected;
    int final_catalog_size;
} unit_test_config_t;

static const unit_test_config_t unit_test_configs[16] = {
    { 1,  3, seq1_giant_steps_16bars_pitches, seq1_giant_steps_16bars_times, 26, expected_seq1_N3,  10,   8 },
    { 1,  4, seq1_giant_steps_16bars_pitches, seq1_giant_steps_16bars_times, 26, expected_seq1_N4,   7,   7 },
    { 1,  5, seq1_giant_steps_16bars_pitches, seq1_giant_steps_16bars_times, 26, expected_seq1_N5,   4,   4 },
    { 1,  6, seq1_giant_steps_16bars_pitches, seq1_giant_steps_16bars_times, 26, expected_seq1_N6,   3,   3 },
    { 2,  7, seq2_giant_steps_32bars_pitches, seq2_giant_steps_32bars_times, 52, expected_seq2_N7,  23,  19 },
    { 2,  8, seq2_giant_steps_32bars_pitches, seq2_giant_steps_32bars_times, 52, expected_seq2_N8,  22,  18 },
    { 2,  9, seq2_giant_steps_32bars_pitches, seq2_giant_steps_32bars_times, 52, expected_seq2_N9,  21,  17 },
    { 2, 10, seq2_giant_steps_32bars_pitches, seq2_giant_steps_32bars_times, 52, expected_seq2_N10,  20,  16 },
    { 2, 11, seq2_giant_steps_32bars_pitches, seq2_giant_steps_32bars_times, 52, expected_seq2_N11,  19,  15 },
    { 2, 12, seq2_giant_steps_32bars_pitches, seq2_giant_steps_32bars_times, 52, expected_seq2_N12,  18,  14 },
    { 2, 13, seq2_giant_steps_32bars_pitches, seq2_giant_steps_32bars_times, 52, expected_seq2_N13,  17,  13 },
    { 2, 14, seq2_giant_steps_32bars_pitches, seq2_giant_steps_32bars_times, 52, expected_seq2_N14,  16,  12 },
    { 2, 15, seq2_giant_steps_32bars_pitches, seq2_giant_steps_32bars_times, 52, expected_seq2_N15,  15,  11 },
    { 2, 16, seq2_giant_steps_32bars_pitches, seq2_giant_steps_32bars_times, 52, expected_seq2_N16,  14,  10 },
    { 3,  5, seq3_case1_test_pitches, seq3_case1_test_times, 21, expected_seq3_N5, 2, 1 },
    { 4,  5, seq4_supergraph_erase_test_pitches, seq4_supergraph_erase_test_times, 21, expected_seq4_N5, 2, 1 },
};

static inline void unit_feed_note(t_md *x, double cents, double ioi)
{
    if (x->note_count == 0) {
        x->pd[0] = cents;
    } else if (x->note_count < x->N) {
        x->pd[x->note_count] = cents;
        x->td[x->note_count - 1] = ioi;
    } else if (x->note_count == x->N) {
        x->pd[x->note_count] = cents;
        x->td[x->note_count - 1] = ioi;
        snap(x->pd, x->N + 1, x->thresholds[0]);
        snap(x->td, x->N,     x->thresholds[1]);
        build_initial_words(x);
        shift_left(x->pd, x->N + 1);
        shift_left(x->td, x->N);
        prefix_index_register(x, 0);
    } else {
        x->pd[x->N] = cents;
        x->td[x->n] = ioi;
        snap(x->pd, x->N + 1, x->thresholds[0]);
        snap(x->td, x->N,     x->thresholds[1]);
        build_next_words(x);
        shift_left(x->pd, x->N + 1);
        shift_left(x->td, x->N);
        find_repeats(x);
        prefix_index_register(x, x->motive_index);
    }
    x->note_count += 1;
}

static inline size_t unit_catalog_size(t_md *x)
{
    switch (x->unsigned_integer_length) {
        case 64:  return catalog64_size(x->cat64);
        case 128: return catalog128_size(x->cat128);
        case 192: return catalog192_size(x->cat192);
        case 256: return catalog256_size(x->cat256);
    }
    return 0;
}

static inline int unit_content_in_catalog(t_md *x, int_fast64_t pos, uint_fast8_t sg)
{
    switch (x->unsigned_integer_length) {
        case 64:  { content_key64_t  k; extract_content64 (x, pos, sg, &k); return catalog64_get (x->cat64,  k) != NULL; }
        case 128: { content_key128_t k; extract_content128(x, pos, sg, &k); return catalog128_get(x->cat128, k) != NULL; }
        case 192: { content_key192_t k; extract_content192(x, pos, sg, &k); return catalog192_get(x->cat192, k) != NULL; }
        case 256: { content_key256_t k; extract_content256(x, pos, sg, &k); return catalog256_get(x->cat256, k) != NULL; }
    }
    return 0;
}

static inline int unit_content_in_subsumed(t_md *x, int_fast64_t pos, uint_fast8_t sg)
{
    switch (x->unsigned_integer_length) {
        case 64:  { content_key64_t  k; extract_content64 (x, pos, sg, &k); return subsumed_set64_get (x->sub64,  k) != NULL; }
        case 128: { content_key128_t k; extract_content128(x, pos, sg, &k); return subsumed_set128_get(x->sub128, k) != NULL; }
        case 192: { content_key192_t k; extract_content192(x, pos, sg, &k); return subsumed_set192_get(x->sub192, k) != NULL; }
        case 256: { content_key256_t k; extract_content256(x, pos, sg, &k); return subsumed_set256_get(x->sub256, k) != NULL; }
    }
    return 0;
}

static int test_find_repeats_and_register(t_md *x)
{
    post("---- test_find_repeats_and_register ----");
    int total_failures = 0;
    int n_configs = (int)(sizeof(unit_test_configs) / sizeof(unit_test_configs[0]));

    double saved_thr[2] = { x->thresholds[0], x->thresholds[1] };
    x->thresholds[0] = 0.001;
    x->thresholds[1] = 0.001;

    for (int t = 0; t < n_configs; t++) {
        const unit_test_config_t *cfg = &unit_test_configs[t];
        preprocess(x, cfg->N);
        unit_captured_count = 0;
        for (int k = 0; k < cfg->n_notes; k++) {
            double ioi = (k >= 1) ? cfg->times[k - 1] : 0.0;
            unit_feed_note(x, cfg->pitches[k], ioi);
        }

        int fail = 0;
        if (unit_captured_count != cfg->n_expected) {
            post("  seq%d N=%d: captured %d matches, expected %d",
                 cfg->seq_id, cfg->N, unit_captured_count, cfg->n_expected);
            fail++;
            for (int k = 0; k < cfg->n_expected; k++) {
                const unit_expected_match_t *e = &cfg->expected[k];
                int found = 0;
                for (int j = 0; j < unit_captured_count; j++)
                    if (unit_captured_matches[j].i == e->i && unit_captured_matches[j].mi == e->mi && unit_captured_matches[j].sg == e->sg) { found = 1; break; }
                if (!found) post("      expected (%d,%d,sg=%d) NOT captured",
                                 (int)e->i, (int)e->mi, (int)e->sg);
            }
        } else {
            for (int k = 0; k < cfg->n_expected; k++) {
                const unit_expected_match_t *e = &cfg->expected[k];
                const unit_captured_match_t *a = &unit_captured_matches[k];
                if (a->i != e->i || a->mi != e->mi || a->sg != e->sg) {
                    post("  seq%d N=%d: match #%d actual (%lld,%lld,sg=%d) vs expected (%d,%d,sg=%d)",
                         cfg->seq_id, cfg->N, k,
                         (long long)a->i, (long long)a->mi, (int)a->sg,
                         (int)e->i, (int)e->mi, (int)e->sg);
                    fail++;
                }
            }
        }

        size_t cat_size = unit_catalog_size(x);
        if ((int)cat_size != cfg->final_catalog_size) {
            post("  seq%d N=%d: final catalog size %zu, expected %d",
                 cfg->seq_id, cfg->N, cat_size, cfg->final_catalog_size);
            fail++;
        }

        for (int k = 0; k < cfg->n_expected; k++) {
            const unit_expected_match_t *e = &cfg->expected[k];
            int in_cat = unit_content_in_catalog(x, e->mi, (uint_fast8_t)e->sg);
            int in_sub = unit_content_in_subsumed(x, e->mi, (uint_fast8_t)e->sg);
            int expected_in_cat = (e->dest == 0);
            if (expected_in_cat && !in_cat) {
                post("  seq%d N=%d: match (%d,%d,sg=%d) expected in catalog but not present",
                     cfg->seq_id, cfg->N, (int)e->i, (int)e->mi, (int)e->sg);
                fail++;
            }
            if (!expected_in_cat && !in_sub) {
                post("  seq%d N=%d: match (%d,%d,sg=%d) expected in subsumed_set but not present",
                     cfg->seq_id, cfg->N, (int)e->i, (int)e->mi, (int)e->sg);
                fail++;
            }
        }

        post("  seq%d N=%-2d: %s (%d failure%s, %d captured / %d expected, catalog size %zu / %d)",
             cfg->seq_id, cfg->N,
             fail == 0 ? "PASS" : "FAIL", fail,
             fail == 1 ? "" : "s",
             unit_captured_count, cfg->n_expected,
             cat_size, cfg->final_catalog_size);
        total_failures += fail;

        t_atom av[5];
        atom_setsym (av + 0, gensym("find_repeats_register"));
        atom_setlong(av + 1, (t_atom_long)cfg->seq_id);
        atom_setlong(av + 2, (t_atom_long)cfg->N);
        atom_setlong(av + 3, (t_atom_long)fail);
        atom_setlong(av + 4, (t_atom_long)cat_size);
        outlet_list(x->m_outlet_test, NULL, 5, av);
    }

    x->thresholds[0] = saved_thr[0];
    x->thresholds[1] = saved_thr[1];

    post("---- test_find_repeats_and_register: %s (total=%d) ----",
         total_failures == 0 ? "PASSED" : "FAILED", total_failures);
    return total_failures;
}

static const uint_fast8_t mdf_order_Ns[]      = { 6, 10, 13, 16 };
static const int          mdf_order_widths[]  = { 64, 128, 192, 256 };
#define MDF_ORDER_NOTES 120

#define DEFINE_SELFMATCH_SCAN(WIDTH)                                          \
static int selfmatch_scan_##WIDTH(catalog##WIDTH##_t cat)                     \
{                                                                             \
    int dup = 0;                                                              \
    catalog##WIDTH##_it_t it;                                                 \
    for (catalog##WIDTH##_it(it, cat); !catalog##WIDTH##_end_p(it);           \
         catalog##WIDTH##_next(it))                                           \
    {                                                                         \
        catalog##WIDTH##_pair_ct *pair = catalog##WIDTH##_ref(it);            \
        size_t n = idx_array_size(pair->value.indices);                       \
        for (size_t a = 0; a + 1 < n; a++) {                                  \
            for (size_t b = a + 1; b < n; b++) {                              \
                if (*idx_array_cget(pair->value.indices, a) ==                \
                    *idx_array_cget(pair->value.indices, b)) {                \
                    dup++;                                                    \
                }                                                             \
            }                                                                 \
        }                                                                     \
    }                                                                         \
    return dup;                                                               \
}
DEFINE_SELFMATCH_SCAN(64)
DEFINE_SELFMATCH_SCAN(128)
DEFINE_SELFMATCH_SCAN(192)
DEFINE_SELFMATCH_SCAN(256)

static int test_md_float_ordering(t_md *x)
{
    post("---- test_md_float_ordering: md_float() FOURTH-branch call order ----");
    int total_failures = 0;

    for (unsigned c = 0; c < sizeof(mdf_order_Ns) / sizeof(mdf_order_Ns[0]); c++) {
        uint_fast8_t N = mdf_order_Ns[c];
        int failures = 0;

        preprocess(x, (long)N);

        static const double cell[] = { 6000.0, 6200.0, 6400.0, 6500.0, 6200.0 };
        const int cell_len = (int)(sizeof(cell) / sizeof(cell[0]));
        for (int i = 0; i < MDF_ORDER_NOTES; i++) {
            md_float(x, cell[i % cell_len]);
        }

        size_t cat = 0, sub = 0;
        int    dup = 0;
        switch (x->unsigned_integer_length) {
            case 64:  cat = catalog64_size(x->cat64);
                      sub = subsumed_set64_size(x->sub64);
                      dup = selfmatch_scan_64(x->cat64);   break;
            case 128: cat = catalog128_size(x->cat128);
                      sub = subsumed_set128_size(x->sub128);
                      dup = selfmatch_scan_128(x->cat128); break;
            case 192: cat = catalog192_size(x->cat192);
                      sub = subsumed_set192_size(x->sub192);
                      dup = selfmatch_scan_192(x->cat192); break;
            case 256: cat = catalog256_size(x->cat256);
                      sub = subsumed_set256_size(x->sub256);
                      dup = selfmatch_scan_256(x->cat256); break;
            default:  post("  FAIL: unexpected container width %d",
                           (int)x->unsigned_integer_length);
                      failures++;                          break;
        }

        if ((int)x->unsigned_integer_length != mdf_order_widths[c]) {
            post("  FAIL: N=%u expected %d-bit container, got %d",
                 (unsigned)N, mdf_order_widths[c],
                 (int)x->unsigned_integer_length);
            failures++;
        }
        if (cat == 0) {
            post("  FAIL: catalog empty -- md_float never reached register_match");
            failures++;
        }
        if (sub == 0) {
            post("  FAIL: subsumed_set empty -- did prefix_index_register() run "
                 "BEFORE find_repeats() in md_float?");
            failures++;
        }
        if (dup != 0) {
            post("  FAIL: %d catalog entr%s list the same index twice "
                 "(self-match) -- check find_repeats/prefix_index_register "
                 "order in md_float", dup, dup == 1 ? "y" : "ies");
            failures++;
        }

        post("  N=%2u (%3d-bit): catalog=%-5zu subsumed=%-5zu self-matches=%-3d %s",
             (unsigned)N, mdf_order_widths[c], cat, sub, dup,
             failures == 0 ? "PASS" : "FAIL");

        if (x->m_outlet_test) {
            t_atom av[5];
            atom_setsym(av + 0, gensym("md_float_order"));
            atom_setlong(av + 1, (t_atom_long)N);
            atom_setlong(av + 2, (t_atom_long)cat);
            atom_setlong(av + 3, (t_atom_long)sub);
            atom_setlong(av + 4, (t_atom_long)failures);
            outlet_list(x->m_outlet_test, NULL, 5, av);
        }
        total_failures += failures;
    }

    post("---- test_md_float_ordering: %s (total=%d) ----",
         total_failures == 0 ? "PASSED" : "FAILED", total_failures);
    return total_failures;
}

void run_all_tests(t_md *x)
{
    uint_fast8_t saved_N = x->N;
    if (saved_N == 0) saved_N = 8;

    post("======================== MD UNIT TEST SUITE ========================");

    test_pd_td_fill_shift(x);
    test_translate_bits(x);
    test_find_repeats_and_register(x);
    test_md_float_ordering(x);
    test_perf_find_repeats(x, PERF_ITERS);
    test_perf_bach(x, PERF_ITERS);

    preprocess(x, saved_N);
    post("================== TESTS COMPLETE (restored N=%u) ==================",
         (unsigned)saved_N);
}

#endif
