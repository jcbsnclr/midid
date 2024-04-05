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

#define ATTACK_TIME SECS(0.01)
#define ADJUST_TIME SECS(0.01)

static float lerp(state_t *st, note_state_t *voice, float base, float to, jack_time_t time) {
     jack_time_t rel_time = st->time - voice->time;
     float t = (float)rel_time / (float)time;

     return (to - t) * base + t;
}

static float sine_wave(state_t *st, size_t i, note_state_t *voice) {
     float hz = powf(2, ((float)voice->note - 69.)/12.) * 440.;
     (void)st;
     (void)i;
     // float out = ((float)voice->velocity / 127) * saw(/*i */ voice->phase);
     
     switch (voice->stage) {
          case NOTE_START:
               voice->ramp = lerp(st, voice, voice->ramp, 1.0, ATTACK_TIME);

               if (voice->time + ATTACK_TIME < st->time) {
                    voice->stage = NOTE_PEAK;
                    voice->time = st->time;
               }          

               break;
          case NOTE_PEAK:
               if (voice->time + ADJUST_TIME < st->time) {
                    voice->ramp = lerp(st, voice, voice->ramp, 1.0, ADJUST_TIME);
               } else {
                    voice->ramp = 1.0;
               }
               break;
          case NOTE_DONE:
               // voice->ramp = 1.0 - lerp(st, voice, voice->ramp, 1.0, ATTACK_TIME);
               voice->ramp = 1.0 - lerp(st, voice, 0.0, 1.0, ATTACK_TIME);

               if (voice->time + ATTACK_TIME < st->time) {
                    voice->stage = NOTE_END;
                    voice->time = st->time;
               }          

               break;
          default:
               voice->ramp = 0.0;
               break;
     }
     
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

     float out = sinf(i * step) * voice->ramp;


     
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

          voices++;

          sample += sine_wave(st, /*idx -*/ voice->idx, voice);
          voice->idx += 1;
     }
     if (voices == 0)
          return 0.0;

     float out = (sample / voices) * volume;

     return out;
}
