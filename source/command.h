#pragma once

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

typedef struct command {
    uint16_t *notes;
    size_t len;
} command_t;

size_t command_parse(size_t argc, char **argv, command_t *cmd);
