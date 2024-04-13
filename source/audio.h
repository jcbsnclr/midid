#pragma once

#include <jack/types.h>
#include <jack/jack.h>
#include <jack/midiport.h>

#include <err.h>

#include <stdbool.h>

#define ENV_SUSTAIN 0

typedef struct env_stage_t {
    jack_time_t time;
    float amp;
    struct env_stage_t *next;
} env_stage_t;

typedef enum midi_kind_t {
    NOTE_OFF = 0x80,
    NOTE_ON  = 0x90,
    CONTROL = 0xB0
} midi_kind_t;

typedef enum midi_field_t {
    MIDI_STATUS = 0,
    MIDI_NOTE   = 1,
    MIDI_VEL    = 2,

    MIDI_CONTROLLER = 1,
    MIDI_VALUE = 2
} midi_field_t;

typedef enum note_stage_t {
    NOTE_START, // start of note (attack phase)
    NOTE_PEAK, // peak of note
    NOTE_DONE, // note fading (release phase)
    NOTE_END  // note finished
} note_stage_t;

#define SECS(n) ((jack_time_t)(n * 1000000))

typedef struct note_state_t {
    uint8_t note;
    uint8_t velocity;
    size_t idx;
    float phase;
    float start_ramp;
    float ramp;
    env_stage_t *stage;
    jack_time_t time;
} note_state_t;

typedef enum osc_kind_t {
    OSC_SIN,
    OSC_SQUARE,
    OSC_SAW,
    OSC_MAX
} osc_kind_t;

typedef struct osc_t {
    osc_kind_t kind;
    size_t base;
    float vol;
    float bias;
} osc_t;

#define VOICES 128

typedef struct instrument_t {
    note_state_t active[VOICES];

    env_stage_t *env_start;
    env_stage_t *env_done;

    osc_t osc;
} instrument_t;

typedef struct state_t {
    jack_client_t *client;
    jack_port_t *input;
    jack_port_t *output;
    jack_nframes_t srate;
    size_t sample;
    uint8_t channel;
    float volume;
    jack_time_t time;

    instrument_t chan[16];
} state_t;

result_t state_init(state_t *st);
void state_free(state_t *st);

