#pragma once

#include <jack/types.h>
#include <jack/jack.h>
#include <jack/midiport.h>

#include <err.h>

typedef struct state_t {
    jack_client_t *client;
    jack_port_t *input;
    jack_port_t *output;
    jack_nframes_t srate;
    uint8_t note;
    float note_on;
    size_t sample;
} state_t;

result_t state_init(state_t *st);
void state_free(state_t *st);
