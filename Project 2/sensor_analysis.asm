; sensor_analysis.asm
; NASM, x86-64 Linux, no C library, only system calls
; build: nasm -f elf64 sensor_analysis.asm -o sensor_analysis.o
;        ld sensor_analysis.o -o sensor_analysis
; run:   ./sensor_analysis   (reads sensor_readings.txt from the current folder)

section .data
    fileName    db "sensor_readings.txt", 0
    msgTotal    db "Total records: ", 0
    msgValid    db "Valid measurements: ", 0
    msgInvalid  db "Invalid measurements: ", 0
    msgSum      db "Sum of valid measurements: ", 0
    msgMin      db "Minimum measurement: ", 0
    msgMax      db "Maximum measurement: ", 0
    msgNone     db "N/A", 10, 0
    msgOpenErr  db "Error: could not open sensor_readings.txt", 10, 0
    msgReadErr  db "Error: could not read sensor_readings.txt", 10, 0

section .bss
    buffer      resb 4096
    numBuf      resb 21
    total       resq 1
    valid       resq 1
    invalid     resq 1
    sum         resq 1
    minimum     resq 1
    maximum     resq 1

section .text
global _start

; registers used while reading the file
; rbx = file descriptor
; r8  = bytes in the buffer, r9 = position in the buffer
; r12 = value of the current line, r13 = digits seen in the line
; r14 = 1 if the line is invalid, r15 = 1 if the line has any character

_start:
    ; open sensor_readings.txt read only
    mov rax, 2
    mov rdi, fileName
    mov rsi, 0
    syscall
    cmp rax, 0
    jl openError            ; a negative result means the open failed
    mov rbx, rax

    ; every valid value is 0 to 100 so the first valid one replaces these
    mov qword [minimum], 100
    mov qword [maximum], 0

    xor r12, r12
    xor r13, r13
    xor r14, r14
    xor r15, r15

readBlock:
    ; read up to 4096 bytes, this repeats until the file ends
    mov rax, 0
    mov rdi, rbx
    mov rsi, buffer
    mov rdx, 4096
    syscall
    cmp rax, 0
    jl readError
    je endOfFile            ; 0 bytes read means the end of the file
    mov r8, rax
    xor r9, r9

nextChar:
    ; loop over the block one character at a time
    cmp r9, r8
    jae readBlock
    movzx eax, byte [buffer + r9]
    inc r9

    cmp al, 13              ; the CR of a Windows line ending is ignored
    je nextChar
    cmp al, 10              ; a line feed ends the record
    je lineEnd

    mov r15, 1              ; the line is not empty
    cmp al, '0'
    jb notDigit
    cmp al, '9'
    ja notDigit
    cmp r14, 1              ; already invalid, no need to convert more
    je nextChar

    ; text to number: value = value * 10 + digit
    sub eax, '0'
    imul r12, r12, 10
    add r12, rax
    inc r13
    cmp r12, 100            ; checked after every digit so a long number cannot overflow
    jbe nextChar

notDigit:
    mov r14, 1              ; a letter, a sign, a dot or a value above 100
    jmp nextChar

lineEnd:
    call endRecord
    jmp nextChar

endOfFile:
    ; the last line may have no line feed, it still counts if it has characters
    cmp r15, 1
    jne closeFile
    call endRecord

closeFile:
    mov rax, 3
    mov rdi, rbx
    syscall

    mov rsi, msgTotal
    call printStr
    mov rax, [total]
    call printNum

    mov rsi, msgValid
    call printStr
    mov rax, [valid]
    call printNum

    mov rsi, msgInvalid
    call printStr
    mov rax, [invalid]
    call printNum

    mov rsi, msgSum
    call printStr
    mov rax, [sum]
    call printNum

    ; minimum and maximum mean nothing when there is no valid measurement
    cmp qword [valid], 0
    je noValid

    mov rsi, msgMin
    call printStr
    mov rax, [minimum]
    call printNum

    mov rsi, msgMax
    call printStr
    mov rax, [maximum]
    call printNum
    jmp exitOk

noValid:
    mov rsi, msgMin
    call printStr
    mov rsi, msgNone
    call printStr
    mov rsi, msgMax
    call printStr
    mov rsi, msgNone
    call printStr

exitOk:
    mov rax, 60             ; exit(0)
    xor rdi, rdi
    syscall

openError:
    mov rsi, msgOpenErr
    call printStr
    mov rax, 60             ; exit(1)
    mov rdi, 1
    syscall

readError:
    mov rsi, msgReadErr
    call printStr
    mov rax, 3              ; the file is closed before leaving
    mov rdi, rbx
    syscall
    mov rax, 60
    mov rdi, 1
    syscall

; counts the finished line and updates the statistics, then clears the line state
endRecord:
    inc qword [total]
    cmp r14, 1
    je recordBad
    cmp r13, 0              ; no digits at all means an empty line
    je recordBad

    inc qword [valid]
    add [sum], r12
    cmp r12, [minimum]
    jge checkMax
    mov [minimum], r12
checkMax:
    cmp r12, [maximum]
    jle recordDone
    mov [maximum], r12
    jmp recordDone

recordBad:
    inc qword [invalid]

recordDone:
    xor r12, r12
    xor r13, r13
    xor r14, r14
    xor r15, r15
    ret

; prints the text that rsi points to, it must end with a 0
printStr:
    xor rdx, rdx
countLen:
    cmp byte [rsi + rdx], 0
    je doWrite
    inc rdx
    jmp countLen
doWrite:
    mov rax, 1
    mov rdi, 1
    syscall
    ret

; prints the number in rax followed by a line feed
printNum:
    mov rdi, numBuf + 20    ; digits are filled in from the back
    mov byte [rdi], 10
    mov rcx, 10
nextDigit:
    xor rdx, rdx
    div rcx                 ; rax = rax / 10, rdx = the last digit
    add dl, '0'
    dec rdi
    mov [rdi], dl
    test rax, rax
    jnz nextDigit
    mov rsi, rdi
    mov rdx, numBuf + 21
    sub rdx, rsi            ; length of the text
    mov rax, 1
    mov rdi, 1
    syscall
    ret
