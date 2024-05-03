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

#include <ansi.h>
#include <audio.h>
#include <log.h>
#include <string.h>
#include <synth.h>

#include "jack/jack.h"
#include "jack/midiport.h"
#include "jack/types.h"
#include "mem.h"

#define JACK_ERR(k) ERR_VAL(ERR_JACK, "jack operation failed")

char *osc_kind_str[OSC_MAX] = {[OSC_SIN] = "sin",
                               [OSC_SQUARE] = "square",
                               [OSC_TRIANGLE] = "triangle",
                               [OSC_SAW] = "saw",
                               [OSC_NOISE] = "noise"};

static void jack_error_report(const char *msg) {
    log_trace("JACK: %s", msg);
}

static int jack_xrun(void *arg) {
    (void)arg;
    log_warn("xrun detected");
    return 0;
}

static void midi_process(state_t *st, void *midi_buf) {
    size_t i = 0;
    jack_midi_event_t ev;

    size_t ev_count = jack_midi_get_event_count(midi_buf);

    while (i < ev_count) {
        jack_midi_event_get(&ev, midi_buf, i++);

        // MIDI event params
        uint8_t kind = ev.buffer[MIDI_STATUS] & 0xf0;

        chan_t *chan = &st->chans[ev.buffer[MIDI_STATUS] & 0x0f];

        for (size_t i = 0; i < chan->len; i++) {
            instrument_t *inst = chan->insts[i];

            switch (kind) {
                case NOTE_ON: {
                    uint8_t note = ev.buffer[MIDI_NOTE];
                    uint8_t vel = ev.buffer[MIDI_VEL];

                    log_trace("note on (note = %d, vel = %d, chan = %d, wave = %d, inst = %d)",
                              note,
                              vel,
                              chan,
                              inst->osc1->kind,
                              i);
                    if (vel != 0)
                        inst->active[note].stage = inst->env->start;
                    else if (inst->env->done)
                        inst->active[note].stage = inst->env->done;

                    inst->active[note].note = note;
                    inst->active[note].velocity = vel;
                    inst->active[note].time = st->time;
                    inst->active[note].start_ramp = inst->active[note].ramp;
                } break;

                case NOTE_OFF: {
                    uint8_t note = ev.buffer[MIDI_NOTE];

                    // log_trace("note off (note = %d, vel = %d, chan = %d, wave = %d, inst = %d)",
                    //           note,
                    //           vel,
                    //           chan,
                    //           inst->osc.kind,
                    //           i);
                    if (inst->env->done) inst->active[note].stage = inst->env->done;
                    inst->active[note].time = st->time;
                    inst->active[note].start_ramp = inst->active[note].ramp;
                } break;

                case CONTROL: {
                    uint8_t cc = ev.buffer[MIDI_CONTROLLER];
                    uint8_t val = ev.buffer[MIDI_VALUE];

                    switch (cc) {
                        case 1: {
                            inst->osc1->bias = (float)val / 127.f;
                        } break;

                        case 12: {
                            inst->osc1->kind = ((float)val / 127.f) * (OSC_MAX - 1);
                            log_trace("lol %d", inst->osc1->kind);
                        } break;

                        default:
                            break;
                    }

                    log_trace("midi cc (cc = %d, val = %d, chan = %d)", cc, val, chan);
                } break;

                default:
                    log_error("unknown event kind %x", kind);
            }
        }
    }
}

static int process(jack_nframes_t nframes, void *arg) {
    state_t *st = (state_t *)arg;

    jack_nframes_t cur_frames;
    jack_time_t next_usecs;
    float period_usecs;
    jack_get_cycle_times(st->client, &cur_frames, &st->time, &next_usecs, &period_usecs);

    void *input_buf = jack_port_get_buffer(st->input, nframes);
    jack_default_audio_sample_t *output_buf =
        (jack_default_audio_sample_t *)jack_port_get_buffer(st->output, nframes);

    midi_process(st, input_buf);

    for (size_t i = 0; i < nframes; i++) {
        output_buf[i] = 0.0;

        for (size_t c = 0; c < 16; c++) {
            chan_t *chan = &st->chans[c];
            for (size_t j = 0; j < chan->len; j++) {
                output_buf[i] += inst_sample(st, chan->insts[j]);
            }
        }
    }

    st->sample += nframes;

    return 0;
}

static int srate(jack_nframes_t nframes, void *arg) {
    state_t *st = (state_t *)arg;
    st->srate = nframes;
    return 0;
}

result_t state_init(state_t *st) {
    log_info("allocating memory pool");
    TRY(mem_init(&st->pool, MiB(8)));

    log_info("creating object map");
    TRY(map_init(&st->map));

    log_debug("opening connection to JACK");

    jack_status_t status;
    st->client = jack_client_open("midid", JackNullOption, &status);

    if (!st->client) return JACK_ERR(status);

    log_debug("opening MIDI input and audio output");

    st->input =
        jack_port_register(st->client, "midi_in", JACK_DEFAULT_MIDI_TYPE, JackPortIsInput, 0);
    st->output =
        jack_port_register(st->client, "audio_out", JACK_DEFAULT_AUDIO_TYPE, JackPortIsOutput, 0);

    if (!st->input || !st->output) return JACK_ERR(status);

    log_info("program state initialised");

    // setup callback
    jack_set_error_function(jack_error_report);
    jack_set_info_function(jack_error_report);

    log_info("installing callbacks");

    jack_set_process_callback(st->client, process, (void *)st);
    jack_set_sample_rate_callback(st->client, srate, (void *)st);
    jack_set_xrun_callback(st->client, jack_xrun, NULL);

    st->volume = 0.5;
    st->channel = 0;

    for (size_t i = 0; i < 16; i++) {
        chan_t *chan = &st->chans[i];

        chan->len = 0;
        memset(chan->insts, 0, sizeof(instrument_t *));
    }

    return OK_VAL;
}

void state_free(state_t *st) {
    mem_free(&st->pool);
    map_free(&st->map);

    jack_deactivate(st->client);
    jack_client_close(st->client);
}

void log_state(state_t *st) {
    log_info("application state:");
    log_line("  map:");
    for (size_t i = 0; i < BUCKETS; i++) {
        map_bucket_t *b = st->map.buckets[i];

        while (b) {
            log_line("     " ANSI_FG_GREEN "%s " ANSI_RESET "%.*s:",
                     obj_kind_str[b->kind],
                     b->key_len,
                     b->key);

            osc_t *osc;
            env_t *env;
            instrument_t *inst;

            (void)env;
            (void)inst;

            switch (b->kind) {
                case OBJ_OSC:
                    osc = (osc_t *)b->val;
                    log_line("       wave=%s", osc_kind_str[osc->kind]);
                    log_line("       base=%lld", osc->base);
                    log_line("       bias=%f", osc->bias);
                    log_line("       vol=%f", osc->vol);
                    break;

                case OBJ_ENV:
                    env = (env_t *)b->val;
                    env_stage_t *cur = env->start;

                    while (cur) {
                        log_line("       %f over %lldμs", cur->amp, cur->time);
                        cur = cur->next;
                    }

                    break;

                case OBJ_INST:
                    inst = (instrument_t *)b->val;

                    log_line("       %s %% %s", inst->osc1->name, inst->osc2->name);

                    break;

                default:
                    log_line("      unknown");
                    break;
            }

            b = b->next;
        }
    }

    log_line("  chans:");

    for (size_t i = 0; i < 16; i++) {
        if (st->chans[i].len == 0) continue;

        log_line("    chan %llu:", i);

        for (size_t j = 0; j < st->chans[i].len; j++) {
            log_line("      %s", st->chans[i].insts[j]->name);
        }
    }
}
