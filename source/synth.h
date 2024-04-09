#pragma once

#include <stdio.h>
#include <stddef.h>

#include <jack/types.h>

#include <audio.h>

float synth_sample(state_t *st, size_t idx, float volume);
