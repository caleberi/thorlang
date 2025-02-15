#include "common.h"
#include "scanner.h"

static void check_error(size_t actual, size_t expected, const char *message, const char *path)
{
    if (actual == expected)
    {
        fprintf(stderr, message, path);
        exit(74);
    }
}

void generate_ast(const char *path)
{
    FILE *fp = fopen(path, "rb");
    if (fp == NULL)
    {
        perror("no file passed to generate_opcode");
        return -1;
    }

    check_error(fseek(fp, 0L, SEEK_END), -1, "could not seek to end of file", path);
    size_t file_sz = ftell(fp);
    check_error(file_sz, -1, "could not calculate file size", path);

    char *buffer = (char *)malloc(file_sz + 1);
    if (buffer == NULL)
    {
        fprintf(stderr, "Not enough memory for reading  source code \"%s\".\n", path);
        exit(74);
    }
    size_t bytes_read = fread(buffer, sizeof(char), file_sz, fp);
    if (bytes_read < file_sz)
    {
        fprintf(stderr, "Could not read file \"%s\".\n", path);
        exit(74);
    }
    buffer[bytes_read] = '\0';
    fclose(fp);

    // TODO:  read the scanner from the
    Scanner scanner = {
        .current = -1,
        .line = 0,
        .start = buffer};

    Tokens tks;
    init_Tokens(&tks);
    scan_tokens(&scanner, &tks.entries);
}
