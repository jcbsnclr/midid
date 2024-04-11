#include <math.h>
#include <stdlib.h>

#include <synth.h>
#include <audio.h>
#include <log.h>

static float square(float x) {
     return roundf(sin(x));
}

static float saw(float x) {
     return x - floorf(x);
}

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

     // return (to - t) * base + t;
     // return (to - t) * base + t * to;
     return (1.0 - t) * base + t * to;
}

static void env_process(state_t *st, note_state_t *voice) {
     env_stage_t *env = voice->stage;
     
     if (env && env->time != ENV_SUSTAIN) {
          // if (voice->start_ramp < env->amp) 
               // voice->ramp = lerp(st, voice, voice->start_ramp, env->amp, env->time);
          // else 
               // voice->ramp = voice->start_ramp - lerp(st, voice, 1.0 - voice->start_ramp, env->amp, env->time);
               // voice->ramp = voice->start_ramp - lerp(st, voice, 1.0 - voice->start_ramp, env->amp, env->time);
          voice->ramp = lerp(st, voice, voice->start_ramp, env->amp, env->time);
          // voice->ramp = 1.0;

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

static float sine_wave(state_t *st, size_t i, note_state_t *voice) {
     float hz = powf(2, ((float)voice->note - 69.)/12.) * 440.;
     (void)st;
     (void)i;
     // float out = ((float)voice->velocity / 127) * saw(/*i */ voice->phase);
     
     env_process(st, voice);
     
    float step = (hz * 2 * M_PI) / st->srate;
     // voice->phase += step;

     // while (voice->phase >= (2 * M_PI))
     //      voice->phase -= (2 * M_PI);
     // while (voice->phase < (2 * M_PI))
     //      voice->phase += (2 * M_PI);

     // // if (voice->phase > (2 * M_PI))
     // //      voice->phase -= (2 * M_PI);

     // if (voice->stage == NOTE_DONE && fabs(voice->phase) > 0.0001) {
     //      voice->stage = NOTE_END;          
     // }
     // if (voice->stage == NOTE_DONE) 
     //      voice->stage = NOTE_END;

     // size_t idx = i % st->srate;
     float out = sinf(fmod(i * step, M_PI * 2)) * voice->ramp;
     // float out = sinf(idx * step) * voice->ramp;


     
    // return (float)voice.velocity / 127) * sinf((float)i * step + phase);
     (void)square;
     (void)saw;
    return out;
}

float synth_sample(state_t *st, size_t idx, float volume) {
     (void)idx;
     int voices = 0;
     float sample = 0.0;
     for (size_t i = 0; i < VOICES; i++) {
          note_state_t *voice = &st->active[i];

          if (voice->stage != NULL)
               voices++;

          sample += sine_wave(st, /*idx -*/ voice->idx, voice);
          voice->idx += 1;
     }
     if (voices == 0)
          return 0.0;

     float out = (sample / voices) * volume;

     return out;
}
