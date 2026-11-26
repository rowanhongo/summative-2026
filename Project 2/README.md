# Project 2: Sensor Data Validation in x86 Assembly

sensor_analysis.asm reads sensor_readings.txt, checks every line and prints the statistics. It is written in NASM for x86-64 Linux. It does not use the C library. It only uses system calls: open, read, write, close and exit.

## Files

- sensor_analysis.asm: the source code.
- sensor_readings.txt: the example input from the brief.
- run_tests.sh: builds the program and runs it on every test case.
- tests/: the input files made by run_tests.sh.
- output.txt: the saved output of run_tests.sh.

## Environment

Ubuntu 24.04 on WSL2, NASM 2.16.01 and GNU ld 2.42.

## Build

```
nasm -f elf64 sensor_analysis.asm -o sensor_analysis.o
ld sensor_analysis.o -o sensor_analysis
```

## Run

```
./sensor_analysis
```

The program must be run in the folder that has sensor_readings.txt. It takes no arguments.

## Input

One measurement per line. A line is valid when it only has digits and the number is from 0 to 100. Leading zeros are fine, so 007 is 7.

A line is invalid when it is:

- empty
- text like abc or 12abc
- negative or has a sign, like -5 or +5
- a decimal like 3.5
- above 100, like 101 or a very long number
- using a space, like " 5"

Unix (LF) and Windows (CRLF) line endings both work. An empty line counts as an invalid record. If the last line has no line feed it still counts as long as it has a character. A file that ends with a line feed does not get an extra record.

## Output

```
Total records: X
Valid measurements: Y
Invalid measurements: Z
Sum of valid measurements: S
Minimum measurement: MIN
Maximum measurement: MAX
```

If there are no valid measurements, the minimum and maximum print N/A. If the file cannot be opened or read, the program prints an error message and exits with code 1.

## Example

With the sensor_readings.txt in this folder (25, 30, invalid, 18, 42, -5):

```
Total records: 6
Valid measurements: 4
Invalid measurements: 2
Sum of valid measurements: 115
Minimum measurement: 18
Maximum measurement: 42
```

The brief shows a sum of 70 and a maximum of 30 for this example. That does not match its own data. The valid numbers are 25, 30, 18 and 42, so the sum is 115 and the maximum is 42. I followed the rules in the brief, so the program prints 115 and 42.

## Tests

```
bash run_tests.sh > output.txt 2>&1
```

| Test | What it checks | Result |
| --- | --- | --- |
| sensor_readings.txt | the example from the brief | 6 records, 4 valid, sum 115, min 18, max 42 |
| unix_no_final_newline | last line without a line feed | same as the example |
| windows_crlf | CRLF line endings | same as the example |
| empty_lines | empty lines | 6 records, 3 valid, 3 invalid |
| mixed_endings | CRLF, an empty CRLF line and LF together | 3 records, 2 valid, 1 invalid |
| range_limits | 0, 100, 101, -1, 007, a huge number, 12abc, 3.5 and " 5" | 9 records, 3 valid, sum 107, min 0, max 100 |
| no_valid | no valid line | min and max print N/A |
| empty_file | file with nothing in it | 0 records, min and max print N/A |
| missing_file | no sensor_readings.txt | open error, exit code 1 |
| read_error | sensor_readings.txt is a folder | read error, exit code 1 |

## How it works

The file is read 4096 bytes at a time, so any file size works. Each character goes through a small set of checks:

- A carriage return is skipped.
- A line feed ends the record. The program counts it and updates the sum, the minimum and the maximum if it was valid.
- A digit is added to the current value with value = value * 10 + digit. If the value goes above 100 the line is marked invalid.
- Any other character marks the line invalid.

The counters and the statistics are kept in memory. The value of the current line, the digit count and the flags are kept in registers r12 to r15. The comments in the source file explain each step.
