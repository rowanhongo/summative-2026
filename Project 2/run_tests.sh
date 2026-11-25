#!/bin/bash
# builds the program and runs it on several input files
# use: bash run_tests.sh > output.txt 2>&1
cd "$(dirname "$0")"

echo '$ nasm -f elf64 sensor_analysis.asm -o sensor_analysis.o'
nasm -f elf64 sensor_analysis.asm -o sensor_analysis.o
echo '$ ld sensor_analysis.o -o sensor_analysis'
ld sensor_analysis.o -o sensor_analysis
echo
BIN="$(pwd)/sensor_analysis"

# writes tests/<name>/sensor_readings.txt from the printf text, shows it and runs the program there
runTest()
{
    mkdir -p tests/$1
    printf "$2" > tests/$1/sensor_readings.txt
    echo "##### $1: $3"
    echo "input (cat -A shows \$ for a line feed and ^M for a carriage return):"

    if [ -s tests/$1/sensor_readings.txt ]; then
        cat -A tests/$1/sensor_readings.txt

        # a file that does not end with a line feed needs its own line break here
        if [ -n "$(tail -c 1 tests/$1/sensor_readings.txt)" ]; then
            echo
        fi
    else
        echo "(empty file)"
    fi

    echo "output:"
    (cd tests/$1 && "$BIN")
    echo "exit code: $?"
    echo
}

echo "##### sensor_readings.txt: the example file in this folder"
echo "input:"
cat -A sensor_readings.txt
echo "output:"
./sensor_analysis
echo "exit code: $?"
echo

runTest unix_no_final_newline '25\n30\ninvalid\n18\n42\n-5' "same data, the last line has no line feed"
runTest windows_crlf '25\r\n30\r\ninvalid\r\n18\r\n42\r\n-5\r\n' "same data with Windows line endings"
runTest empty_lines '10\n\n20\n\n\n30\n' "empty lines count as invalid records"
runTest mixed_endings '5\r\n\r\n7\n' "CRLF, an empty CRLF line and LF together"
runTest range_limits '0\n100\n101\n-1\n007\n99999999999999999999\n12abc\n3.5\n 5\n' "limits of the range and bad formats"
runTest no_valid 'abc\n-3\n200\n' "no valid measurement at all"
runTest empty_file '' "file with nothing in it"

mkdir -p tests/missing_file
rm -f tests/missing_file/sensor_readings.txt
echo "##### missing_file: sensor_readings.txt does not exist"
echo "output:"
(cd tests/missing_file && "$BIN")
echo "exit code: $?"
echo

# a folder can be opened but not read, so this reaches the read error
rm -rf tests/read_error
mkdir -p tests/read_error/sensor_readings.txt
echo "##### read_error: sensor_readings.txt is a folder, open works but read fails"
echo "output:"
(cd tests/read_error && "$BIN")
echo "exit code: $?"
