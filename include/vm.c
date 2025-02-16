#include "common.h"
#include "debug.h"
#include "compiler.h"
#include "vm.h"

VM vm;

STACK_IMPL(stack, Value, stack);

void init_vm() { init_stack(&vm.stack, 256); }
void free_vm() { free_stack(&vm.stack); }

void push(Value value)
{
    push_stack(&vm.stack, (value));
}

Value pop()
{
    return pop_stack(&vm.stack);
}

Value peek(int distance)
{
    return peek_stack(&vm.stack, distance);
}

static void runtime_error(const char *format, ...)
{
    va_list args;
    va_start(args, format);
    vfprintf(stderr, format, args);
    va_end(args);
    fputs("\n", stderr);

    size_t instruction = vm.ip - vm.chunk->code - 1;
    int line = vm.chunk->lines.entries[instruction];
    fprintf(stderr, "[line %d] in script\n", line);
    reset_stack(&vm.stack);
}

static InterpretResult run()
{
#define READ_BYTE() (*vm.ip++)
#define READ_CONSTANT() (vm.chunk->constants.entries[READ_BYTE()])
#define BINARY_OP(valueType, op)                        \
    do                                                  \
    {                                                   \
        if (!IS_NUMBER(peek(0)) || !IS_NUMBER(peek(1))) \
        {                                               \
            runtime_error("operands must be numbers."); \
            return INTERPRET_RUNTIME_ERROR;             \
        }                                               \
        double a = AS_NUMBER(pop());                    \
        double b = AS_NUMBER(pop());                    \
        push(valueType(a op b));                        \
    } while (false);

    for (;;)
    {
#ifdef DEBUG_TRACE_EXECUTION
        printf(" ");
        for (Value *slot = vm.stack.storage; slot < vm.stack.top; slot++)
        {
            printf("[ ");
            print_value(*slot);
            printf(" ]");
        }
        printf("\n");
        disassemble_instruction(vm.chunk, (int)(vm.ip - vm.chunk->code));
#endif
        uint8_t instruction;
        switch (instruction = READ_BYTE())
        {
        case OP_CONSTANT:
        case OP_CONSTANT_LONG:
        {
            Value constant = READ_CONSTANT();
            push(constant);
            break;
        }
        case OP_NEGATE:
            if (!IS_NUMBER(peek(0)))
            {
                runtime_error("operand must be a number.");
                return INTERPRET_RUNTIME_ERROR;
            }
            push(NUMBER_VAL(-AS_NUMBER(pop())));
            break;
        case OP_ADD:
            BINARY_OP(NUMBER_VAL, +);
            break;
        case OP_SUBTRACT:
            BINARY_OP(NUMBER_VAL, -);
            break;
        case OP_MULTIPLY:
            BINARY_OP(NUMBER_VAL, *);
            break;
        case OP_DIVIDE:
            BINARY_OP(NUMBER_VAL, /);
            break;
        case OP_RETURN:
        {
            print_value(pop());
            printf("\n");
            return INTERPRET_OK;
        }
        }
    }
#undef READ_BYTE
#undef READ_CONSTANT
}

InterpretResult interpret(const char *source)
{
    Chunk chunk;
    if (!compile(source, &chunk))
    {
        free_chunk(&chunk);
        return INTERPRET_COMPILE_ERROR;
    }
    vm.chunk = &chunk;
    vm.ip = vm.chunk->code;

    InterpretResult result = run();

    free_chunk(&chunk);
    return INTERPRET_OK;
}
