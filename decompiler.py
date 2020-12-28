import os, sys

INSTRUCTIONS = [
    ("halt", 0),
    ("set", 2),
    ("push", 1),
    ("pop", 1),
    ("eq", 3),
    ("gt", 3),
    ("jmp", 1),
    ("jt", 2),
    ("jf", 2),
    ("add", 3),
    ("mult", 3),
    ("mod", 3),
    ("and", 3),
    ("or", 3),
    ("not", 2),
    ("rmem", 2),
    ("wmem", 2),
    ("call", 1),
    ("ret", 0),
    ("out", 1),
    ("in", 1),
    ("noop", 0)
]

REGISTER_MIN_VALUE = 32768

def process(path: str):
    """Processes the binary file at the given path into a memory array."""
    memory = []
    with open(path, "rb") as file:
        eof = False
        while not eof:
            buffer = file.read(2)
            if len(buffer) < 2:
                eof = True
            else:
                num = ((256 + buffer[0]) % 256) + (((256 + buffer[1]) % 256) * 256)
                memory.append(num)
    return memory

def decompile(memory: list, convert_registers=True, parse_strings=True):
    """Decompiles the given memory array and outputs a list of instructions in a readable format."""
    instructions = []
    out_str = ""

    i = 0
    while i < len(memory):
        # Get the next instruction.
        ins = memory[i]
        i += 1

        if ins < 0 or ins >= len(INSTRUCTIONS):
            continue
        
        name, argc = INSTRUCTIONS[ins]
        args = memory[i:i+argc]
        i += argc

        # Convert registers to $<register> format.
        if convert_registers:
            for j in range(len(args)):
                if args[j] >= REGISTER_MIN_VALUE:
                    args[j] = '$' + str(args[j] - REGISTER_MIN_VALUE)
        
        # Handle the 'out' operation (decompile the actual ASCII rather than the number).
        if parse_strings:
            flush_out_str = False; new_line = False
            if name == 'out' and isinstance(args[0], int):
                ch = None
                if args[0] == 10:
                    ch = '\\n'
                    flush_out_str = True
                    new_line = True
                else:
                    ch = chr(args[0]) 
                out_str += ch
            elif len(out_str) > 0:
                flush_out_str = True
            
            if flush_out_str:
                if len(out_str) == 1:
                    out_str = '\'' + out_str.replace('\'', '\\\'') + '\''
                else:
                    out_str = '"' + out_str.replace('"', '\\"') + '"'
                instructions.append("{0:8} {1}".format("out", out_str))
                out_str = ""
        
        if not parse_strings or (len(out_str) == 0 and not new_line):
            instructions.append("{0:8} {1}".format(name, "".join("{0:8}".format(str(x)) for x in args)).strip())

    return instructions

# Get the input files.
if len(sys.argv) < 2:
    print("No input file(s).")
    exit()

files = sys.argv[1:]
for file in files:
    memory = process(file)
    instructions = decompile(memory)
    with open(os.path.splitext(file)[0] + ".syn", "w", encoding='UTF-8') as output:
        for instruction in instructions:
            output.write(instruction + '\n')
