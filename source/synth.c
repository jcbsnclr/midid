/*
 * midid - software MIDI synthesiser, utilising JACK
 * Copyright (C) 2024  Jacob Sinclair <jcbsnclr@outlook.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include <audio.h>
#include <log.h>
#include <math.h>
#include <stdlib.h>
#include <synth.h>

/*
Input: Peak amplitude (A), Frequency (f)
Output: Amplitude value (y)

y = A * sin(phase)

phase = phase + ((2 * pi * f) / samplerate)

if phase > (2 * pi) then
      phase = phase - (2 * pi)*/

// #define ATTACK_TIME SECS(1)
// #define ADJUST_TIME SECS(1)

static float lerp(state_t *st, note_state_t *voice, float base, float to, jack_time_t time) {
    jack_time_t rel_time = st->time - voice->time;
    float t = (float)rel_time / (float)time;

    return (1.0 - t) * base + t * to;
}

static void env_process(state_t *st, note_state_t *voice) {
    env_stage_t *env = voice->stage;

    if (env && env->time != ENV_SUSTAIN) {
        voice->ramp = lerp(st, voice, voice->start_ramp, env->amp, env->time);

        if (voice->time + env->time < st->time) {
            voice->stage = env->next;
            voice->time = st->time;
            voice->start_ramp = voice->ramp;
        }
    } else if (env && env->time == ENV_SUSTAIN) {
        voice->ramp = voice->start_ramp;
    } else if (!env) {
        voice->ramp = 0.0;
    }
}

static int rand_between(int min, int max) {
    return rand() % (max - min + 1) + min;
}

static float wave_sin(state_t *st, osc_t *osc, note_state_t *n, size_t i, float hz) {
    (void)osc;
    (void)n;
    float step = (hz * 2 * M_PI) / st->srate;

    // return sinf(fmod(i * step, M_PI * 2));
    return sinf(i * step);
}

static float wave_square(state_t *st, osc_t *osc, note_state_t *n, size_t x, float hz) {
    return wave_sin(st, osc, n, x, hz) > osc->bias ? 1.0 : -1.0;
}

static float wave_saw(state_t *st, osc_t *osc, note_state_t *n, size_t x, float hz) {
    (void)osc;
    (void)n;
    // return (2.0 / M_PI) * (hz * M_PI * hz - (M_PI / 2.0));
    // return (2.0 / M_PI) * (hz * M_PI * fmod(x, 1.0 / hz)) - M_PI / 2.0;
    float step = (hz * 2 * M_PI) / st->srate;
    return 2 * (fmod(x * step, 1) - 0.5);
}

static float wave_triangle(state_t *st, osc_t *osc, note_state_t *n, size_t x, float hz) {
    // return asinf(sinf(x) * 2.0 / M_PI);
    return asinf(wave_sin(st, osc, n, x, hz));
}

static float wave_noise(state_t *st, osc_t *osc, note_state_t *n, size_t x, float hz) {
    return wave_sin(st, osc, n, x, hz) * (float)rand_between(-100, 100) / 100.f;
}

typedef float (*wave_fn_t)(state_t *, osc_t *, note_state_t *, size_t, float);

static wave_fn_t wave_fn_table[] = {
    [OSC_SIN] = wave_sin,
    [OSC_SQUARE] = wave_square,
    [OSC_TRIANGLE] = wave_triangle,
    [OSC_SAW] = wave_saw,
    [OSC_NOISE] = wave_noise,
};

#define A4_MIDI 69
#define A4_HZ 440.f

static float semi_hz(int16_t semi) {
    return powf(2, (float)(semi - A4_MIDI) / 12.) * A4_HZ;
}

static float gen_wave(instrument_t *inst, state_t *st, size_t i, note_state_t *voice) {
    (void)st;
    (void)wave_fn_table;

    float car_hz = semi_hz(voice->note + inst->osc1->base);
    float mod_hz = semi_hz(voice->note + inst->osc2->base);

    float car_step = (2 * M_PI * car_hz) / st->srate;
    float mod_step = (2 * M_PI * mod_hz) / st->srate;

    float mod_samp = inst->osc2->vol * sinf(mod_step * i);

    return voice->ramp * inst->osc1->vol * sinf(car_step * i + mod_samp);
}

float inst_sample(state_t *st, instrument_t *inst) {
    //  0.15191
    // log_error("LOL %f", semi_hz(-69));

    // X(1);
    float sample = 0.0;
    for (size_t i = 0; i < VOICES; i++) {
        // X(2);
        note_state_t *voice = &inst->active[i];
        env_process(st, voice);

        if (voice->stage != NULL) {
            // X(4);
            sample += gen_wave(inst, st, voice->idx++, voice);
        }
    }

    // X(6);

    // float out = (sample / voices) * osc->vol;
    float out = sample * 0.5;

    // log_warn("out = %f", out);

    return out;
}
