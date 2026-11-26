#!/bin/bash
# builds both versions and saves the raw command outputs in outputs/
# use: bash analysis.sh
cd "$(dirname "$0")"
mkdir -p outputs

{
    echo '$ gcc -Wall -O0 -fno-inline -o program program.c'
    gcc -Wall -O0 -fno-inline -o program program.c
    echo '$ file program   (before strip)'
    file program
    echo '$ ls -l program   (before strip)'
    ls -l program
    echo '$ nm program | head -n 12   (before strip)'
    nm program | head -n 12
    echo '$ strip program'
    strip program
    echo '$ file program   (after strip)'
    file program
    echo '$ ls -l program   (after strip)'
    ls -l program
    echo '$ nm program   (after strip)'
    nm program
    echo '$ gcc -Wall -O0 -g -fno-inline -o program_debug program.c'
    gcc -Wall -O0 -g -fno-inline -o program_debug program.c
    echo '$ file program_debug'
    file program_debug
} > outputs/compile.txt 2>&1

./program > outputs/program_output.txt

readelf -h program > outputs/readelf_header.txt
readelf -S -W program > outputs/readelf_sections.txt
readelf -l -W program > outputs/readelf_segments.txt
readelf -d program > outputs/readelf_dynamic.txt
readelf -r -W program > outputs/readelf_relocations.txt
ldd program > outputs/ldd.txt
objdump -d -M intel program > outputs/objdump_disassembly.txt
objdump -s -j .rodata -j .data program > outputs/objdump_data.txt
objdump -R program > outputs/objdump_dynamic_relocs.txt

# the debug build has the same code addresses, nm gives the function names
nm -n program_debug > outputs/nm_debug_symbols.txt

# output sent to /dev/null is not a terminal, so printf buffers everything into one write
strace -o outputs/strace_not_terminal.txt ./program > /dev/null

# script gives the program a terminal like a normal run, so printf writes line by line
script -qec "strace -o outputs/strace_terminal.txt ./program" /dev/null > /dev/null

# the gdb commands are in gdb_commands.txt
gdb -q -batch -x gdb_commands.txt ./program_debug > outputs/gdb_session.txt 2>&1
