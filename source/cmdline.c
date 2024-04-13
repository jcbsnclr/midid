#include <cmdline.h>

typedef enum arg_id_t {
    NONE = 0,
    HELP,
    LIST
} arg_id_t;

typedef struct arg_t {
    arg_id_t id;
    // short name
    char s;
    // long name
    char *l;
} arg_t;

bool cmd_process(state_t *st, int argc, char *argv[]) {
    return true;
}
