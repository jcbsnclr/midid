#include <command.h>

#include <stdlib.h>
#include <errno.h>

size_t command_parse(size_t argc, char **argv, command_t *cmd) {
    cmd->notes = malloc(sizeof(uint16_t) * argc - 1);
    cmd->len = argc - 1;

    for (size_t i = 1; i < argc; i++) {
        char *end;
        long note = strtol(argv[i], &end, 10);

        if (argv[i] == end) {
            return i;
        } else if (errno == ERANGE) {
            return i;
        }  else {
            cmd->notes[i - 1] = note;    
        }
    }
    
    return 0;
}
