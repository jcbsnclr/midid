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

static float sine_wave(state_t *st, size_t i, note_state_t *voice) {
    float hz = powf(2, ((float)voice->note - 69.)/12.) * 440.;
     (void)st;
     (void)i;
     
     float out = ((float)voice->velocity / 127) * saw(/*i */ voice->phase);
     
    float step = (hz * 2 * M_PI) / st->srate;
     voice->phase += step;

     out = sinf(i * step + out);

     if (voice->phase > (2 * M_PI))
          voice->phase = voice->phase - (2 * M_PI);
    // return ((float)voice.velocity / 127) * sinf((float)i * step + phase);
     (void)square;
     (void)saw;
    return out;
}

float synth_sample(state_t *st, size_t idx, float volume) {
     (void)idx;
     int voices = 0;
     float sample = 0.0;
     for (size_t i = 0; i < VOICES; i++) {
          if (st->active[i].on) {
               voices++;

               sample += sine_wave(st, /*idx -*/ st->active[i].idx, &st->active[i]);
               st->active[i].idx += 1;
          }
     }
     if (voices == 0)
          return 0.0;

     float out = (sample / voices) * volume;

     return out;
}
