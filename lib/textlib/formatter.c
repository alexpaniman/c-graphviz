// #include "formatter.h"

#include <stdarg.h>
#include <stddef.h>

typedef size_t debug_writer(void* argument);

#define debuggable \
    debug_writer __debug_writer_function;

struct test {
    debuggable

    int test;
};

char* test_printer(test* instance) {

}


size_t write_formatted(char* output_buffer, char* format, va_list args) {

}
