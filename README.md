# Synacor Challenge
This repositry contains my implemenation for the [Synacor Challenge](https://challenge.synacor.com/) in C++ with some additional features. Included is a virtual machine capable of running binaries coded for the given architecture as well as a compiler that allows you to write your own programs using a more readable assembly-style language.

## Virtual Machine
To run the virtual machine, simply compile the source files using g++ (`vm.cpp` and `instructions.cpp`). The program can then be run via the terminal using the binary file as the first command-line argument.
```
./vm <program>
```
This will run the program using the virtual machine. If the program is interrupted via a keyboard interrupt (i.e. `^C`), then the current state of the VM is dumped to the file `latest.dump`, which can be used to restore the VM back to the state it had before the program was interrupted. This can be done using the `-load` directive instead of directly inputting a program.
```
./vm -load latest.dump
```

## Compiler
The compiler is written in Python and can be used to compile source files (`.syn`) into executable binary files that can be run by the virtual machine. To compile a file, simply list the input files as command-line arguments and the compiler will compile each of them individually and output them as `<filename>.out`.
```
python compiler.py <files ...>
```

### Syncode
Syncode is an extension of the architecture provided that allows users to write source files in a readable language and compile them into binary files that can be executed by the virual machine. This description assumes that you're familiar with the architecture of this challenge and will only go over the additional features added to make the language easier to use. After reading the features, there are plenty of examples which demonstrate how to use the language to write actual programs. While the architecture is limited, it can still be useful to write basic programs, and the language features along with the compiler should make this experience far more enjoyable.

#### Instructions
As with most assembly languages, each line contains an instruction along with the arguments for that instruction, however, instead of using the number for the instruction (as described by the spec), you would use the actual name for that instruction. For example, `set 32768 10` would set the value of register 0 to 10.

#### Registers
Registers can be referred to using the syntax `$<register>` instead of the number that is used in the bytecode. So, the instruction `set $0 10` would be equivalent to the previous example, where `$0` refers to register 0, which gets converted to the number `32768` at compile time. If, instead, the argument requires a value, then using this syntax would dereference the register and extract its value. For example, `add $0 $1 5` would take the value of register 1, add 5 to it, and store it in register 0.

#### Characters
ASCII characters can be referred to using the actual character rather than the number. For example, `out 'a'` would output the character `a` to the terminal.

#### Strings
As an extension, you may also use full strings with the `out` instruction in order to specify multiple characters to be outputted to the terminal. This can be indicated using double quotes and would get converted to multiple individual `out` instructions at compile time. For example, `out "Hello World!"` would print the full `Hello World!` string to the terminal.

#### Comments
In-line comments may be easily specified using hash-tags '#' in exactly the same manner as Python; any text following it is ignored by the compiler.
```
# This is a comment.
set $0 0    # This is another comment.
```

#### Labels
Labels can be placed around the program which get resolved to the memory address of the next instruction at runtime. These labels can then be referenced using the `@` operator, which would dereference the label to the exact memory address that it refers to relative to the start of the program (memory address 0). This is useful for jump operations as it makes it easier to jump to a particular point in the program without needing to keep track of the program's memory address space.
```
set $0 0
jmp @loop
out "Jump failed!"

loop:
    eq $1 $0 10
    jt @loop_end
    add $0 0 1
    jmp @loop

loop_end:
    out "Finished!"

halt
```

#### Memory
Values can be directly stored in the program's memory using the `.` directive; this specifies the memory-type, which is then followed by a value or series of values that all define how to affect the memory at the current location in the program. There are 3 memory types: `.word`, `.space` and `.align`.
- `.word` defines a single literal value, or multiple values separated by commas (`,`) to be written to memory.
- `.space` reserves a block of memory that spans the given number of memory addresses. This memory is automatically zeroed by the compiler.
- `.align` pads the memory with zeroes such that the next memory address is a multiple of the number specified.

```
number:
    .word 42                # Stores the number '42'.
hello:
    .word "Hello world!\0"  # Stores the null-terminated string "Hello World".
array:
    .space 8                # Reserves a block of memory that spans 8 memory addresses.
```
It is wise for a label to preceed the memory directive so that the memory may be easily accessed later by the program.
```
rmem $0 @number     # Reads the memory at the address of label 'number' and stores it in register 0.
mod $0 $0 10        # Extracts the last digit.
add $0 $0 '0'
out $0              # Prints this value to the terminal.
```
You can also specify a constant offset for the memory address by suffixing the label with a plus or minus sign (`+/-`) followed by a number. This is to provide convenience when working with arrays/structs.
```
wmem @array+0 1     # Writes 1 to memory at the address of label 'array' with no offset.
wmem @array+1 2     # Writes 2 to memory at the address of label 'array' with an offset of 1 (i.e. the next address after 'array').
wmem @array+2 3     # Writes 2 to memory at the address of label 'array' with an offset of 2 (i.e. the next 2 addresses after 'array).
# ...
```
Note however that you would still need to store the address in a register and do appropriate operations if you wish to offset it by a dynamic value. So, something like `@array+$0` would be invalid.

## Decompiler
There is also a decompiler written in Python which takes a binary file (or series of files) and attempts to decompile it into a more readable format, however this is experimental and often doesn't prove to be very useful.
