#include "architecture.h"
#include "instructions.h"
#include <iostream>
#include <functional>
using namespace std;

const Instruction INSTRUCTION_SET[N_INSTRUCTIONS] = {
    Instruction(0, "halt", 0, &ins_halt),
    Instruction(1, "set", 2, &ins_set),
    Instruction(2, "push", 1, &ins_push),
    Instruction(3, "pop", 1, &ins_pop),
    Instruction(4, "eq", 3, &ins_eq),
    Instruction(5, "gt", 3, &ins_gt),
    Instruction(6, "jmp", 1, &ins_jump),
    Instruction(7, "jt", 2, &ins_jt),
    Instruction(8, "jf", 2, &ins_jf),
    Instruction(9, "add", 3, &ins_add),
    Instruction(10, "mult", 3, &ins_mult),
    Instruction(11, "mod", 3, &ins_mod),
    Instruction(12, "and", 3, &ins_and),
    Instruction(13, "or", 3, &ins_or),
    Instruction(14, "not", 2, &ins_not),
    Instruction(15, "rmem", 2, &ins_rmem),
    Instruction(16, "wmem", 2, &ins_wmem),
    Instruction(17, "call", 1, &ins_call),
    Instruction(18, "ret", 0, &ins_ret),
    Instruction(19, "out", 1, &ins_out),
    Instruction(20, "in", 1, &ins_in),
    Instruction(21, "noop", 0, &ins_noop)
};

// Returns the register corresponding to the given literal value.
short get_register(uint16_t value) {
    return value - REGISTER_MIN_VALUE;
}

// Determines if the given literal value corresponds to a register. 
bool is_register(uint16_t value) {
    return value >= REGISTER_MIN_VALUE;
}

// Resolves the given literal value. If the number refers to a register, then the
// number from that register is fetched; otherwise, the number itself is retured.
short resolve_literal(uint16_t value, uint16_t *registers) {
    if (is_register(value)) {
        short reg = get_register(value);
        return registers[reg];
    }
    return value;
}

// A helper function that conducts a binary operation <a> = <b> + <c> for an instruction with 3 arguments.
void binary_op(State *state, uint16_t *args, function<uint32_t(int, int)> op) {
    short reg = get_register(args[0]);
    int num1 = resolve_literal(args[1], state->registers);
    int num2 = resolve_literal(args[2], state->registers);
    state->registers[reg] = op(num1, num2);
}

int ins_halt(State *state, uint16_t *args) {
    return SIG_ABORT;
}

int ins_set(State *state, uint16_t *args) {
    short reg = get_register(args[0]);
    state->registers[reg] = resolve_literal(args[1], state->registers);
    return SIG_OKAY;
}

int ins_push(State *state, uint16_t *args) {
    short value = resolve_literal(args[0], state->registers);
    *state->stackptr = value;
    state->stackptr++;
    return SIG_OKAY;
}

int ins_pop(State *state, uint16_t *args) {
    short reg = get_register(args[0]);
    state->stackptr--;
    state->registers[reg] = *state->stackptr;
    return SIG_OKAY;
}

int ins_eq(State *state, uint16_t *args) {
    binary_op(state, args, [](int a, int b) {
        return (a == b) ? 1 : 0;
    });
    return SIG_OKAY;
}

int ins_gt(State *state, uint16_t *args) {
    binary_op(state, args, [](int a, int b) {
        return (a > b) ? 1 : 0;
    });
    return SIG_OKAY;
}

int ins_jump(State *state, uint16_t *args) {
    short dest = resolve_literal(args[0], state->registers);
    state->insptr = state->memory + dest;
    return SIG_OKAY;
}

int ins_jt(State *state, uint16_t *args) {
    short value = resolve_literal(args[0], state->registers);
    if (value != 0)
        return ins_jump(state, args + 1);
    return SIG_OKAY;
}

int ins_jf(State *state, uint16_t *args) {
    short value = resolve_literal(args[0], state->registers);
    if (value == 0)
        return ins_jump(state, args + 1);
    return SIG_OKAY;
}

int ins_add(State *state, uint16_t *args) {
    binary_op(state, args, [](int a, int b) {
        return (a + b) % MATH_MODULO;
    });
    return SIG_OKAY;
}

int ins_mult(State *state, uint16_t *args) {
    binary_op(state, args, [](int a, int b) {
        return (a * b) % MATH_MODULO;
    });
    return SIG_OKAY;
}

int ins_mod(State *state, uint16_t *args) {
    binary_op(state, args, [](int a, int b) {
        return a % b;
    });
    return SIG_OKAY;
}

int ins_and(State *state, uint16_t *args) {
    binary_op(state, args, [](int a, int b) {
        return a & b;
    });
    return SIG_OKAY;
}

int ins_or(State *state, uint16_t *args) {
    binary_op(state, args, [](int a, int b) {
        return a | b;
    });
    return SIG_OKAY;
}

int ins_not(State *state, uint16_t *args) {
    short reg = get_register(args[0]);
    short value = resolve_literal(args[1], state->registers);
    state->registers[reg] = 0x7FFF - value;
    return SIG_OKAY;
}

int ins_rmem(State *state, uint16_t *args) {
    short reg = get_register(args[0]);
    short addr = resolve_literal(args[1], state->registers);
    state->registers[reg] = state->memory[addr];
    return SIG_OKAY;
}

int ins_wmem(State *state, uint16_t *args) {
    short addr = resolve_literal(args[0], state->registers);
    short value = resolve_literal(args[1], state->registers);
    state->memory[addr] = value;
    return SIG_OKAY;
}

int ins_call(State *state, uint16_t *args) {
    *state->stackptr = state->insptr - state->memory;
    *state->stackptr++;
    return ins_jump(state, args);
}

int ins_ret(State *state, uint16_t *args) {
    *state->stackptr--;
    return ins_jump(state, state->stackptr);
}

int ins_out(State *state, uint16_t *args) {
    short ch = resolve_literal(args[0], state->registers);
    cout << (char)ch;
    return SIG_OKAY;
}

int ins_in(State *state, uint16_t *args) {
    char ch = cin.get();
    short reg = get_register(args[0]);
    state->registers[reg] = ch;
    return SIG_OKAY;
}

int ins_noop(State *state, uint16_t *args) {
    return SIG_OKAY;
}
