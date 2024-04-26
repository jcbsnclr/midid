#include <err.h>
#include <jack/types.h>
#include <log.h>
#include <string.h>

static size_t resolve(char *src, size_t pos) {
    size_t line = 1;

    for (size_t i = 0; i < pos && src[i]; i++)
        if (src[i] == '\n') line++;

    return line;
}

void report_result(result_t info) {
    size_t line;

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

        case ERR_EXPECTED_NAME:
            line = resolve(info.src, info.where);
            log_line("  line %lu: expected name", line);
            break;
        case ERR_EXPECTED_CHAR:
            line = resolve(info.src, info.where);
            log_line("  line %lu: expected char '%c'", line, info.expected);
            break;
        case ERR_EXPECTED_NUM:
            line = resolve(info.src, info.where);
            log_line("  line %lu: expected number", line);
            break;
        case ERR_EXPECTED_IDENT:
            line = resolve(info.src, info.where);
            log_line("  line %lu: expected ident", line);
            break;
        case ERR_EXPECTED_LIT:
            line = resolve(info.src, info.where);
            log_line("  line %lu: expected lit '%s'", line, info.exp_lit);
            break;

        case ERR_EXPECTED_PAIR:
            line = resolve(info.src, info.where);
            log_line(" line %lu: expected pair", line);
            break;

        case ERR_EXPECTED_VALUE:
            line = resolve(info.src, info.where);
            log_line(" line %lu: expected value", line);
            break;

        case ERR_KEY_REQUIRED:
            line = resolve(info.src, info.where);
            log_line(" line %lu: key '%s' required", info.key);
            break;

        case ERR_EXPECTED_WAVE:
            line = resolve(info.src, info.where);
            log_line("  line %lu: expected wave", line);
            break;

        case ERR_UNEXPECTED_EOF:
            line = resolve(info.src, info.where);
            log_line("  line %lu: unexpected EOF", line);
            break;

        case ERR_KEY_INVALID:
            line = resolve(info.src, info.where);
            log_line("  line %lu: invalid key '%.*s'", line, info.len, info.key);
            break;

        case ERR_MEM_POOL: {
            log_line("failed to allocate memory pool");
        } break;

        case ERR_OOM: {
            log_line("out of memory");
        } break;
    }
}
