#include "jack/jack.h"
#include "jack/midiport.h"
#include "jack/types.h"
#include <audio.h>
#include <log.h>
#include <string.h>
#include <synth.h>

// static void insert_voice(state_t *st, instrument_t *chan, note_state_t voice) {
//     bool updated = false;
    
//     voice.stage = chan->env_start;
//     for (size_t i = 0; i < VOICES; i++) {
//         if (chan->active[i].note == voice.note) {
//             updated = true;
//             chan->active[i].stage = chan->env_start;
//             chan->active[i].velocity = voice.velocity;
//             chan->active[i].time = voice.time;
//             chan->active[i].start_ramp = chan->active[i].ramp;
//             return;
//         }    
//     }

//     if (!updated) {
//         chan->active[chan->active_recent++] = voice;
//         chan->active_recent %= VOICES;
//     }
// }

// static void log_voices(state_t *st, instrument_t *chan) {
//     log_info("voices:");
//     for (size_t i = 0; i < VOICES; i++) {
//         note_state_t voice = chan->active[i];
//         log_line("[%u] = { .note = %u, .velocity = %u, .stage= %lx, .time = %ums }", i, voice.note, voice.velocity, voice.stage, voice.time);
//     }
// }

// static void filter_voice(state_t *st, instrument_t *chan, uint8_t note) {
//     for (size_t i = 0; i < VOICES; i++)
//         if (chan->active[i].note == note) {
//             chan->active[i].time = st->time;
//             chan->active[i].start_ramp = chan->active[i].ramp;
//             chan->active[i].stage = chan->env_done;

//             // env_stage_t *cur = st->active[i].stage;
//             // while (cur) {
//             //     if (cur->time == 0) {
//             //         cur = cur->next;
//             //         break;
//             //     }
//             //     cur = cur->next;
//             // }            
//         }
// }

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
        uint8_t chan = ev.buffer[MIDI_STATUS] & 0x0f;
        uint8_t note = ev.buffer[MIDI_NOTE];
        uint8_t vel = ev.buffer[MIDI_VEL];

        instrument_t *inst = &st->chan[chan];

        switch (kind) {
            case NOTE_ON:
                log_trace("note on (note = %d, vel = %d)", note, vel);
                if (vel != 0)
                    inst->active[note].stage = inst->env_start;
                else
                    inst->active[note].stage = inst->env_done;

                inst->active[note].note = note;
                inst->active[note].velocity = vel;
                inst->active[note].time = st->time;
                inst->active[note].start_ramp = inst->active[note].ramp;
                break;

            case NOTE_OFF:
                log_trace("note off (note = %d, vel = %d)", note, vel);
                inst->active[note].stage = inst->env_done;
                inst->active[note].time = st->time;
                inst->active[note].start_ramp = inst->active[note].ramp;
                break;

            case CONTROL:

                break;

            default:
                log_error("unknown event kind %x", kind);
        }
    }
}

static int process(jack_nframes_t nframes, void *arg) {
    state_t *st = (state_t*)arg;

    jack_nframes_t cur_frames;
    jack_time_t next_usecs;
    float period_usecs;
    jack_get_cycle_times(st->client, &cur_frames, &st->time, &next_usecs, &period_usecs);
    
    void *input_buf = jack_port_get_buffer(st->input, nframes);
    jack_default_audio_sample_t *output_buf = (jack_default_audio_sample_t *)jack_port_get_buffer(st->output, nframes);

    midi_process(st, input_buf);

    for (size_t i = 0; i < nframes; i++) {
        output_buf[i] = 0.0;

        for (size_t j = 0; j < 16; j++) {
            output_buf[i] += osc_sample(st, &st->chan[j]);
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
    log_info("initialising program state");
    log_debug("opening connection to JACK");
    
    jack_status_t status;
    st->client = jack_client_open("midid", JackNullOption, &status);

    if (!st->client)
        return JACK_ERR(status);

    log_debug("opening MIDI input and audio output");

    st->input = jack_port_register(st->client, "midi_in", JACK_DEFAULT_MIDI_TYPE, JackPortIsInput, 0);
    st->output = jack_port_register(st->client, "audio_out", JACK_DEFAULT_AUDIO_TYPE, JackPortIsOutput, 0);

    if (!st->input || !st->output)
        return JACK_ERR(status);

    log_info("program state initialised");

    // setup callback
    jack_set_error_function(jack_error_report);
    jack_set_info_function(jack_error_report);

    log_info("installing callbacks");

    jack_set_process_callback(st->client, process, (void*)st);
    jack_set_sample_rate_callback(st->client, srate, (void *)st);
    jack_set_xrun_callback(st->client, jack_xrun, NULL);

    st->volume = 0.5;
    st->channel = 0;

    for (size_t j = 0; j < 16; j++) {
        instrument_t *inst = &st->chan[j];

        // inst->active_recent = 0;
        inst->env_start = NULL;
        inst->env_done = NULL;

        inst->osc.base = 0;
        inst->osc.kind = OSC_SIN;
        inst->osc.vol = 0.5;
        
        for (size_t i = 0; i < VOICES; i++) {
            note_state_t *ns = &inst->active[i];

            ns->note = 0;
            ns->velocity = 0;
            ns->idx = 0;
            ns->phase = 0.0;
            ns->start_ramp = 0.0;
            ns->ramp = 0.0;
            ns->stage = NULL;
            ns->time = 0;
        }
    }
    
    return OK_VAL;
}
