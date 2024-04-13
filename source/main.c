#include <jack/jack.h>
#include <jack/midiport.h>

#include <errno.h>
#include <unistd.h>

#include <command.h>
#include <synth.h>
#include <err.h>
#include <log.h>
#include <audio.h>
#include <cmdline.h>

result_t test_fail() {
    return LIBC_ERR(EINVAL);
}

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
    st.chan[1].env_start = &a2;
    st.chan[1].env_done = &r2;

    if (cmd_process(&st, argc, argv)) {
        log_info("activating JACK client");
        jack_activate(st.client);

        while(true)
            sleep(1);
    }

    jack_client_close(st.client);

    return 0;
}
