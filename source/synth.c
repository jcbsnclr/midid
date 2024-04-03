#include <math.h>
#include <stdlib.h>

#include <synth.h>
#include <audio.h>

float synth_sample(state_t *st, size_t i, float volume, float phase) {
     // float hz = 440.0 * powf(powf(2, 1. / 12), (float)st->note);
    float hz = powf(2, ((float)st->note - 69.)/12.) * 440.;
    float step = (hz * 2 * M_PI) / st->srate;
    return volume * sinf((float)i * step + phase);
    // (void)i;

    // float hz = (2.0 * 440.0 / 32.0) * pow(2, (((jack_default_audio_sample_t)st->note - 9.0) / 12.0)) / st->srate;
    // float step = sin(i * (hz * 2 * M_PI));
    // return volume * step;
}
