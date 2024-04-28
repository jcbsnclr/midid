#include <audio.h>
#include <err.h>
#include <errno.h>
#include <jack/jack.h>
#include <jack/midiport.h>
#include <locale.h>
#include <log.h>
#include <stdio.h>
#include <synth.h>
#include <unistd.h>

#include "jack/types.h"
#include "parse.h"

int main(int argc, char **argv) {
    setlocale(LC_ALL, "C");
    log_init();

    state_t st;
    UNWRAP(state_init(&st));

    const char *in_port_name = jack_port_name(st.input);
    const char *out_port_name = jack_port_name(st.output);
    const char **ports;

    bool activate = true;

    int c;
    while ((c = getopt(argc, argv, "li:o:I:E:O:")) != -1) {
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

                jack_free(ports);
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

                jack_free(ports);
                break;

            case 'I':
                log_warn("flag '%c' unimplemented", c);
                break;

            case 'E':
                activate = false;
                parser_t p;
                parser_init(&p, optarg);
                UNWRAP(parse_env(&st, &p));
                break;

            case 'O':
                activate = false;
                parser_init(&p, optarg);
                UNWRAP(parse_osc(&st, &p));
                break;

            case '?':
                jack_client_close(st.client);
                return 1;

            default:
                log_error("unknown error");
                jack_client_close(st.client);
                return 1;
        }
    }

    if (activate) {
        log_info("activating JACK client");
        jack_activate(st.client);
        getc(stdin);
    }

    state_free(&st);

    return 0;
}
