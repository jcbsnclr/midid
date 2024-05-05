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

float fold(float x, float threshold) {
    float sign = 1.0f;
    if (x < 0.0f) sign = -1.0;

    x *= sign;
    if (x > threshold) {
        const float remainder = fmod(x, threshold);
        const int numFolds = (int)floor(x / threshold);

        float y;
        if (numFolds % 2 == 0)
            y = remainder;
        else
            y = threshold - remainder;
        return y * sign;
    }
    return x * sign;
}
static float wave_sin(float x, float bias) {
    float samp = sinf(x);

    return fold(samp, 1.0 - bias);
}

static float wave_square(float x, float bias) {
    return sinf(x) > bias ? 1.0 : -1.0;
}

static float wave_triangle(float x, float bias) {
    return asinf(wave_sin(x, bias));
}

static float wave_saw(float x, float bias) {
    (void)bias;

    float s = 2 * (fmod(x, 1) - 0.5);

    return fold(s, bias);
}

static float wave_noise(float x, float bias) {
    return wave_sin(x, bias) * (float)rand_between(-100, 100) / 100.f;
}

// type of a wave function
typedef float (*wave_fn_t)(float x, float bias);

static wave_fn_t wave_fn_table[] = {
    [OSC_SIN] = wave_sin,
    [OSC_SQUARE] = wave_square,
    [OSC_TRIANGLE] = wave_triangle,
    [OSC_SAW] = wave_saw,
    [OSC_NOISE] = wave_noise,
};

#define A4_MIDI 69
#define A4_HZ 440.f

// converts a MIDI note number to a frequency in hz
static float semi_hz(int16_t semi) {
    return powf(2, (float)(semi - A4_MIDI) / 12.) * A4_HZ;
}

static float sample_link(state_t *st, osc_link_t *link, size_t i, note_state_t *voice) {
    wave_fn_t fn = wave_fn_table[link->osc->kind];
    float hz = link->osc->hz != 0 ? link->osc->hz : semi_hz(voice->note + link->osc->base);
    float step = (2 * M_PI * hz) / st->srate;

    if (link->next == NULL) {
        return link->osc->vol * fn(i * step, link->osc->bias);
    } else {
        float mod_samp = sample_link(st, link->next, i, voice);

        switch (link->method) {
            case MOD_AM:
                return link->osc->vol * fn(step * i, link->osc->bias) * mod_samp;

            case MOD_PM:
                return link->osc->vol * fn(step * i + mod_samp, link->osc->bias);

            case MOD_FM:
                return link->osc->vol * fn((step * mod_samp) * i, link->osc->bias);

            case MOD_BM:
                return link->osc->vol * fn(step * i, (mod_samp + 1.0) / 2. - 0.0005);

            default:
                log_error("unknown modulation method %d", link->method);
                return 0.0;
        }
    }
}

static float gen_wave(instrument_t *inst, state_t *st, size_t i, note_state_t *voice) {
    return voice->ramp * sample_link(st, inst->links, i, voice);

    // work out frequency for carrier and modulator wave
    // float car_hz = inst->osc1->hz != 0 ? inst->osc1->hz : semi_hz(voice->note +
    // inst->osc1->base); float mod_hz = inst->osc2->hz != 0 ? inst->osc2->hz : semi_hz(voice->note
    // + inst->osc2->base);

    // // calculate step, used to map from real time to 0 -> PI
    // float car_step = (2 * M_PI * car_hz) / st->srate;
    // float mod_step = (2 * M_PI * mod_hz) / st->srate;

    // // retrieve wave functions
    // wave_fn_t car_fn = wave_fn_table[inst->osc1->kind];
    // wave_fn_t mod_fn = wave_fn_table[inst->osc2->kind];

    // // sample modulator wave
    // float mod_samp = inst->osc2->vol * mod_fn(mod_step * i, inst->osc2->bias);

    // // return mod_samp;

    // // log_warn("inst->method = %d", inst->method);

    // switch (inst->method) {
    //     case MOD_AM:
    //         return voice->ramp * inst->osc1->vol * car_fn(car_step * i, inst->osc1->bias) *
    //                mod_samp;

    //     case MOD_PM:
    //         return voice->ramp * inst->osc1->vol *
    //                car_fn(car_step * i + mod_samp, inst->osc1->bias);

    //     case MOD_FM:
    //         return voice->ramp * inst->osc1->vol *
    //                car_fn((car_step * mod_samp) * i, inst->osc1->bias);

    //     case MOD_BM:
    //         return voice->ramp * inst->osc1->vol *
    //                car_fn(car_step * i, (mod_samp + 1.0) / 2. - 0.0005);

    //     default:
    //         log_error("unknown modulation method %d", inst->method);
    //         return 0.0;
    // }
}

float inst_sample(state_t *st, instrument_t *inst) {
    float sample = 0.0;
    for (size_t i = 0; i < VOICES; i++) {
        note_state_t *voice = &inst->active[i];
        // process note envelope
        env_process(st, voice);

        if (voice->stage != NULL) {
            // if note is active, generate a sample and add it to the current sample
            sample += gen_wave(inst, st, voice->idx++, voice);
        }
    }

    float out = sample * 0.5;

    return out;
}
