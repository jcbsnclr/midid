#include "jack/types.h"
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
    
    state_t st;
    UNWRAP(state_init(&st));

    log_info("activating JACK client");
    jack_activate(st.client);

    while(true)
        sleep(1);

    jack_client_close(st.client);

    return 0;
}
