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

#include <jbase.h>

static void midi_cb(void *state, jb_midi_t ev) {
    (void)state;
    (void)ev;

    switch (ev.kind) {
        case JB_NOTE_ON:
            jb_info("note %u on", ev.args[JB_NOTE]);
            break;

        case JB_NOTE_OFF:
            jb_info("note %u off", ev.args[JB_NOTE]);
            break;

        default:
            jb_warn("unhandled MIDI event kind '%x'", ev.kind);
    }
}

static void audio_cb(void *state, jb_ctx_t ctx, size_t nframes, jb_sample_t *buf) {
    (void)state;

    jb_osc_t osc1, osc2;

    // configure oscillator
    osc1.fn = jb_wave_sin;
    osc1.bias = 0.0;
    osc1.amp = 0.5;
    osc1.detune = -JB_SEMIS(12);

    // configure oscillator
    osc2.fn = jb_wave_sin;
    osc2.bias = 0.0;
    osc2.amp = 0.5;
    osc2.detune = -JB_SEMIS(-12);

    jb_osc_link_t o1, o2;

    o1.mod = JB_MOD_PM;
    o1.osc = &osc1;
    // o1.next = &o2;
    o1.next = &o2;

    o2.mod = JB_MOD_AM;
    o2.osc = &osc2;
    o2.next = NULL;

    (void)o2;

    for (size_t i = 0; i < nframes; i++) {
        buf[i] = jb_chain_sample(&o1, JB_A4_MIDI, ctx.cur_sample + i, ctx.srate);
    }
}

jb_res_t start_client(jb_client_t *cl, jb_client_config_t cfg) {
    JB_TRY(jb_client_init(cl, cfg));

    JB_TRY(jb_client_connect_midi(cl, "Through"));
    JB_TRY(jb_client_connect_audio(cl, "Audio Device|oscroll"));

    JB_TRY(jb_client_start(cl));

    return JB_OK_VAL;
}

int main(void) {
    jb_log_init();
    jb_info("Hello, World!");

    jb_client_t cl;
    jb_client_config_t cfg = {.name = "midid", .midi_cb = midi_cb, .audio_cb = audio_cb};

    jb_res_t res = start_client(&cl, cfg);

    if (res JB_IS_ERR) {
        jb_report_result(res);
        return 1;
    }

    return 0;
}
