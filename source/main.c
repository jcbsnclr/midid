#include <jack/jack.h>
#include <jack/midiport.h>

#include <errno.h>
#include <unistd.h>

#include <command.h>
#include <synth.h>
#include <err.h>
#include <log.h>
#include <audio.h>

result_t test_fail() {
    return LIBC_ERR(EINVAL);
}

int main(int argc, char **argv) {
    (void)argc;
    (void)argv;

    env_stage_t a, s, r;

    a.amp = 1.0;
    a.time = SECS(1);

    s.amp = 1.0;
    s.time = ENV_SUSTAIN;

    r.amp = 0.0;
    r.time = SECS(1);    

    a.next = &s;
    s.next = &r;
    r.next = NULL;

    state_t st;
    UNWRAP(state_init(&st));

    st.env_start = &a;
    st.env_done = &r;

    log_info("activating JACK client");
    jack_activate(st.client);

    while(true)
        sleep(1);

    jack_client_close(st.client);

    return 0;
}
