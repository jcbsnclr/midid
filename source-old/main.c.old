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

#include <audio.h>
#include <err.h>
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
    parser_t p;
    while ((c = getopt(argc, argv, "li:o:I:E:O:C:")) != -1) {
        switch (c) {
            case 'l':  // list available MIDI/audio ports
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

            case 'i':  // connects MIDI input to ports matching regex
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

            case 'o':  // connects audio output to ports matching regex
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
                parser_init(&p, optarg);
                UNWRAP(parse_inst(&st, &p));
                break;

            case 'E':  // parse an envelope
                parser_init(&p, optarg);
                UNWRAP(parse_env(&st, &p));
                break;

            case 'O':  // parse an instrument
                parser_init(&p, optarg);
                UNWRAP(parse_osc(&st, &p));
                break;

            case 'C':
                parser_init(&p, optarg);
                UNWRAP(parse_chan(&st, &p));
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

    log_state(&st);

    if (activate) {
        log_info("activating JACK client");
        jack_activate(st.client);
        getc(stdin);
    }

    state_free(&st);

    return 0;
}
