#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

// First implementation - Pointer-based String
struct PString
{
    int length;
    char *chars;
};

// Second implementation - Flexible array member String
struct FString
{
    int length;
    char chars[]; // flexible array
};

// Create a pointer-based string
struct PString *create_pstring(const char *source, int length)
{
    struct PString *str = malloc(sizeof(struct PString));
    if (str == NULL)
        return NULL;

    str->chars = malloc(sizeof(char) * (length + 1)); // +1 for null terminator
    if (str->chars == NULL)
    {
        free(str);
        return NULL;
    }

    str->length = length;
    memcpy(str->chars, source, length);
    str->chars[length] = '\0';
    return str;
}

// Free pointer-based string
void free_pstring(struct PString *str)
{
    if (str != NULL)
    {
        free(str->chars);
        free(str);
    }
}

// Create a flexible array member string
struct FString *create_fstring(const char *source, int length)
{
    struct FString *str = malloc(sizeof(struct FString) + length + 1); // +1 for null terminator
    if (str == NULL)
        return NULL;

    str->length = length;
    memcpy(str->chars, source, length);
    str->chars[length] = '\0';
    return str;
}

// Free flexible array member string
void free_fstring(struct FString *str)
{
    free(str);
}

// Helper function to generate random strings
char *generate_random_string(int length)
{
    static const char charset[] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
    char *str = malloc(length + 1);
    if (str == NULL)
        return NULL;

    for (int i = 0; i < length; i++)
    {
        int key = rand() % (sizeof(charset) - 1);
        str[i] = charset[key];
    }
    str[length] = '\0';
    return str;
}

// Benchmark creation and access performance
void benchmark(int iterations, int string_length)
{
    clock_t start, end;
    double p_create_time, f_create_time, p_access_time, f_access_time;
    double p_total_create = 0, f_total_create = 0, p_total_access = 0, f_total_access = 0;

    struct PString **p_strings = malloc(iterations * sizeof(struct PString *));
    struct FString **f_strings = malloc(iterations * sizeof(struct FString *));
    char **source_strings = malloc(iterations * sizeof(char *));

    if (!p_strings || !f_strings || !source_strings)
    {
        printf("Memory allocation failed\n");
        return;
    }

    // Generate random strings for testing
    for (int i = 0; i < iterations; i++)
    {
        source_strings[i] = generate_random_string(string_length);
        if (!source_strings[i])
        {
            printf("Failed to generate random string\n");
            return;
        }
    }

    // Benchmark PString creation
    start = clock();
    for (int i = 0; i < iterations; i++)
    {
        p_strings[i] = create_pstring(source_strings[i], string_length);
    }
    end = clock();
    p_create_time = ((double)(end - start)) / CLOCKS_PER_SEC;
    p_total_create += p_create_time;

    // Benchmark FString creation
    start = clock();
    for (int i = 0; i < iterations; i++)
    {
        f_strings[i] = create_fstring(source_strings[i], string_length);
    }
    end = clock();
    f_create_time = ((double)(end - start)) / CLOCKS_PER_SEC;
    f_total_create += f_create_time;

    // Benchmark PString access (sum ASCII values to ensure compiler doesn't optimize away)
    long p_sum = 0;
    start = clock();
    for (int i = 0; i < iterations; i++)
    {
        for (int j = 0; j < p_strings[i]->length; j++)
        {
            p_sum += p_strings[i]->chars[j];
        }
    }
    end = clock();
    p_access_time = ((double)(end - start)) / CLOCKS_PER_SEC;
    p_total_access += p_access_time;

    // Benchmark FString access
    long f_sum = 0;
    start = clock();
    for (int i = 0; i < iterations; i++)
    {
        for (int j = 0; j < f_strings[i]->length; j++)
        {
            f_sum += f_strings[i]->chars[j];
        }
    }
    end = clock();
    f_access_time = ((double)(end - start)) / CLOCKS_PER_SEC;
    f_total_access += f_access_time;

    // Print results
    printf("String length: %d, Iterations: %d\n", string_length, iterations);
    printf("PString creation time: %.6f seconds\n", p_create_time);
    printf("FString creation time: %.6f seconds\n", f_create_time);
    printf("Creation speedup: %.2f%%\n", (p_create_time - f_create_time) / p_create_time * 100);

    printf("PString access time: %.6f seconds\n", p_access_time);
    printf("FString access time: %.6f seconds\n", f_access_time);
    printf("Access speedup: %.2f%%\n", (p_access_time - f_access_time) / p_access_time * 100);

    // Verify sums match (should be equal)
    printf("Access check sums (should match): P=%ld, F=%ld\n", p_sum, f_sum);

    // Clean up
    for (int i = 0; i < iterations; i++)
    {
        free_pstring(p_strings[i]);
        free_fstring(f_strings[i]);
        free(source_strings[i]);
    }
    free(p_strings);
    free(f_strings);
    free(source_strings);
}

int main(int argc, char const *argv[])
{
    char a[4] = "cat";
    srand(time(NULL));

    printf("---- Small Strings Benchmark ----\n");
    benchmark(100000, 16);

    printf("\n---- Medium Strings Benchmark ----\n");
    benchmark(50000, 128);

    printf("\n---- Large Strings Benchmark ----\n");
    benchmark(10000, 1024);

    return 0;
}