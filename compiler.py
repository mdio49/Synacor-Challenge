import codecs, os, sys, re

INSTRUCTIONS = {
    "halt": (0, 0),
    "set": (1, 2),
    "push": (2, 1),
    "pop": (3, 1),
    "eq": (4, 3),
    "gt": (5, 3),
    "jmp": (6, 1),
    "jt": (7, 2),
    "jf": (8, 2),
    "add": (9, 3),
    "mult": (10, 3),
    "mod": (11, 3),
    "and": (12, 3),
    "or": (13, 3),
    "not": (14, 2),
    "rmem": (15, 2),
    "wmem": (16, 2),
    "call": (17, 1),
    "ret": (18, 0),
    "out": (19, 1),
    "in": (20, 1),
    "noop": (21, 0)
}

MEM_BUFFER = 32768
N_REGISTERS = 8

REGISTER_MIN_VALUE = 32768
MATH_MODULO = 32768

class CompileError(Exception):
    pass

def compile(program: list):
    # Regex patterns.
    END_TOKEN = r'(?:\s*|^)'
    comma_pattern = re.compile(r'\,' + END_TOKEN)
    comment_pattern = re.compile(r'\#.*' + END_TOKEN)
    label_pattern = re.compile(r'(\w+)\:' + END_TOKEN)
    memory_pattern = re.compile(r'\.(\w+)' + END_TOKEN)
    register_pattern = re.compile(r'\$(\d+)' + END_TOKEN)
    reference_pattern = re.compile(r'\@(\w+)' + END_TOKEN)
    string_pattern = re.compile(r'\"((?:[^\"\\]|\\.)+)\"' + END_TOKEN)
    char_pattern = re.compile(r'\'([^\'\\]|\\.)\'' + END_TOKEN)
    number_pattern = re.compile(r'(-?\d+)' + END_TOKEN)
    generic_pattern = re.compile('([^\s]+)' + END_TOKEN)

    output = []
    labels = {}
    pending_references = {}
    for i, line in enumerate(program):
        if line.isspace():
            continue
        line = line.strip()

        instruction = None
        mem_type = None
        mem_matched = False
        manual_write = False
        args = []
        while len(line) > 0:
            # Check for matches.
            comma_match = comma_pattern.match(line)
            comment_match = comment_pattern.match(line)
            label_match = label_pattern.match(line)
            memory_match = memory_pattern.match(line)
            register_match = register_pattern.match(line)
            reference_match = reference_pattern.match(line)
            string_match = string_pattern.match(line)
            char_match = char_pattern.match(line)
            number_match = number_pattern.match(line)
            generic_match = generic_pattern.match(line)

            match = None
            token = None
            if comment_match:
                match = comment_match
            elif instruction or mem_type:
                if mem_type == "word" and comma_match:
                    mem_matched = False
                    match = comma_match
                elif register_match and instruction:
                    register = int(register_match.group(1))
                    if register >= N_REGISTERS:
                        raise CompileError("Invalid register at line {}: {}".format(i + 1, register_match.group(0).strip()))
                    token = register + REGISTER_MIN_VALUE
                    match = register_match
                elif reference_match and (instruction or mem_type == 'word'):
                    label = reference_match.group(1)
                    if label in labels:
                        token = labels[label]
                    else:
                        if label not in pending_references:
                            pending_references[label] = []
                        pending_references[label].append(len(output) + len(args))
                        token = 0
                    match = reference_match
                elif string_match:
                    chars = codecs.decode(string_match.group(1), 'unicode_escape')
                    if instruction and instruction[0] == INSTRUCTIONS['out'][0]:
                        del output[-1]
                        for ch in chars:
                            output.append(instruction[0])
                            output.append(ord(ch))
                        args.append(None)
                        manual_write = True
                    elif mem_type == 'word':
                        for ch in chars:
                            output.append(ord(ch))
                        mem_matched = True
                    else:
                        raise CompileError("Invalid use of string literal at line {}: {}".format(i + 1, string_match.group(0).strip()))
                    match = string_match
                elif char_match and (instruction or mem_type == 'word'):
                    ch = codecs.decode(char_match.group(1), 'unicode_escape')
                    if len(ch) > 1:
                        raise CompileError("Invalid escape character at line {}: {}".format(i + 1, ch))
                    token = ord(ch)
                    match = char_match
                elif number_match:
                    number = int(number_match.group(1))
                    token = number if number >= 0 else number % MATH_MODULO
                    match = number_match
                elif generic_match:
                    raise CompileError("Invalid token at line {}: {}".format(i + 1, generic_match.group(1).strip()))
                else:
                    raise CompileError("Expected token at line {}".format(i + 1))
            elif label_match:
                label = label_match.group(1)
                if label in labels:
                    raise CompileError("Label '{}' at line {} has already been defined".format(label, i + 1))
                labels[label] = len(output)
                if label in pending_references:
                    for loc in pending_references[label]:
                        output[loc] = labels[label]
                    del pending_references[label]
                match = label_match
            elif memory_match:
                mem_type = memory_match.group(1)
                match = memory_match
            elif generic_match:
                ins = generic_match.group(1)
                if ins not in INSTRUCTIONS:
                    raise CompileError("Invalid instruction at line {}: {}".format(i + 1, ins))
                instruction = INSTRUCTIONS[ins]
                output.append(instruction[0])
                match = generic_match
            else:
                raise CompileError("Expected token at line {}".format(i + 1))
            
            if token is not None:
                if instruction:
                    args.append(token)
                elif mem_type:
                    if mem_matched:
                        raise CompileError("Invalid token at line {}: {}".format(i + 1, token))
                    if mem_type == "word":
                        output.append(token)
                    elif mem_type == "space":
                        if match == number_match:
                            output.extend([0] * token)
                        else:
                            raise CompileError("Invalid token at line {} for memory type 'space': {}".format(i + 1, token))
                    elif mem_type == "align":
                        if match == number_match:
                            padding = (token - (len(output) % token)) % token
                            output.extend([0] * padding)
                        else:
                            raise CompileError("Invalid token at line {} for memory type 'align': {}".format(i + 1, token))
                    else:
                        raise CompileError("Invalid memory type at line {}: {}".format(i + 1, mem_type))
                    mem_matched = True

            line = line[len(match.group(0)):]

        if mem_type and not mem_matched:
            raise CompileError("Expected memory value at line {}".format(i + 1))

        if instruction:
            if len(args) != instruction[1]:
                ins = next(k for k, v in INSTRUCTIONS.items() if v == instruction)
                raise CompileError("Invalid argument count for instruction '{}' at line {}: expected {}, not {}".format(ins, i + 1, instruction[1], len(args)))
            if not manual_write:
                output.extend(args)

    if len(pending_references) > 0:
        raise CompileError("Unresolved label: {}".format(next((k for k in pending_references.keys()))))

    return output

# Get the input files.
if len(sys.argv) < 2:
    print("No input file(s).")
    exit()

files = sys.argv[1:]
for path in files:
    output = None
    with open(path, "r") as file:
        try:
            program = file.readlines()
            output = compile(program)
        except CompileError as ex:
            print("Error: " + ex.args[0])
    
    if output:
        with open(os.path.splitext(path)[0] + ".out", "wb") as binary:
            for x in output:
                binary.write(int.to_bytes(x, 2, byteorder='little', signed=False))
