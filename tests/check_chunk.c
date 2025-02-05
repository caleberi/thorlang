#include <check.h>
#include "./include/chunk.h"

#define assert_chunk_init_details(chunk) \
    ck_assert(chunk.capacity == 0);      \
    ck_assert(chunk.count == 0);         \
    ck_assert(chunk.code == NULL);

START_TEST(test_chunk_create)
{
    Chunk chunk;
    init_chunk(&chunk);
    assert_chunk_init_details(chunk);
}
END_TEST

START_TEST(test_chunk_allocation_and_deallocation)
{
    Chunk chunk;
    init_chunk(&chunk);
    assert_chunk_init_details(chunk);
    write_chunk(&chunk, OP_RETURN, 1);
    write_chunk(&chunk, OP_RETURN, 1);
    ck_assert(chunk.count > 0 && chunk.count == 2);
    free_chunk(&chunk);
    assert_chunk_init_details(chunk);
}
END_TEST

int main(void)
{
    return 0;
}
