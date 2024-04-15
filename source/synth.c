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

#define ATTACK_TIME SECS(1)
#define ADJUST_TIME SECS(1)

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
    float step = (hz * 2 * M_PI) / st->srate;

    return sinf(fmod(i * step, M_PI * 2));
}

static float wave_square(state_t *st, osc_t *osc, note_state_t *n, size_t x, float hz) {
    return wave_sin(st, osc, n, x, hz) > osc->bias ? 1.0 : -1.0;
}

static float wave_saw(state_t *st, osc_t *osc, note_state_t *n, size_t x, float hz) {
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

typedef float (*wave_fn_t)(state_t *st, osc_t *, note_state_t *, size_t, float);

static wave_fn_t wave_fn_table[] = {
    [OSC_SIN] = wave_sin,
    [OSC_SQUARE] = wave_square,
    [OSC_TRIANGLE] = wave_triangle,
    [OSC_SAW] = wave_saw,
    [OSC_NOISE] = wave_noise,
};

static float gen_wave(osc_t *osc, state_t *st, size_t i, note_state_t *voice) {
    int64_t note = voice->note + osc->base - 69;
    float hz = powf(2, (float)note / 12.) * 440.;

    env_process(st, voice);

    wave_fn_t fn = wave_fn_table[osc->kind];

    // float step = (hz * 2 * M_PI) / st->srate;
    // float out = fn(st, osc, voice, fmod(i * step, M_PI * 2), hz) * voice->ramp;
    float out = fn(st, osc, voice, i, hz) * voice->ramp;

    return out;
}

float osc_sample(state_t *st, instrument_t *inst) {
    // X(1);
    float sample = 0.0;
    for (size_t i = 0; i < VOICES; i++) {
        // X(2);
        note_state_t *voice = &inst->active[i];

        if (voice->stage != NULL) {
            // X(4);
            sample += gen_wave(&inst->osc, st, voice->idx, voice);
        }

        voice->idx += 1;
    }

    // X(6);

    // float out = (sample / voices) * osc->vol;
    float out = sample * inst->osc.vol;

    return out;
}
