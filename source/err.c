#include <err.h>
#include <jack/types.h>
#include <log.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void report_result(result_t info) {
    switch (info.kind) {
        case OK:
            break;

        case ERR_LIBC: {
            char *msg = strerror(info.err_libc);
            log_line("libc: %s", msg);
        } break;

        case ERR_JACK: {
            jack_status_t err_jack = info.err_jack;

            if (err_jack & JackFailure) log_line("JACK operation failed");
            if (err_jack & JackInvalidOption) log_line("  option invalid or unsupported");
            if (err_jack & JackNameNotUnique) log_line("  desired client name not unique");
            if (err_jack & JackServerStarted) log_line("  server started");
            if (err_jack & JackServerFailed) log_line("  unable to connect to JACK ");
            if (err_jack & JackNoSuchClient) log_line("  requested client does not exist");
            if (err_jack & JackLoadFailure) log_line("  unable to load internal client");
            if (err_jack & JackInitFailure) log_line("  unable to initialise client");
            if (err_jack & JackShmFailure) log_line("  unable to access shared memory");
            if (err_jack & JackVersionError) log_line("  client's protocol version does not match");
            if (err_jack & JackBackendError) log_line("  backend error");
            if (err_jack & JackClientZombie) log_line("  client zombie");
        } break;

        case ERR_CMDLINE: {
            log_line("cmdline: failed to parse argument '%s'", info.arg);
        } break;

        case ERR_UNKNOWN_WAVE: {
            log_line("unknown wave-form '%s'", info.wave_name);
        } break;

        case ERR_KV: {
            switch (info.kv.kind) {
                case ERR_KV_EMPTY:
                    log_line("kv: empty expression: '%s'", info.kv.expr);
                    break;
                case ERR_KV_NO_VAL:
                    log_line("kv: no value: '%s'", info.kv.expr);
                    break;
                case ERR_KV_UNKNOWN_KEY:
                    log_line("kv: unknown key: '%s'", info.kv.expr);
                    break;
            }
        } break;
    }
}
