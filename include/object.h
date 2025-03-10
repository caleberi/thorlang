#ifndef __THOR_OBJECT__ // __THOR_OBJECT__
#define __THOR_OBJECT__

#include "common.h"
#include "value.h"

#define AS_OBJ(value) ((value).pas.obj)
#define OBJ_TYPE(value) (AS_OBJ(value)->type)
#define IS_STRING(value) is_obj_type(value, OBJ_STRING)
#define AS_STRING(value) ((ObjString *)AS_OBJ(value))
#define AS_CSTRING(value) (((ObjString *)AS_OBJ(value))->chars)

typedef enum ObjType
{
    OBJ_STRING,
} ObjType;

struct Obj
{
    ObjType type;
    struct Obj *next;
};

struct ObjString
{
    Obj obj;
    int length;
    char chars[];
};

ObjString *copy_string(const char *chars, int length);
ObjString *take_string(char *chars, int length);
void print_object(Value value);

static inline bool is_obj_type(Value value, ObjType type)
{
    return IS_OBJ(value) && AS_OBJ(value)->type == type;
}

#endif // __THOR_OBJECT__