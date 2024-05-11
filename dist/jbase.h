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

#pragma  once

#include <stdint.h>
#include <stdlib.h>
#include "jack/types.h"

//
// logging facilities: log.c
//

typedef enum {
    JB_TRACE,   // extra verbose info
    JB_DEBUG,   // mostly unimportant
    JB_INFO,    // general info
    JB_WARN,    // something might be wrong
    JB_ERROR    // something is catastrophically wrong
} jb_llevel_t;

// initialise logging, read filter from `LOG_FILTER` env var
void jb_log_init();

// log an individual message to stderr w/ metadata
void jb_log_inner(jb_llevel_t level, const char *filename, uint32_t line, const char *func, char *fmt, ...);
// log an individual line, without any error metadata
void jb_log_line(char *fmt, ...);

// utility macros for logging messages with a given level and printf-formatted message
#define jb_trace(...) jb_log_inner(JB_TRACE, __FILE__, __LINE__, __func__, __VA_ARGS__)
#define jb_debug(...) jb_log_inner(JB_DEBUG, __FILE__, __LINE__, __func__, __VA_ARGS__)
#define jb_info(...)  jb_log_inner(JB_INFO,  __FILE__, __LINE__, __func__, __VA_ARGS__)
#define jb_warn(...)  jb_log_inner(JB_WARN,  __FILE__, __LINE__, __func__, __VA_ARGS__)
#define jb_error(...) jb_log_inner(JB_ERROR, __FILE__, __LINE__, __func__, __VA_ARGS__)

//
// error handling: err.c
//

typedef enum {
    JB_OK,        // nothing wrong
    JB_ERR_JACK,  // JACK operation failed
    JB_ERR_LIBC,  // libc operation failed
    JB_ERR_OOM,   // out of memory (currently unused)
    JB_ERR_USER   // user-defined error info, for users of library
} jb_err_t;

typedef struct{
    jb_err_t kind;    // status of result
    char *msg;        // err message (if kind != JB_OK0)

    const char *file; // file the error originates from (in-source, or at runtime)
    const char *func; // function the error originates in
    size_t line;      // line causing the error
} jb_res_t;

// macros for constructing/checking a jb_res_t
#define JB_OK_VAL ((jb_res_t){ .kind = JB_OK })
#define JB_ERR(k, ...) jb_err_impl((k), __LINE__, __FILE__, __func__, __VA_ARGS__)

#define JB_IS_OK .kind == JB_OK
#define JB_IS_ERR .kind != JB_OK

// if result is error, return result
#define JB_TRY(res) {jb_res_t result = (res); if ((result) JB_IS_ERR) return result;}

// log a result on the terminal
void jb_report_result(jb_res_t info); 
// construct jb_res_t with printf-formatted message
jb_res_t jb_err_impl(jb_err_t kind, size_t line, const char *file, const char *func, char *fmt, ...); 


// 
// audio client 
//

// individual audio sample
typedef jack_default_audio_sample_t jb_sample_t;

// constants to access MIDI event params 
enum {
    // note event
    JB_NOTE = 0,
    JB_VELOCITY = 1,

    // control event
    JB_CONTROLLER = 0,
    JB_VALUE = 1
};

typedef struct {
    enum {
        JB_NOTE_OFF = 0x80,
        JB_NOTE_ON = 0x90,
        JB_CTRL = 0xB0
    } kind;          // MIDI event kind (first 4 bits of status byte)

    uint8_t chan;    // MIDI channel
    uint8_t args[2]; // 2 argument bytes
} jb_midi_t;

typedef struct {
    size_t srate;              // sample rate
    size_t cur_sample;         // current base sample (samples processed up to start of current cycle)

    jack_nframes_t cur_frames; // precise time at start of current cycle (?)
    jack_time_t time;          // absolute time in usecs 
    jack_time_t next_usecs;    // best estimate of time of next sample 
    float period_usecs;        // roughly difference between next_usecs and time (?)
} jb_ctx_t;

// MIDI event processing callback
typedef void (*jb_midi_fn_t)(void *state, jb_midi_t ev);
// audio buffer generating callback
typedef void (*jb_audio_fn_t)(void *state, jb_ctx_t ctx, size_t nframes, jb_sample_t *buf);

typedef struct {
    char *name;             // name of JACK client
    void *state;            // pointer to user-supplied state (accessible in callbacks)

    jb_midi_fn_t midi_cb;   // callback to process MIDI events
    jb_audio_fn_t audio_cb; // callback to generate audio
} jb_client_config_t;

typedef struct {
    jb_client_config_t cfg; // client configuration
    
    jack_client_t *jack;    // JACK client
    jack_port_t *midi_in;   //  MIDI input port
    jack_port_t *audio_out; // audio output port

    jb_ctx_t ctx;           // current context (timing information)
} jb_client_t;

jb_res_t jb_client_init(jb_client_t *cl, jb_client_config_t cfg); // initialise client with config

jb_res_t jb_client_connect_midi(jb_client_t *cl, char *pat);      // connect MIDI input to ports matching pattern
jb_res_t jb_client_connect_audio(jb_client_t *cl, char *pat);     // connect audio output to ports matching pattern

jb_res_t jb_client_start(jb_client_t *cl);                        // activate JACK client

// 
// audio synthesis: synth.c
//

typedef float (*jb_wave_fn_t)(float x, float bias); // wave function (0 <= x < 2pi)
typedef int32_t jb_cents_t;                         // 1 cent = 1/100th of a semitone

typedef enum {
    JB_MOD_AM, // amplitude modulation
    JB_MOD_FM, // frequency modulation
    JB_MOD_PM, // phase modulation
    JB_MOD_BM, // "bias" modulation (pulse-width or wave folding modulation)
    JB_MOD_MAX
} jb_mod_t;

typedef struct {
    jb_wave_fn_t fn;   // wave function
    jb_cents_t detune; // detune (in cents)
    float amp;         // wave amplitude
    float bias;        // amount of folding, or pulse width
} jb_osc_t;

typedef struct jb_osc_link {
    jb_osc_t *osc;            // oscillator
    jb_mod_t mod;             // type of modulation
    struct jb_osc_link *next; // next in chain
} jb_osc_link_t;

// macro to convert semitones to cents
#define JB_SEMIS(semi) ((semi) * 100)

// A4 note (relative to MIDI) in cents
#define JB_A4_MIDI JB_SEMIS(69)
// A4 note in Hz
#define JB_A4_HZ 440.f

float jb_cents_hz(jb_cents_t cents);                                                   // convert cents (in MIDI space) to Hz 
float jb_osc_sample(jb_osc_t *osc, jb_cents_t note, size_t idx, size_t srate);         // sample an oscillator at a given note in cents
float jb_chain_sample(jb_osc_link_t *link, jb_cents_t note, size_t idx, size_t srate); // sample a chain of oscillators modulating eachother

float jb_wave_sin(float x, float bias);
float jb_wave_square(float x, float bias);
float jb_wave_triangle(float x, float bias);
float jb_wave_saw(float x, float bias);
float jb_wave_noise(float x, float bias);

// terminal control
//

#define JB_RESET              "\x1b[0m"
#define JB_BOLD               "\x1b[1m"

#define JB_FG_BLACK           "\x1b[30m"
#define JB_FG_RED             "\x1b[31m"
#define JB_FG_GREEN           "\x1b[32m"
#define JB_FG_YELLOW          "\x1b[33m"
#define JB_FG_BLUE            "\x1b[34m"
#define JB_FG_MAGENTA         "\x1b[35m"
#define JB_FG_CYAN            "\x1b[36m"
#define JB_FG_WHITE           "\x1b[37m"

#define JB_FG_BLACK_BRIGHT    "\x1b[30;1m"
#define JB_FG_RED_BRIGHT      "\x1b[31;1m"
#define JB_FG_GREEN_BRIGHT    "\x1b[32;1m"
#define JB_FG_YELLOW_BRIGHT   "\x1b[33;1m"
#define JB_FG_BLUE_BRIGHT     "\x1b[34;1m"
#define JB_FG_MAGENTA_BRIGHT  "\x1b[35;1m"
#define JB_FG_CYAN_BRIGHT     "\x1b[36;1m"
#define JB_FG_WHITE_BRIGHT    "\x1b[37;1m"


#define JB_BG_BLACK           "\x1b[40m" 
#define JB_BG_RED             "\x1b[41m" 
#define JB_BG_GREEN           "\x1b[42m" 
#define JB_BG_YELLOW          "\x1b[43m" 
#define JB_BG_BLUE            "\x1b[44m" 
#define JB_BG_MAGENTA         "\x1b[45m" 
#define JB_BG_CYAN            "\x1b[46m" 
#define JB_BG_WHITE           "\x1b[47m" 


#define JB_BG_BLACK_BRIGHT    "\x1b[40;1m"
#define JB_BG_RED_BRIGHT      "\x1b[41;1m"
#define JB_BG_GREEN_BRIGHT    "\x1b[42;1m"
#define JB_BG_YELLOW_BRIGHT   "\x1b[43;1m"
#define JB_BG_BLUE_BRIGHT     "\x1b[44;1m"
#define JB_BG_MAGENTA_BRIGHT  "\x1b[45;1m"
#define JB_BG_CYAN_BRIGHT     "\x1b[46;1m"
#define JB_BG_WHITE_BRIGHT    "\x1b[47;1m"
