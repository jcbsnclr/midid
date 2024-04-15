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
#include "kv.h"

result_t test_fail() {
    return LIBC_ERR(EINVAL);
}

#define CMD_ERR(e)                      \
    (result_t) {                        \
        .kind = ERR_CMDLINE, .arg = (e) \
    }

int main(int argc, char **argv) {
    (void)argc;
    (void)argv;

    // env_stage_t a1, d1, s1, r1;

    // a1.amp = 1.0;
    // a1.time = SECS(0.03);

    // d1.amp = 0.35;
    // d1.time = SECS(1);

    // s1.time = ENV_SUSTAIN;

    // r1.amp = 0.0;
    // r1.time = SECS(0.25);

    // a1.next = &s1;
    // s1.next = &d1;
    // d1.next = &r1;
    // r1.next = NULL;

    state_t st;
    UNWRAP(state_init(&st));

    // st.inst_pool[st.inst_len].chan = 0;
    // st.inst_pool[st.inst_len].osc.base = -12;
    // st.inst_pool[st.inst_len].osc.kind = OSC_SQUARE;
    // st.inst_pool[st.inst_len].osc.bias = 0.0;
    // st.inst_pool[st.inst_len].osc.vol = 0.25;
    // st.inst_pool[st.inst_len].env.start = &a1;
    // st.inst_pool[st.inst_len].env.done = &r1;

    // st.inst_len++;

    // st.inst_pool[st.inst_len].chan = 0;
    // st.inst_pool[st.inst_len].osc.base = 0;
    // st.inst_pool[st.inst_len].osc.kind = OSC_SQUARE;
    // st.inst_pool[st.inst_len].osc.bias = 0.33;
    // st.inst_pool[st.inst_len].osc.vol = 0.25;
    // st.inst_pool[st.inst_len].env.start = &a1;
    // st.inst_pool[st.inst_len].env.done = &r1;

    // st.inst_len++;

    // st.inst_pool[st.inst_len].chan = 0;
    // st.inst_pool[st.inst_len].osc.base = 0;
    // st.inst_pool[st.inst_len].osc.kind = OSC_SAW;
    // st.inst_pool[st.inst_len].osc.vol = 0.25;
    // st.inst_pool[st.inst_len].env.start = &a1;
    // st.inst_pool[st.inst_len].env.done = &r1;

    // st.inst_len++;

    env_stage_t env_pool[ENVELOPES];
    size_t env_frame = 0;
    size_t env_len = 0;

    const char *in_port_name = jack_port_name(st.input);
    const char *out_port_name = jack_port_name(st.output);

    const char **ports;
    int c;
    while ((c = getopt(argc, argv, "li:o:I:")) != -1) {
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

            case 'I':
                (void)0;

                instrument_t *inst = &st.inst_pool[st.inst_len];

                env_frame = env_len;

                for (;;) {
                    float t;
                    float a;
                    int n = 0;
                    if (sscanf(optarg, " %fs%f%n", &t, &a, &n) == 2) {
                        env_pool[env_len].amp = a;
                        env_pool[env_len].time = SECS(t);

                    } else if (sscanf(optarg, " SUST%n", &n) != EOF && n >= 4) {
                        env_pool[env_len].time = ENV_SUSTAIN;
                    } else {
                        return 1;
                    }
                    env_pool[env_len].next = NULL;
                    optarg += n;

                    if (env_len - env_frame > 0) {
                        env_pool[env_len - 1].next = &env_pool[env_len];

                        if (env_pool[env_len - 1].time == ENV_SUSTAIN) {
                            inst->env.done = &env_pool[env_len];
                        }
                    } else {
                        inst->env.start = &env_pool[env_len];
                    }

                    env_len++;

                    n = 0;
                    int res = sscanf(optarg, " ->%n", &n);
                    if (res == EOF || n < 2) {
                        break;
                    } else {
                        optarg += n;
                    }
                }

                kv_field_t fields[] = {{.key = "wave", .out = &inst->osc1.kind, .parser = kv_wave},
                                       {.key = "base", .out = &inst->osc1.base, .parser = kv_int},
                                       {.key = "vol", .out = &inst->osc1.vol, .parser = kv_float},
                                       {.key = "bias", .out = &inst->osc1.bias, .parser = kv_float},
                                       {.key = "chan", .out = &inst->chan, .parser = kv_byte},
                                       {NULL}};

                inst->osc2.kind = OSC_SQUARE;
                inst->osc2.base = 0;
                inst->osc2.vol = 0.5;

                UNWRAP(kv_parse(fields, optarg));

                if (inst->chan == 255) {
                    log_error("you must specify instrument channel");
                    return 1;
                }

                st.inst_len++;

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

    log_info("activating JACK client");
    jack_activate(st.client);
    getc(stdin);

    // char *str = "wave=square base=0 vol=0.5 bias=0.5 chan=0";
    // UNWRAP(kv_parse(fields, str));

    // log_error("osc = %s", osc_kind_str[wave]);

    jack_free(ports);
    jack_client_close(st.client);

    return 0;
}
