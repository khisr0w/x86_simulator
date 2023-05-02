/*  +======| File Info |===============================================================+
    |                                                                                  |
    |     Subdirectory:  /optimization_practice                                        |
    |    Creation date:  Di 02 Mai 2023 15:10:01 CEST                                  |
    |    Last Modified:                                                                |
    |                                                                                  |
    +=====================| Sayed Abid Hashimi, Copyright © All rights reserved |======+  */

#include <stdio.h>
#include <stdlib.h>

static char *
GetFileContent(char *FileName)
{
    FILE* File = fopen(FileName, "r");
    if (File == NULL) return NULL; 

    // Get the size of the File
    fseek(File, 0, SEEK_END);
    size_t FileSize = ftell(File);
    rewind(File);

    // Allocate memory for the File content
    char* Content = (char *)malloc(FileSize + 1);
    if (Content == NULL) {
        fclose(File);
        return NULL;
    }

    // Read the content of the File
    size_t BytesRead = fread(Content, 1, FileSize, File);
    if (BytesRead != FileSize) {
        fclose(File);
        free(Content);
        return NULL;
    }
    Content[FileSize] = '\0';
    fclose(File);

    return Content;
}

typedef struct word word;

struct word
{
    char *Ptr;
    word *Next;
};

typedef struct 
{
    word *Init;
} parser;

static word *
ParseWords(char *Code)
{
    word *CurrentWord = NULL;

    while(*Code)
    {
        word *Word = (word *)malloc(sizeof(word));
        Word->Ptr = Code;

        if(CurrentWord) CurrentWord->Next = Word;
        CurrentWord = Word;

        while(*(Code++) != '\0');
        *Code = '\0';
    }

    return CurrentWord;
}

// NOTE(Abid): Debug this thing
int main()
{
    char *Str = GetFileContent("code.asm");
    printf("File contains:\n%s\n", Str);
    return 0;
}
