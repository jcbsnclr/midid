#pragma once

#include <stdio.h>
#include <stddef.h>

#include <audio.h>

typedef struct buffer {
    float *buf;
    size_t len;
} buffer_t;

float synth_sample(state_t *st, size_t idx, float volume);
