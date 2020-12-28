#pragma once

#include <stdint.h>

// Constants.

constexpr int MEM_BUFFER = 32768;
constexpr int N_REGISTERS = 8;

constexpr int REGISTER_MIN_VALUE = 32768;
constexpr int MATH_MODULO = 32768;

// Signals.

constexpr int SIG_OKAY = 0x00;
constexpr int SIG_ABORT = 0x01;
constexpr int SIG_ERROR = 0x02;
constexpr int SIG_OP_NOT_IMPLEMENTED = 0x03;

typedef struct _State {
    uint16_t memory[MEM_BUFFER];
    uint16_t registers[N_REGISTERS];
    uint16_t *insptr;
    uint16_t *stackptr;
} State;
