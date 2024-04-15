#include <errno.h>
#include <kv.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include "audio.h"
#include "log.h"

#define KV_ERR(k, e) ((result_t){.kind = ERR_KV, .kv = {.kind = (k), .expr = (e)}})

result_t kv_any(char *str, void *out) {
    char **o = (char **)out;
    *o = strdup(str);
    return OK_VAL;
}

result_t kv_wave(char *str, void *out) {
    osc_kind_t *o = (osc_kind_t *)out;

    if (strcmp(str, "sin") == 0)
        *o = OSC_SIN;
    else if (strcmp(str, "square") == 0)
        *o = OSC_SQUARE;
    else if (strcmp(str, "triangle") == 0)
        *o = OSC_TRIANGLE;
    else if (strcmp(str, "saw") == 0)
        *o = OSC_SAW;
    else if (strcmp(str, "noise") == 0)
        *o = OSC_NOISE;
    else
        return (result_t){.kind = ERR_UNKNOWN_WAVE, .wave_name = str};

    return OK_VAL;
}

result_t kv_int(char *str, void *out) {
    int *outp = (int *)out;

    int o = 0;
    if (sscanf(str, "%d", &o) == 1) {
        *outp = o;
    } else {
        return KV_ERR(ERR_KV_NO_VAL, str);
    }

    return OK_VAL;
}

result_t kv_byte(char *str, void *out) {
    int8_t *outp = (int8_t *)out;

    int o = 0;
    if (sscanf(str, "%d", &o) == 1) {
        *outp = o;
    } else {
        return KV_ERR(ERR_KV_NO_VAL, str);
    }

    return OK_VAL;
}

result_t kv_float(char *str, void *out) {
    float *outp = (float *)out;

    float o = 0;
    if (sscanf(str, "%f", &o) == 1) {
        *outp = o;
    } else {
        return KV_ERR(ERR_KV_NO_VAL, str);
    }

    return OK_VAL;
}

result_t kv_parse(kv_field_t *fields, char *str) {
    while (*str) {
        char key[32] = {0};
        char val[32] = {0};
        int taken = 0;

        int res = sscanf(str, " %31[^=]=%31s%n", key, val, &taken);

        if (res == EOF)
            break;
        else if (res == 0)
            return KV_ERR(ERR_KV_EMPTY, str);
        else if (res == 1)
            return KV_ERR(ERR_KV_NO_VAL, str);

        bool found = false;
        kv_field_t *cur = fields;
        while (cur->key != NULL) {
            if (strcmp(key, cur->key) == 0) {
                found = true;
                TRY(cur->parser(val, cur->out));
            }

            cur++;
        }

        if (!found) return KV_ERR(ERR_KV_UNKNOWN_KEY, key);

        str += taken;
    }

    return OK_VAL;
}
