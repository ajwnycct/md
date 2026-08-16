/*!
    * md, a Max external
    * Copyright (c) 2026 Adam James Wilson. All rights reserved.
    * SPDX-License-Identifier: BSD 3-Clause Clear
    */

#ifndef SNAP_H
#define SNAP_H

static inline void sort_idx_by_value(int *idx, int n, const double *values)
{
    for (int i = 1; i < n; i++) {
        int key = idx[i];
        double kv = values[key];
        int j = i - 1;
        while (j >= 0 && values[idx[j]] > kv) {
            idx[j + 1] = idx[j];
            j--;
        }
        idx[j + 1] = key;
    }
}

static inline void snap(double *input, int input_length, double threshold)
{
    int idx[input_length];
    for (int i = 0; i < input_length; i++) idx[i] = i;

    sort_idx_by_value(idx, input_length, input);

    int i = 0;
    while (i < input_length) {
        int j = i + 1;
        while (j < input_length && input[idx[j]] - input[idx[j - 1]] < threshold) j++;
        double anchor = input[idx[i]];
        for (int k = i + 1; k < j; k++) input[idx[k]] = anchor;
        i = j;
    }
}

#endif
