#pragma once

#include <cstdio>
#include <cstdlib>
#include <assert.h>

template <typename E>
struct simple_stack {
    E* elements;

    size_t length;
    size_t used;
};

static const size_t init_nmemb = 10;

static const size_t grow_coefficient = 2.0;

template <typename E>
void simple_stack_create(simple_stack<E>* const stack) {
    stack->elements = (E*) calloc(sizeof(E), init_nmemb);

    stack->length = init_nmemb;
    stack->used = 0;
}

template <typename E>
void simple_stack_push(simple_stack<E>* const stack, const E element) {
    if (stack->length == stack->used) {
        stack->length *= grow_coefficient;
        E* new_space = (E*) realloc(stack->elements, stack->length * sizeof(E));

        assert(new_space != NULL);

        stack->elements = new_space;
    }

    stack->elements[stack->used ++] = element;
}

template <typename E>
E simple_stack_peek(simple_stack<E>* const stack) {
    assert(stack->used > 0); // TODO
    return stack->elements[stack->used - 1];
}

template <typename E>
E simple_stack_pop(simple_stack<E>* const stack) {
    const size_t shrinked_size = stack->length / grow_coefficient;

    if (stack->used - 1 < shrinked_size && shrinked_size >= init_nmemb) {
        stack->length = shrinked_size;
        E* new_space = (E*) realloc(stack->elements, stack->length * sizeof(E));

        assert(new_space != NULL);

        stack->elements = new_space;
    }

    return stack->elements[-- stack->used];
}

template <typename E>
void simple_stack_reverse(simple_stack<E>* const stack) {
    for (int low = 0, high = stack->used - 1; low < high; low++, high--) {
        E temp                = stack->elements[ low];
        stack->elements[ low] = stack->elements[high];
        stack->elements[high] = temp;
    }
}

template <typename E>
void simple_stack_destruct(simple_stack<E>* const stack) {
    free(stack->elements);
    stack->length = stack->used = 0;
}

#define SIMPLE_STACK_TRAVERSE(stack, type, current)                     \
    for (type* current = (stack)->elements;                             \
         current < (stack)->elements + (stack)->used; ++ current)

#define SIMPLE_STACK_VALUE(element) (*element)
