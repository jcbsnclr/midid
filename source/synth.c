#include <math.h>
#include <stdlib.h>

#include <synth.h>
#include <audio.h>
#include <log.h>

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

static float square(float x) {
     return sinf(x) > 0.0 ? 1.0 : -1.0;
}

static float saw(float x) {
     return 0.0;
}

typedef float (*wave_fn_t)(float);

static wave_fn_t wave_fn_table[] = {
     [OSC_SIN] = sinf,
     [OSC_SQUARE] = square,
     [OSC_SAW] = saw,
};

static float gen_wave(osc_t *osc, state_t *st, size_t i, note_state_t *voice) {
    float hz = powf(2, ((float)voice->note - 69.)/12.) * 440.;
     
    env_process(st, voice);
    
     wave_fn_t fn = wave_fn_table[osc->kind];
    
    float step = (hz * 2 * M_PI) / st->srate;
    float out = fn(fmod(i * step, M_PI * 2)) * voice->ramp;
    
    (void)square;
    (void)saw;
    return out;
}

float osc_sample(state_t *st, instrument_t *inst) {
   #define X(x) log_trace("THERE %d",x);
     
     // X(1);
     int voices = 0;
     float sample = 0.0;
     for (size_t i = 0; i < VOICES; i++) {
          // X(2);
          note_state_t *voice = &inst->active[i];

          if (voice->stage != NULL) {
               voices++;

               // X(4);
               sample += gen_wave(&inst->osc, st, voice->idx, voice);
          }

          voice->idx += 1;
     }
     // X(5);
     if (voices == 0)
          return 0.0;

     // X(6);

     // float out = (sample / voices) * osc->vol;
     float out = sample * inst->osc.vol;

     return out;
}
