#include "jack/jack.h"
#include "jack/midiport.h"
#include "jack/types.h"
#include <audio.h>
#include <log.h>
#include <string.h>
#include <synth.h>

static void jack_error_report(const char *msg) {
    log_trace("JACK: %s", msg);
}

static int process(jack_nframes_t nframes, void *arg) {
    state_t *st = (state_t*)arg;
    
    void *input_buf = jack_port_get_buffer(st->input, nframes);
    jack_default_audio_sample_t *output_buf = (jack_default_audio_sample_t *)jack_port_get_buffer(st->output, nframes);
    uint32_t ev_count = jack_midi_get_event_count(input_buf);
    uint32_t ev_index = 0;

    jack_midi_event_t ev;

    log_trace("nframes = %d", nframes);    

    // if (ev_count != 0) {
    //     log_trace("got %u events", ev_count);

    //     for (uint32_t i = 0; i < ev_count; i++) {
    //         jack_midi_event_get(&ev, input_buf, i);
    //         log_line("  event %d time %d. 1st byte 0x%x", i, ev.time, *ev.buffer);
    //     }
    // }

    jack_midi_event_get(&ev, input_buf, 0);

    for (size_t i = 0; i < nframes; i++) {
        if (ev.time == i && ev_index < ev_count) {
            if ((ev.buffer[0] & 0xf0) == 0x90) {
                st->note = ev.buffer[1];
                st->note_on = 1.0;
            } else if ((ev.buffer[0] & 0xf0) == 0x80) {
                st->note = ev.buffer[1];
                st->note_on = 0.0;
            }
            if (ev_index++ < ev_count)
                jack_midi_event_get(&ev, input_buf, ev_index);
        }

        output_buf[i] = synth_sample(st, st->sample + i, 0.5, 0.2) * st->note_on;
        st->note += 3;
        output_buf[i] += synth_sample(st, st->sample + i, 0.5, 0.3) * st->note_on;
        st->note += 2;
        output_buf[i] += synth_sample(st, st->sample + i, 0.5, 0.5) * st->note_on;
        st->note += 2;
        output_buf[i] += synth_sample(st, st->sample + i, 0.5, 0.7) * st->note_on;
        st->note -= 7;

        
        output_buf[i] /= 2;
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
    
    return OK_VAL;
}
