/*!
    * md, a Max external
    * Copyright (c) 2026 Adam James Wilson. All rights reserved.
    * SPDX-License-Identifier: BSD 3-Clause Clear
    */

#include "md.h"
#include "math.h"

void ext_main(void *r)
{
    t_class *c;
    c = class_new("md", (method)md_new, (method)md_free, (long)sizeof(t_md), 0L, A_GIMME, 0);

    class_addmethod(c, (method)md_float, "float", A_FLOAT, 0);
    class_addmethod(c, (method)md_int, "int", A_LONG, 0);
    class_addmethod(c, (method)md_assist, "assist", A_CANT, 0);
    class_addmethod(c, (method)md_clear, "clear", 0);

    #ifdef UNIT
    class_addmethod(c, (method)md_run_tests, "run-tests", 0);
    #endif
    class_addmethod(c, (method)md_maxcounts, "maxcounts", A_LONG, 0);
    class_addmethod(c, (method)md_maxgraphs, "maxgraphs", A_LONG, 0);

    CLASS_ATTR_LONG(c, "motive-max-length", ATTR_FLAGS_NONE, t_md, N);
    CLASS_ATTR_ACCESSORS(c, "motive-max-length", (method)md_max_length_get, (method)md_max_length_set);

    CLASS_ATTR_DOUBLE_ARRAY(c, "thresholds", ATTR_FLAGS_NONE, t_md, thresholds, 2);
    CLASS_ATTR_ACCESSORS(c, "thresholds", (method)md_thresholds_get, (method)md_thresholds_set);

    class_register(CLASS_BOX, c);
    md_class = c;
}

void *md_new(t_symbol *s, long argc, t_atom *argv)
{
    t_md *x = (t_md *)object_alloc(md_class);

    if (x)
    {
        object_post((t_object *)x, "A new %s object was instantiated: 0x%X.", s->s_name, x);

        post("BitInt max width %d", BITINT_MAXWIDTH);
        post("numbits in uint256_t = %d", sizeof(uint256_t) * CHAR_BIT);

        x->m_outlet = intout((t_object *)x);

        #ifdef UNIT
        x->m_outlet_test = listout((t_object *)x);
        #endif

        x->min_subgraph_size = 3;

        x->N = 0;

        x->thresholds[0] = 0.25;
        x->thresholds[1] = 5.0;

        attr_args_process(x, argc, argv);

        if (x->N == 0) preprocess(x, 8);
    }
    return(x);
}

void md_free(t_md *x)
{
    sysmem_freeptr(x->pd); x->pd = NULL;
    sysmem_freeptr(x->td); x->td = NULL;
    sysmem_freeptr(x->p);  x->p  = NULL;
    sysmem_freeptr(x->t);  x->t  = NULL;
    matches_clear(x->matches);
    prefix_index_clear(x->prefix_index);

    switch (x->unsigned_integer_length) {
        case 64:  catalog64_clear(x->cat64);   subsumed_set64_clear(x->sub64);   break;
        case 128: catalog128_clear(x->cat128); subsumed_set128_clear(x->sub128); break;
        case 192: catalog192_clear(x->cat192); subsumed_set192_clear(x->sub192); break;
        case 256: catalog256_clear(x->cat256); subsumed_set256_clear(x->sub256); break;
    }
}

void md_assist(t_md *x, void *b, long m, long a, char *s)
{
    if (m == ASSIST_INLET)
    {
        sprintf(s, "float/int: cents (0-12700). Msgs: clear, thresholds <pd> <td>, run-tests. motive-max-length (3-16)");
    }
    else
    {
#ifdef UNIT
        switch (a) {
            case 0:
                sprintf(s, "list: unit-test results (format varies per test)");
                break;
            case 1:
                sprintf(s, "list: matched motives sorted by count or subgraph size (format TBD)");
                break;
        }
#else
        sprintf(s, "list: matched motives sorted by count or subgraph size (format TBD)");
#endif
    }
}

void md_thresholds(t_md *x, double pd_thresh, double td_thresh)
{
    x->thresholds[0] = pd_thresh;
    x->thresholds[1] = td_thresh;
}

t_max_err md_max_length_get(t_md *x, t_object *attr, long *argc, t_atom **argv)
{
    if (argc && argv)
    {
        char alloc;

        if (atom_alloc(argc, argv, &alloc))
        {
            return MAX_ERR_GENERIC;
        }

        atom_setlong(*argv, x->N);
        post("retrieving motive-max-length: %ld", atom_getlong(argv[0]));

    }

    return MAX_ERR_NONE;
}

t_max_err md_max_length_set(t_md *x, t_object *attr, long argc, t_atom *argv)
{
    if (argc && argv)
    {
        t_atom_long mml = atom_getlong(argv);

        if (mml < 3 || mml > 16)
        {
            object_error((t_object *)x, "\"Motive Max Length\" cannot be less than 3 or greater than 16.\nSetting to default (8)");
            return MAX_ERR_GENERIC;
        }
        preprocess(x, mml);
        post("motive-max-length setter got called");
    }

    return MAX_ERR_NONE;
}

t_max_err md_thresholds_get(t_md *x, t_object *attr, long *argc, t_atom **argv)
{
    if (argc && argv)
    {
        char alloc;
        if (atom_alloc_array(2, argc, argv, &alloc))
        {
            return MAX_ERR_GENERIC;
        }
        atom_setfloat(*argv + 0, x->thresholds[0]);
        atom_setfloat(*argv + 1, x->thresholds[1]);
    }
    return MAX_ERR_NONE;
}

t_max_err md_thresholds_set(t_md *x, t_object *attr, long argc, t_atom *argv)
{
    if (argc >= 1 && argv) x->thresholds[0] = atom_getfloat(argv);
    if (argc >= 2 && argv) x->thresholds[1] = atom_getfloat(argv + 1);
    return MAX_ERR_NONE;
}

void md_int(t_md *x, long cents)
{
    md_float(x, (double)cents);
}

void md_float(t_md *x, double cents)
{

    double current_time = systimer_gettime();

    if (cents < 0.0 || cents > 12700.0)
    {
        object_error((t_object *)x, "Cents must be a positive float c where 0.0 <= c < 12700.0");
        return;
    }

    critical_enter(0);

    if (x->note_count == x->maximum_notes) {
        object_error((t_object *)x, "Maximum memory is used; we can no longer read in data.");
        goto out;
    }

    if (x->note_count > x->N)
    {
        x->pd[x->N] = cents;
        x->td[x->n] = current_time - x->previous_time;
        snap(x->pd, x->N+1, x->thresholds[0]);
        snap(x->td, x->N,   x->thresholds[1]);
        build_next_words(x);
        shift_left(x->pd, x->N+1);
        shift_left(x->td, x->N);
        find_repeats(x);
        prefix_index_register(x, x->motive_index);
    }
    else if (x->note_count == 0)
    {
        x->pd[x->note_count] = cents;
    }
    else if (x->note_count < x->N)
    {
        x->pd[x->note_count] = cents;
        x->td[x->note_count - 1] = current_time - x->previous_time;
    }
    else if (x->note_count == x->N)
    {
        x->pd[x->note_count] = cents;
        x->td[x->note_count - 1] = current_time - x->previous_time;
        snap(x->pd, x->N+1, x->thresholds[0]);
        snap(x->td, x->N,   x->thresholds[1]);
        build_initial_words(x);
        shift_left(x->pd, x->N+1);
        shift_left(x->td, x->N);
        prefix_index_register(x, x->motive_index);
    }

    x->previous_time = current_time;

    x->note_count += 1;

  out:
      critical_exit(0);
}

void shift_left(double *input_data, long input_data_length)
{
    for (long i = 1; i < input_data_length; i++)
    {
        input_data[i - 1] = input_data[i];
    }
}

void preprocess(t_md *x, long maximum_notes_per_motive)
{

    if (x->N != 0) md_free(x);

    x->N = maximum_notes_per_motive;

    x->n = x->N - 1;

    x->b = x->N * x->n;

    x->C = x->b / 2;

    x->pd = (double *)sysmem_newptr(sizeof(double) * (x->N+1));

    x->td = (double *)sysmem_newptr(sizeof(double) * x->N);

    x->maximum_notes = 108000;

    x->maximum_maximum_length_motives = (x->maximum_notes - x->N);

     matches_init(x->matches);

    prefix_index_init(x->prefix_index);

    x->unsigned_integer_length = (x->b + 63) & ~63;
    switch (x->unsigned_integer_length)
    {
        case 256:
            x->p = (uint256_t *)sysmem_newptrclear(x->maximum_maximum_length_motives * sizeof(uint256_t));
            x->t = (uint256_t *)sysmem_newptrclear(x->maximum_maximum_length_motives * sizeof(uint256_t));
            translate_initial_to_bits = &translate_initial_to_256_bits;
            translate_next_to_bits = &translate_next_to_256_bits;
            catalog256_init(x->cat256);
            subsumed_set256_init(x->sub256);
            break;
        case 192:
            x->p = (uint192_t *)sysmem_newptrclear(x->maximum_maximum_length_motives * sizeof(uint192_t));
            x->t = (uint192_t *)sysmem_newptrclear(x->maximum_maximum_length_motives * sizeof(uint192_t));
            translate_initial_to_bits = &translate_initial_to_192_bits;
            translate_next_to_bits = &translate_next_to_192_bits;
            catalog192_init(x->cat192);
            subsumed_set192_init(x->sub192);
            break;
        case 128:
            x->p = (uint128_t *)sysmem_newptrclear(x->maximum_maximum_length_motives * sizeof(uint128_t));
            x->t = (uint128_t *)sysmem_newptrclear(x->maximum_maximum_length_motives * sizeof(uint128_t));
            translate_initial_to_bits = &translate_initial_to_128_bits;
            translate_next_to_bits = &translate_next_to_128_bits;
            catalog128_init(x->cat128);
            subsumed_set128_init(x->sub128);
            break;
        case 64:
            x->p = (uint64_t *)sysmem_newptrclear(x->maximum_maximum_length_motives * sizeof(uint64_t));
            x->t = (uint64_t *)sysmem_newptrclear(x->maximum_maximum_length_motives * sizeof(uint64_t));
            translate_initial_to_bits = &translate_initial_to_64_bits;
            translate_next_to_bits = &translate_next_to_64_bits;
            catalog64_init(x->cat64);
            subsumed_set64_init(x->sub64);
            break;
    }
    if ((x->p == NULL) || (x->t == NULL))
    {
        object_error((t_object *)x, "Error allocating memory for md arrays. Make sure you have %ldMB of RAM available.", 128);
        return;
    }

    x->note_count = 0;
    x->motive_index = -1;
    x->previous_time = 0.0;
    x->min_subgraph_size = 3;

}

void build_initial_words(t_md *x)
{
    translate_initial_to_bits(x, x->pd, x->p);
    translate_initial_to_bits(x, x->td, x->t);

    x->motive_index = 0;
}

void build_next_words(t_md *x)
{
    translate_next_to_bits(x, x->pd, x->p);
    translate_next_to_bits(x, x->td, x->t);

    x->motive_index += 1;
}

void find_repeats(t_md *x)
{
    if (x->min_subgraph_size > 3) x->min_subgraph_size -= 1;

    uint64_t key = prefix3_fingerprint(x, x->motive_index);
    idx_array_t *bucket = prefix_index_get(x->prefix_index, key);

    if (bucket == NULL) return;

    size_t n = idx_array_size(*bucket);
    for (size_t k = n; k-- > 0; )
    {
        int_fast64_t i = *idx_array_cget(*bucket, k);

        int_fast8_t sg = compare_graphs(x, x->motive_index, i, x->min_subgraph_size);

        if (sg != -1) {

            #ifdef UNIT
            if (unit_captured_count < UNIT_MATCH_CAPTURE_MAX) {
                unit_captured_matches[unit_captured_count].i  = i;
                unit_captured_matches[unit_captured_count].mi = x->motive_index;
                unit_captured_matches[unit_captured_count].sg = sg;
                unit_captured_count++;
            }
            #endif

            register_match(x, (uint_fast64_t)i, (uint_fast8_t)sg);
            x->min_subgraph_size = sg + 1;

            if (sg == x->N) return;
        }
    }
}

#ifdef UNIT
void md_run_tests(t_md *x)
{
    run_all_tests(x);
}
#endif

void md_clear(t_md *x)
{
    if (x->N == 0) return;
    critical_enter(0);
    preprocess(x, x->N);
    critical_exit(0);
}

#define MD_HEX_BUFSZ 80

static inline void md_emit_row_64(t_md *x, int rank, const heap_entry64_t *e)
{
    char pg_buf[MD_HEX_BUFSZ];
    char tg_buf[MD_HEX_BUFSZ];

    char *p;
    p = pg_buf;
    *p++ = '0'; *p++ = 'x';
    p = hex64_fixed(p, (uint64_t)e->key->pg);
    *p++ = 'U'; *p++ = 'L'; *p++ = 'L'; *p = '\0';
    p = tg_buf;
    *p++ = '0'; *p++ = 'x';
    p = hex64_fixed(p, (uint64_t)e->key->tg);
    *p++ = 'U'; *p++ = 'L'; *p++ = 'L'; *p = '\0';

    t_atom argv[5];
    atom_setlong(&argv[0], (long)rank);
    atom_setsym (&argv[1], gensym(pg_buf));
    atom_setsym (&argv[2], gensym(tg_buf));
    atom_setlong(&argv[3], (long)e->size);
    atom_setlong(&argv[4], (long)e->last_index);
    outlet_anything(x->m_outlet, gensym("store"), 5, argv);
}

static inline void md_emit_row_128(t_md *x, int rank, const heap_entry128_t *e)
{
    char pg_buf[MD_HEX_BUFSZ];
    char tg_buf[MD_HEX_BUFSZ];

    char *p;
    p = pg_buf;
    *p++ = '0'; *p++ = 'x';
    p = hex128_fixed(p, e->key->pg);
    *p++ = 'U'; *p++ = 'W'; *p++ = 'B'; *p = '\0';
    p = tg_buf;
    *p++ = '0'; *p++ = 'x';
    p = hex128_fixed(p, e->key->tg);
    *p++ = 'U'; *p++ = 'W'; *p++ = 'B'; *p = '\0';

    t_atom argv[5];
    atom_setlong(&argv[0], (long)rank);
    atom_setsym (&argv[1], gensym(pg_buf));
    atom_setsym (&argv[2], gensym(tg_buf));
    atom_setlong(&argv[3], (long)e->size);
    atom_setlong(&argv[4], (long)e->last_index);
    outlet_anything(x->m_outlet, gensym("store"), 5, argv);
}

static inline void md_emit_row_192(t_md *x, int rank, const heap_entry192_t *e)
{
    char pg_buf[MD_HEX_BUFSZ];
    char tg_buf[MD_HEX_BUFSZ];

    char *p;
    p = pg_buf;
    *p++ = '0'; *p++ = 'x';
    p = hex192_fixed(p, e->key->pg);
    *p++ = 'U'; *p++ = 'W'; *p++ = 'B'; *p = '\0';
    p = tg_buf;
    *p++ = '0'; *p++ = 'x';
    p = hex192_fixed(p, e->key->tg);
    *p++ = 'U'; *p++ = 'W'; *p++ = 'B'; *p = '\0';

    t_atom argv[5];
    atom_setlong(&argv[0], (long)rank);
    atom_setsym (&argv[1], gensym(pg_buf));
    atom_setsym (&argv[2], gensym(tg_buf));
    atom_setlong(&argv[3], (long)e->size);
    atom_setlong(&argv[4], (long)e->last_index);
    outlet_anything(x->m_outlet, gensym("store"), 5, argv);
}

static inline void md_emit_row_256(t_md *x, int rank, const heap_entry256_t *e)
{
    char pg_buf[MD_HEX_BUFSZ];
    char tg_buf[MD_HEX_BUFSZ];

    char *p;
    p = pg_buf;
    *p++ = '0'; *p++ = 'x';
    p = hex256_fixed(p, e->key->pg);
    *p++ = 'U'; *p++ = 'W'; *p++ = 'B'; *p = '\0';
    p = tg_buf;
    *p++ = '0'; *p++ = 'x';
    p = hex256_fixed(p, e->key->tg);
    *p++ = 'U'; *p++ = 'W'; *p++ = 'B'; *p = '\0';

    t_atom argv[5];
    atom_setlong(&argv[0], (long)rank);
    atom_setsym (&argv[1], gensym(pg_buf));
    atom_setsym (&argv[2], gensym(tg_buf));
    atom_setlong(&argv[3], (long)e->size);
    atom_setlong(&argv[4], (long)e->last_index);
    outlet_anything(x->m_outlet, gensym("store"), 5, argv);
}

static inline long md_clamp_m(t_md *x, long n, const char *msg)
{
    if (n < 1) {
        object_warn((t_object *)x, "%s: N=%ld clamped to 1", msg, n);
        return 1;
    }
    if (n > HEAP_MAX_M) {
        object_warn((t_object *)x, "%s: N=%ld clamped to %d",
                    msg, n, HEAP_MAX_M);
        return HEAP_MAX_M;
    }
    return n;
}

#define EMIT_ROWS(WIDTH)                                                      \
    do {                                                                      \
        int n_ret = query_top_m_##WIDTH(x, x->cat##WIDTH, M, mode,            \
                                        x->heap##WIDTH);                      \
        for (int r = 0; r < n_ret; r++) {                                     \
            md_emit_row_##WIDTH(x, r + 1, &x->heap##WIDTH[r]);                \
        }                                                                     \
        n = n_ret;                                                            \
    } while (0)

static void md_query_dispatch(t_md *x, int M, heap_cmp_mode_t mode)
{
    if (x->N == 0) {
        object_warn((t_object *)x, "no motives yet -- send some notes first");
        return;
    }

    int n = 0;

    critical_enter(0);

    outlet_anything(x->m_outlet, gensym("clear"), 0, NULL);

    switch (x->unsigned_integer_length) {
        case 64:  EMIT_ROWS(64);  break;
        case 128: EMIT_ROWS(128); break;
        case 192: EMIT_ROWS(192); break;
        case 256: EMIT_ROWS(256); break;
        default:
            object_error((t_object *)x,
                         "query: unexpected width %u",
                         (unsigned)x->unsigned_integer_length);
            critical_exit(0);
            return;
    }
    critical_exit(0);

    if (n <= 0) {
        object_post((t_object *)x, "no motives available");
    }
}

#undef EMIT_ROWS

void md_maxcounts(t_md *x, long n)
{
    long m = md_clamp_m(x, n, "maxcounts");
    md_query_dispatch(x, (int)m, HEAP_CMP_COUNT);
}

void md_maxgraphs(t_md *x, long n)
{
    long m = md_clamp_m(x, n, "maxgraphs");
    md_query_dispatch(x, (int)m, HEAP_CMP_SIZE);
}
