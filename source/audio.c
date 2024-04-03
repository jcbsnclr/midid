#include "jack/jack.h"
#include "jack/midiport.h"
#include "jack/types.h"
#include "math.h"
#include <audio.h>
#include <log.h>
#include <string.h>
#include <synth.h>

static void insert_voice(state_t *st, note_state_t voice) {
    voice.on = true;
    st->active[st->active_recent++] = voice;
    st->active_recent %= VOICES;
}

static void log_voices(state_t *st) {
    log_info("voices:");
    for (size_t i = 0; i < VOICES; i++) {
        note_state_t voice = st->active[i];
        log_line("[%u] = { .note = %u, .velocity = %u, .on = %s }", i, voice.note, voice.velocity, voice.on ? "true" : "false");
    }
}

static void filter_voice(state_t *st, uint8_t note) {
    for (size_t i = 0; i < VOICES; i++)
        if (st->active[i].note == note)
            st->active[i].on = false;
}

static void jack_error_report(const char *msg) {
    log_trace("JACK: %s", msg);
}

static int jack_xrun(void *arg) {
    (void)arg;
    log_warn("xrun detected");
    return 0;
}

static int process(jack_nframes_t nframes, void *arg) {
    state_t *st = (state_t*)arg;
    
    void *input_buf = jack_port_get_buffer(st->input, nframes);
    jack_default_audio_sample_t *output_buf = (jack_default_audio_sample_t *)jack_port_get_buffer(st->output, nframes);
    uint32_t ev_count = jack_midi_get_event_count(input_buf);
    uint32_t ev_index = 0;

    jack_midi_event_t ev;

    // if (ev_count != 0) {
    //     log_trace("got %u events", ev_count);

    //     for (uint32_t i = 0; i < ev_count; i++) {
    //         jack_midi_event_get(&ev, input_buf, i);
    //         log_line("  event %d time %d. 1st byte 0x%x", i, ev.time, *ev.buffer);
    //     }
    // }

    jack_midi_event_get(&ev, input_buf, 0);
        
    for (size_t i = 0; i < nframes; i++) {
        if (ev_index < ev_count) {
            uint8_t kind = ev.buffer[MIDI_STATUS] & 0xf0;
            uint8_t chan = ev.buffer[MIDI_STATUS] & 0x0f;
            uint8_t note = ev.buffer[MIDI_NOTE];
            uint8_t vel = ev.buffer[MIDI_VEL];
            (void)chan;

            if (chan == st->channel && (kind == NOTE_OFF || (kind == NOTE_ON && vel == 0))) {
                log_info("note off (note = %d, vel = %d)", note, vel);
                filter_voice(st, note);
                // log_voices(st);
            } else if (kind == NOTE_ON && chan == st->channel) {
                log_info("note on (note = %d, vel = %d)", note, vel);
                note_state_t voice = {
                    .note = note,
                    .velocity = vel,
                    .on = true,
                    .phase = 0.0,
                    // .idx = st->sample + i
                    .idx = 0
                };             
                insert_voice(st, voice);
                // log_voices(st);
            } else if (kind == CONTROL && chan == st->channel) {
                // log_info("midi control (controller = %u, value = %u)", ev.buffer[MIDI_CONTROLLER], ev.buffer[MIDI_VALUE]);
            }

            ev_index++;
            if (ev_index < ev_count)
                jack_midi_event_get(&ev, input_buf, ev_index);
        }

        (void)log_voices;

        // output_buf[i] = 0.;
        output_buf[i] = synth_sample(st, st->sample + i, st->volume);
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


    st->active_recent = 0;
    memset(st->active, 0, sizeof(*st->active) * VOICES);
    
    return OK_VAL;
}
