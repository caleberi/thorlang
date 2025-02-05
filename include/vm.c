#include "common.h"
#include "debug.h"
#include "compiler.h"
#include "vm.h"

VM vm;

STACK_IMPL(stack, Value, stack);

void init_vm() { init_stack(&vm.stack, 256); }
void free_vm() { free_stack(&vm.stack); }

static InterpretResult run()
{
#define READ_BYTE() (*vm.ip++)
#define READ_CONSTANT() (vm.chunk->constants.entries[READ_BYTE()])
#define BINARY_OP(op)     \
    do                    \
    {                     \
        double a = pop(); \
        double b = pop(); \
        push(a op b);     \
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

            push(-pop());
            break;
        case OP_ADD:
            BINARY_OP(+);
            break;
        case OP_SUBTRACT:
            BINARY_OP(-);
            break;
        case OP_MULTIPLY:
            BINARY_OP(*);
            break;
        case OP_DIVIDE:
            BINARY_OP(/);
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

void push(Value value) { push_stack(&vm.stack, value); }

Value pop() { return pop_stack(&vm.stack); }

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
