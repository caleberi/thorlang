#include "./include/common.h"
#include "./include/memory.h"
#include "./include/debug.h"
#include "./include/alloc.h"
#include "./include/vm.h"

void repl();
static char *readFile(const char *);
static void runFile(const char *);
static void check_error(size_t, size_t, const char *, const char *);

int main(int argc, const char *argv[])
{
    init_vm();
    switch (argc)
    {
    case 1:
        repl();
        break;
    case 2:
        runFile(argv[1]);
        break;
    default:
        fprintf(stderr, "Usage: clox [path]\n");
        exit(64);
    }

    free_vm();

    // int *x = (int *)p_malloc(sizeof(int));
    // *x = 100;
    // p_free(x);
    return 0;
}

void repl()
{
    char line[1024];
    for (;;)
    {
        printf(">> ");
        if (!fgets(line, sizeof(line), stdin))
        {
            printf("\n");
            break;
        }
        interpret(line);
    }
}

static void check_error(size_t actual, size_t expected, const char *message, const char *path)
{
    if (actual == expected)
    {
        fprintf(stderr, message, path);
        exit(74);
    }
}

static char *readFile(const char *path)
{
    FILE *file = fopen(path, "rb");
    if (file == NULL)
    {
        fprintf(stderr, "Could not open file \"%s\".\n", path);
        exit(74);
    }
    check_error(fseek(file, 0L, SEEK_END), (size_t)-1, "Could not seek to the end of the file \"%s\".\n", path);
    size_t file_size = ftell(file);
    check_error(file_size, (size_t)-1, "Could not obtain the current value of the file position \"%s\".\n", path);
    rewind(file);
    char *buffer = (char *)malloc(file_size + 1);
    if (buffer == NULL)
    {
        fprintf(stderr, "Not enough memory for reading  source code \"%s\".\n", path);
        exit(74);
    }
    size_t bytes_read = fread(buffer, sizeof(char), file_size, file);
    if (bytes_read < file_size)
    {
        fprintf(stderr, "Could not read file \"%s\".\n", path);
        exit(74);
    }
    buffer[bytes_read] = '\0';

    fclose(file);
    return buffer;
}

static void runFile(const char *path)
{
    char *source = readFile(path);
    InterpretResult result = interpret(source);
    free(source);
    if (result == INTERPRET_COMPILE_ERROR)
        exit(65);
    if (result == INTERPRET_RUNTIME_ERROR)
        exit(70);
}
