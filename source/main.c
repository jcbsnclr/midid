#include <audio.h>
#include <err.h>
#include <errno.h>
#include <jack/jack.h>
#include <jack/midiport.h>
#include <log.h>
#include <stdio.h>
#include <synth.h>
#include <unistd.h>

#include "jack/types.h"

result_t test_fail() { return LIBC_ERR(EINVAL); }

#define CMD_ERR(e) \
    (result_t) { .kind = ERR_CMDLINE, .arg = (e) }

int main(int argc, char **argv) {
    (void)argc;
    (void)argv;

    env_stage_t a1, d1, s1, r1;

    a1.amp = 1.0;
    a1.time = SECS(0.03);

    d1.amp = 0.35;
    d1.time = SECS(0.05);

    s1.amp = 1.0;
    s1.time = ENV_SUSTAIN;

    r1.amp = 0.0;
    r1.time = SECS(0.3);

    a1.next = &d1;
    d1.next = &s1;
    s1.next = &r1;
    r1.next = NULL;

    env_stage_t a2, d2, s2, r2;

    a2.amp = 1.0;
    a2.time = SECS(0.05);

    d2.amp = 0.80;
    d2.time = SECS(0.25);

    s2.amp = 1.0;
    s2.time = ENV_SUSTAIN;

    r2.amp = 0.0;
    r2.time = SECS(0.5);

    a2.next = &d2;
    d2.next = &s2;
    s2.next = &r2;
    r2.next = NULL;

    state_t st;
    UNWRAP(state_init(&st));

    st.chan[0].osc.base = 0;
    st.chan[0].osc.kind = OSC_SIN;
    st.chan[0].osc.vol = 0.5;
    st.chan[0].env_start = &a1;
    st.chan[0].env_done = &r1;

    st.chan[1].osc.base = 0;
    st.chan[1].osc.kind = OSC_SQUARE;
    st.chan[1].osc.vol = 0.1;
    st.chan[1].osc.bias = 0.9;
    st.chan[1].env_start = &a2;
    st.chan[1].env_done = &r2;

    const char *in_port_name = jack_port_name(st.input);
    const char *out_port_name = jack_port_name(st.output);

    const char **ports;
    int c;
    while ((c = getopt(argc, argv, "li:o:")) != -1) {
        switch (c) {
            case 'l':
                ports = jack_get_ports(st.client, "", JACK_DEFAULT_MIDI_TYPE, JackPortIsOutput);
                if (!ports) {
                    log_error("no MIDI ports found");
                    jack_client_close(st.client);
                    jack_free(ports);
                    return 1;
                }

                log_info("MIDI Ports:");
                for (const char **cur = ports; *cur; cur++) {
                    log_line(" * %s", *cur);
                }

                ports = jack_get_ports(st.client, "", JACK_DEFAULT_AUDIO_TYPE, JackPortIsInput);
                if (!ports) {
                    log_error("no MIDI ports found");
                    jack_client_close(st.client);
                    jack_free(ports);
                    return 1;
                }

                log_line("Audio Ports:");
                for (const char **cur = ports; *cur; cur++) {
                    log_line(" * %s", *cur);
                }

                return 0;

            case 'i':
                ports = jack_get_ports(st.client, optarg, JACK_DEFAULT_MIDI_TYPE, JackPortIsOutput);
                if (!ports) {
                    log_error("no MIDI ports found");
                    jack_client_close(st.client);
                    jack_free(ports);
                    return 1;
                }

                for (const char **cur = ports; *cur; cur++) {
                    jack_connect(st.client, *cur, in_port_name);
                }

                break;

            case 'o':
                ports = jack_get_ports(st.client, optarg, JACK_DEFAULT_AUDIO_TYPE, JackPortIsInput);
                if (!ports) {
                    log_error("no audio ports found");
                    jack_client_close(st.client);
                    jack_free(ports);
                    return 1;
                }

                for (const char **cur = ports; *cur; cur++) {
                    jack_connect(st.client, out_port_name, *cur);
                }

                break;

            case '?':
                jack_client_close(st.client);
                return 1;

            default:
                log_error("unknown error");
                jack_free(ports);
                jack_client_close(st.client);
                return 1;
        }
    }

    // const char **ports = jack_get_ports(st.client, "",
    // JACK_DEFAULT_MIDI_TYPE, JackPortIsOutput);

    // if (!ports) {
    // log_error("null");
    // return 1;
    // }

    // const char **cur = ports;
    // log_debug("MIDI inputs:");

    // for (const char **cur = ports; *cur != NULL; cur++)
    // log_line(" * %s", *cur);

    log_info("activating JACK client");
    jack_activate(st.client);

    getc(stdin);

    jack_free(ports);
    jack_client_close(st.client);

    return 0;
}
