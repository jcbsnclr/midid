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

#pragma once

#include <jack/types.h>
#include <jack/jack.h>
#include <jack/midiport.h>

#include <err.h>

#include <stdbool.h>
#include "map.h"
#include "mem.h"

#define ENV_SUSTAIN 0

typedef struct env_stage_t {
    jack_time_t time;
    float amp;
    struct env_stage_t *next;
} env_stage_t;

typedef struct env_t {
    char *name;
    env_stage_t *start;
    env_stage_t *done;
} env_t;

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
    int8_t note;
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
    OSC_TRIANGLE,
    OSC_SAW,
    OSC_NOISE,
    OSC_MAX
} osc_kind_t;

typedef struct osc_t {
    osc_kind_t kind;
    int64_t base;
    float vol;
    float bias;

    int hz;

    char *name;
} osc_t;

#define VOICES 128
#define INSTRUMENTS 4
#define ENVELOPES 256

typedef struct inst_t {
    char *name;
    note_state_t active[VOICES];
    env_t *env;
    osc_t *osc1;
    osc_t *osc2;
} instrument_t;

typedef struct chan_t {
    size_t len;
    instrument_t *insts[INSTRUMENTS];
} chan_t;

typedef struct state_t {
    mem_pool_t pool;
    map_t map;
    
    jack_client_t *client;
    jack_port_t *input;
    jack_port_t *output;
    jack_nframes_t srate;
    size_t sample;
    uint8_t channel;
    float volume;
    jack_time_t time;

    chan_t chans[16];
} state_t;

extern char *osc_kind_str[OSC_MAX];

result_t state_init(state_t *st);
void state_free(state_t *st);

void log_state(state_t *st);

