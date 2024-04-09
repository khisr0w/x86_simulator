/*  +======| File Info |===============================================================+
    |                                                                                  |
    |     Subdirectory:  /src                                                          |
    |    Creation date:  1/9/2024 3:55:55 AM                                           |
    |    Last Modified:                                                                |
    |                                                                                  |
    +======================================| Copyright © Sayed Abid Hashimi |==========+  */

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>

typedef uint32_t u32;
typedef int32_t i32;
typedef int64_t i64;
typedef uintptr_t uintptr;
typedef float f32;
typedef double f64;
typedef int8_t bool;
typedef uint8_t u8;
typedef int8_t i8;
typedef uint16_t u16;
typedef int16_t i16;
#define internal static
#define local_persist static
#define global_var static
#define true 1
#define false 0
#define ArraySize(Arr) sizeof((Arr)) / sizeof((Arr)[0])
#define Assert(Expr, ErrorStr) if(!(Expr)) { fprintf(stderr, "ASSERTION ERROR (%s:%d): " ErrorStr "\nExiting...\n", __FILE__, __LINE__); *(i32 *)0 = 0; }

#include "decode8086.c"
#include "simulate8086.c"

internal inline byte_stream
ReadBinaryFileIntoStream(char *FileName) {
    byte_stream Stream = {0};

    FILE* File;
    if(fopen_s(&File, FileName, "rb") != 0) return Stream;

    /* NOTE(Abid): Get the size of the File */
    fseek(File, 0, SEEK_END);
    size_t FileSize = ftell(File);
    rewind(File);

    // u8* Content = (u8 *)malloc(FileSize);

    /* NOTE(Abid): Read into the start of the main memory */
    u8* Content = GLOBALMemory;
    if (Content == NULL) {
        fclose(File);
        return Stream;
    }

    size_t BytesRead = fread(Content, 1, FileSize, File);
    if (BytesRead != FileSize) {
        fclose(File);
        free(Content);
        return Stream;
    }
    fclose(File);

    Stream.Bytes = Content;
    Stream.NumBytes = BytesRead;
    Stream.Loaded = true;

    return Stream;
}

/* NOTE(Abid): We are assuming 16-bit instructions for now */
i32 main(i32 argc, char *argv[]) {
    if(argc < 2) {
        printf("Please provide an .asm file\n");
        return 0;
    }

    char *FileName = argv[1];
    byte_stream ByteStream = ReadBinaryFileIntoStream(FileName);
    Assert(ByteStream.Loaded, "could not open the binary file.");

    /* NOTE(Abid): Here is how the new simulation loop will work:
     *             We will go through the instruction stream byte by byte, giving
     *             the decoder the byte stream as well as the pointer to ip.
     *             We then count the number of bytes we have passed and update ip
     *             accordingly. The decoder, therefore, updates the ip as well as
     *             decoding. Then, before simulating the code, we have access to
     *             our ip and can use the jump instructions.
     */

    while(((u16 *)GLOBALRegisters)[IP_REG_16_IDX] < ByteStream.NumBytes) {
        instruction Inst = DecodeNext(&ByteStream, ((u16 *)GLOBALRegisters) + IP_REG_16_IDX);
        SimulateNext(&Inst);
    }

    printf("\nFinal registers:\n");
    for(u16 Idx = 0; Idx < ArraySize(GLOBALRegisters)/2; ++Idx) {
        printf("\t%s: ", GLOBALRegToStr[Idx]);

        /* NOTE(Abid): Print the whole register only */
        PrintRegister(2*Idx, true); printf(" ("); PrintRegister(2*Idx, false); printf(")\n");
    }

    /* NOTE(Abid): Dump the memory to .data file */
    FILE* MemoryData;
    fopen_s(&MemoryData, "memory.data", "wb");
    fwrite(GLOBALMemory, ArraySize(GLOBALMemory), 1, MemoryData);

    return 0;
}
