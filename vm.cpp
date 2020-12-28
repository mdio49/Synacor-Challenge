#include <iostream>
#include <fstream>
#include <string>
#include <csignal>
#include "architecture.h"
#include "instructions.h"
using namespace std;

// Constants.

constexpr int INITIAL_STACK_SIZE = 32;

// Function definitions.

bool load_program(string path, uint16_t *memory);
bool save_state(string path, State *state, uint16_t *stack, uint32_t stack_size);
State *load_state(string path, uint16_t **stack, uint32_t *stack_size);

char *to_bytes(uint64_t value, int n);
uint64_t from_bytes(char *bytes, int n);

void SIGINT_handler(int signum);

// Global variables.

State *state;
uint16_t *stack;
uint32_t stack_size;

void print_instruction(Instruction instruction, uint16_t *args) {
    cout << instruction.name;
    for (int i = 0; i < instruction.argc; i++) {
        cout << " " << args[i];
    }
    cout << endl;
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        cout << "Usage: ./vm (<program>|-load <state>)" << endl;
        return 1;
    }
    
    // Parse command line arguments.
    bool load = false;
    for (int i = 1; i < argc; i++) {
        string arg = argv[i];
        if (arg.rfind("-", 0) == 0) {
            string par = arg.substr(1);
            if (par == "load") {
                load = true;
            }
            else {
                cout << "Invalid parameter: " << par << endl;
                return 1;
            }
        }
        else {
            if (load) {
                // Load the state.
                state = load_state(arg, &stack, &stack_size);
                if (state == nullptr) {
                    cout << "Cannot load state: " << arg << endl;
                    return 1;
                }

                cout << "State loaded from memory address " << state->insptr - state->memory << "." << endl;
            }
            else {
                // Initialise the VM state.
                state = new State();
                
                // Load the program into memory.
                if (!load_program(arg, state->memory)) {
                    cout << "Cannot load program: " << arg << endl;
                    return 1;
                }

                // Initialise the stack.
                stack_size = INITIAL_STACK_SIZE;
                stack = new uint16_t[stack_size];
                state->stackptr = stack;
                
                // Set the instruction pointer to the start of the program.
                state->insptr = state->memory;
            }
        }
    }

    // Register signals.
    signal(SIGINT, SIGINT_handler);
    
    // Run the program.
    while (state->insptr < state->memory + MEM_BUFFER) {
        // Fetch the next instruction.
        uint16_t ins = *state->insptr;
        if (ins >= N_INSTRUCTIONS) {
            cout << "Invalid instruction: " << ins << endl;
            break;
        }

        // Decode the instruction.
        Instruction instruction = INSTRUCTION_SET[ins];
        state->insptr++;

        // Execute the instruction.
        uint16_t *args = state->insptr;
        state->insptr += instruction.argc;
        //print_instruction(instruction, args);
        int signal = instruction.exec(state, args);

        // Handle the instruction based on the signal it returned.
        bool terminate = false;
        switch (signal) {
            case SIG_ABORT:
                //cout << "Program terminated." << endl;
                terminate = true;
                break;
            case SIG_ERROR:
                terminate = true;
                break;
            case SIG_OP_NOT_IMPLEMENTED:
                //cout << "Instruction not implemented: " << ins << endl;
                break;
        }

        // Check the stack.
        size_t offset = state->stackptr - stack;
        if (offset < 0 || offset > stack_size) {
            cout << "Error: Stack pointer misaligned." << endl;
            terminate = true;
        }
        else if (stack_size > INITIAL_STACK_SIZE && offset < stack_size / 4) {
            // Allocate less memory if it has shrunk more than a quarter of its size.
            uint16_t *new_stack = new uint16_t[stack_size / 2];
            for (int i = 0; i < offset; i++)
                new_stack[i] = stack[i];
            delete stack;
            stack = new_stack;
            state->stackptr = stack + offset;
            stack_size /= 2;
        }
        else if (offset == stack_size) {
            // Allocate more memory for the stack if it has exceeded its size.
            uint16_t *new_stack = new uint16_t[stack_size * 2];
            for (int i = 0; i < offset; i++)
                new_stack[i] = stack[i];
            delete stack;
            stack = new_stack;
            state->stackptr = stack + offset;
            stack_size *= 2;
        }

        // Terminate the program if necessary.
        if (terminate)
            break;
    }

    return 0;
}

void SIGINT_handler(int signum) {
    save_state("latest.dump", state, stack, stack_size);
    exit(signum);
}

bool load_program(string path, uint16_t *memory) {
    ifstream file;
    file.open(path, ios::binary | ios::ate);
    if (!file.is_open())
        return false;
    size_t size = file.tellg();
    file.seekg(0, ios::beg);
    for (int i = 0; i < size / 2; i++) {
        char buffer[2];
        file.read(buffer, 2);
        memory[i] = from_bytes(buffer, 2);
    }
    file.close();
    return true;
}

bool save_state(string path, State *state, uint16_t *stack, uint32_t stack_size) {
    ofstream file;
    file.open(path, ios::binary);
    if (!file.is_open())
        return false;

    // Write the memory.
    for (int i = 0; i < MEM_BUFFER; i++) {
        char *buffer = to_bytes(state->memory[i], 2);
        file.write(buffer, 2);
    }

    // Write the registers.
    for (int i = 0; i < N_REGISTERS; i++) {
        char *buffer = to_bytes(state->registers[i], 2);
        file.write(buffer, 2);
    }

    // Write the stack.
    uint32_t offset = state->stackptr - stack;
    file.write(to_bytes(offset, 4), 4);
    file.write(to_bytes(stack_size, 4), 4);
    for (int i = 0; i < offset; i++) {
        char *buffer = to_bytes(stack[i], 2);
        file.write(buffer, 2);
    }

    // Write the current instruction.
    uint32_t pc = state->insptr - state->memory;
    file.write(to_bytes(pc, 4), 4); 
    
    // Close the file.
    file.close();

    return true;
}

State *load_state(string path, uint16_t **stack, uint32_t *stack_size) {
    ifstream file;
    file.open(path, ios::binary);
    if (!file.is_open())
        return nullptr;
    
    State *state = new State();

    // Read the memory.
    for (int i = 0; i < MEM_BUFFER; i++) {
        char buffer[2];
        file.read(buffer, 2);
        state->memory[i] = from_bytes(buffer, 2);
    }

    // Read the registers.
    for (int i = 0; i < N_REGISTERS; i++) {
        char buffer[2];
        file.read(buffer, 2);
        state->registers[i] = from_bytes(buffer, 2);
    }

    // Read the stack.
    char offset_buffer[4];
    file.read(offset_buffer, 4);
    uint32_t offset = from_bytes(offset_buffer, 4);

    char stack_size_buffer[4];
    file.read(stack_size_buffer, 4);
    *stack_size = from_bytes(stack_size_buffer, 4);

    *stack = new uint16_t[*stack_size];
    state->stackptr = *stack + offset;
    for (int i = 0; i < offset; i++) {
        char buffer[2];
        file.read(buffer, 2);
        (*stack)[i] = from_bytes(buffer, 2);
    }

    // Read the current instruction.
    char pc_buffer[4];
    file.read(pc_buffer, 4);
    uint32_t pc = from_bytes(pc_buffer, 4);
    state->insptr = state->memory + pc;
    
    // Close the file.
    file.close();

    return state;
}

char *to_bytes(uint64_t value, int n) {
    char *result = new char[n];
    for (int i = 0; i < n; i++) {
        result[i] = value & 0xFF;
        value >>= 8;
    }
    return result;
}

uint64_t from_bytes(char *bytes, int n) {
    uint64_t result = 0; int m = 1;
    for (int i = 0; i < n; i++) {
        result += ((256 + bytes[i]) % 256) * m;
        m *= 256;
    }
    return result;
}
