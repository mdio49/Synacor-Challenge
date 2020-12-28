#pragma once

#include <iostream>
#include "architecture.h"
using namespace std;

constexpr int N_INSTRUCTIONS = 22;

typedef struct _Instruction {
    unsigned char id;
    string name;
    int argc;
    int (*exec)(State *state, uint16_t *args);

    _Instruction(unsigned char id, string name, int argc, int (*exec)(State *state, uint16_t *args)) {
        this->id = id;
        this->name = name;
        this->argc = argc;
        this->exec = exec;
    }
} Instruction;

// The instruction set.
const extern Instruction INSTRUCTION_SET[N_INSTRUCTIONS];

int ins_halt(State *state, uint16_t *args);
int ins_set(State *state, uint16_t *args);

int ins_push(State *state, uint16_t *args);
int ins_pop(State *state, uint16_t *args);

int ins_eq(State *state, uint16_t *args);
int ins_gt(State *state, uint16_t *args);

int ins_jump(State *state, uint16_t *args);
int ins_jt(State *state, uint16_t *args);
int ins_jf(State *state, uint16_t *args);

int ins_add(State *state, uint16_t *args);
int ins_mult(State *state, uint16_t *args);
int ins_mod(State *state, uint16_t *args);

int ins_and(State *state, uint16_t *args);
int ins_or(State *state, uint16_t *args);
int ins_not(State *state, uint16_t *args);

int ins_rmem(State *state, uint16_t *args);
int ins_wmem(State *state, uint16_t *args);

int ins_call(State *state, uint16_t *args);
int ins_ret(State *state, uint16_t *args);

int ins_out(State *state, uint16_t *args);
int ins_in(State *state, uint16_t *args);

int ins_noop(State *state, uint16_t *args);
