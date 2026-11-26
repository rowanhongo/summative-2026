# Project 1: Reverse Engineering an ELF Executable

program.c is a small bank transaction program. It keeps six transactions on the heap, adds up the deposits and the withdrawals, and prints the final balance. I used it to study how a C program becomes an ELF file and how it runs.

## Files

- program.c: the source code.
- program: the stripped executable.
- program_debug: the same program built with debug information.
- analysis.sh: builds both versions and saves the output of every command in outputs/.
- gdb_commands.txt: the GDB commands used on program_debug.
- outputs/: the saved output of readelf, objdump, ldd, strace and GDB.

## Environment

Ubuntu 24.04 on WSL2, GCC 13.3.0, GNU binutils 2.42, strace 6.8 and GDB 15.1.

## What the program does

It has a struct called transaction with an id, an amount and a type ('D' for deposit, 'W' for withdrawal). It has three global variables: amounts, totalDeposits and totalWithdrawals. It has exactly three functions besides main().

| Function | Job |
| --- | --- |
| fillTransactions() | Fills the transactions. Every second one is a withdrawal. |
| processTransactions() | Loops over the transactions, adds each amount to the right total and prints a line. |
| printSummary() | Prints total deposits, total withdrawals and the final balance. |

main() allocates the memory with malloc(), calls the three functions and releases the memory with free().

## Compile

```
gcc -Wall -O0 -fno-inline -o program program.c
strip program
gcc -Wall -O0 -g -fno-inline -o program_debug program.c
```

## Run

```
./program
```

There is no input. The six transactions are built into the program.

## Expected output

```
Transaction 1001: deposit of 500
Transaction 1002: withdrawal of 200
Transaction 1003: deposit of 1200
Transaction 1004: withdrawal of 350
Transaction 1005: deposit of 75
Transaction 1006: withdrawal of 125
Total deposits: 1775
Total withdrawals: 675
Final balance: 1100
```

## Analysis commands

bash analysis.sh runs all of these and saves the output in outputs/.

| Command | Saved in outputs/ |
| --- | --- |
| file, ls -l and nm before and after strip | compile.txt |
| ./program | program_output.txt |
| readelf -h program | readelf_header.txt |
| readelf -S -W program | readelf_sections.txt |
| readelf -l -W program | readelf_segments.txt |
| readelf -d program | readelf_dynamic.txt |
| readelf -r -W program | readelf_relocations.txt |
| ldd program | ldd.txt |
| objdump -d -M intel program | objdump_disassembly.txt |
| objdump -s -j .rodata -j .data program | objdump_data.txt |
| objdump -R program | objdump_dynamic_relocs.txt |
| nm -n program_debug | nm_debug_symbols.txt |
| strace ./program with the output sent to /dev/null | strace_not_terminal.txt |
| strace ./program in a terminal (run with script) | strace_terminal.txt |
| gdb -q -batch -x gdb_commands.txt ./program_debug | gdb_session.txt |

To follow the GDB session yourself, run gdb -q -x gdb_commands.txt ./program_debug.

## Notes

- When the output goes to a terminal, printf() writes one line at a time, so strace shows nine write() calls. When the output goes to a file or /dev/null it is buffered, so strace shows one write(). That is why there are two strace files.
- Addresses and process ids in strace and GDB change a little between runs. The code addresses in the stripped file stay the same.
