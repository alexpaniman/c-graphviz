#ifndef GRAPH_H
#define GRAPH_H

#include <cassert>
#include <cerrno>
#include <cmath>
#include <csetjmp>
#include <cstdarg>
#include <cstdbool>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cwchar>
#include <malloc.h>
#include <sys/stat.h>


static char* vsprintf_to_new_buffer(const char* format, va_list args);

struct line {
    const wchar_t* begin;
    size_t length;
};

struct text {
    wchar_t* buffer;

    line* lines;
    size_t number_of_lines;
};


/**
 * Error codes meant primarily for #error.
 * 
 * If you want to make your own error with unique code,
 * you should create second enum like #error_codes in
 * which first value will be +1 bigger then the last
 * value in #error_codes, so they don't overlap.
 */
enum error_codes {
    SUCCESS,      //!< Successfully finished

    LOGIC_ERROR,  //!< Bug that causes program to operate incorrectly,
                  //!< but not to terminate abnormally

    RUNTIME_ERROR //!< Error that occurs due to illegal operation in 
                  //!< a program, that causes it to terminate
};

/**
 * Struct that describes where #error happend
 * It's usually constructed via #__LINE__, #__FUNCTION__ and #__ macro
 */
struct occurance {
    int line;             //!< Line number on which error occured
    const char* file;     //!< Full name of file in which error occured

    const char* function; //!< Full name of function in which error occured 
};

/**
 * Error description. You can use it in conjuction with
 * #error_codes to describe your own errors.
 *
 * If you want to make error with unique error code, you
 * should look into documentation for #error_codes
 *
 * @note There's error code #SUCCESS
 */
struct error {
    int error_code;    //!< Error code from #error_codes or extended from it enum

    char* description; //!< Error description in english
    occurance occured;
};

/**
 * Represents linked list of errors that caused each
 * other in chronological order.
 */
struct stack_trace {
    error latest_error; //!< Last error in chronological order

    stack_trace *trace; //!< Linked list of errors that cause it,
                        //!< #NULL if doesn't have cause
};


static stack_trace* __trace_create_success();
static stack_trace* __trace_create_failure(stack_trace* cause,
    int code, occurance occured, const char* message, ...);


#define __TRACE_CREATE_OCCURANCE()         \
    (occurance { .line = __LINE__,         \
                 .file = __FILE__,         \
                 .function = __FUNCTION__ })


#define SUCCESS() __trace_create_success()

#define FAILURE(            code, ...)                                         \
    __trace_create_failure( NULL, code, __TRACE_CREATE_OCCURANCE(), __VA_ARGS__)

#define PASS_FAILURE(cause, code, ...)                                         \
    __trace_create_failure(cause, code, __TRACE_CREATE_OCCURANCE(), __VA_ARGS__)


static bool trace_is_success(stack_trace* trace);

static void trace_print_stack_trace(FILE* stream, stack_trace* trace);

static void trace_destruct(stack_trace* trace);


#define TRY                                                       \
    do {                                                          \
        stack_trace* __trace = ({

#define CATCH(impl)                                               \
        ;   });                                                   \
        if (!trace_is_success(__trace)) {                         \
            impl                                                  \
        }                                                         \
    } while(false)

#define FAIL(...)                                                 \
    CATCH({                                                       \
        return PASS_FAILURE(__trace, RUNTIME_ERROR, __VA_ARGS__); \
    })                                                            \

#define THROW(...)                                                \
    CATCH({                                                       \
        stack_trace* __current_trace =                            \
            PASS_FAILURE(__trace, RUNTIME_ERROR, __VA_ARGS__);    \
                                                                  \
        trace_print_stack_trace(stderr, __current_trace);         \
        /* Free memory before aborting! */                        \
        trace_destruct(__current_trace);                          \
        /* This should be avoided in favour of FAIL */            \
        abort(); /* Dangerous way of handling errors */           \
    })


static thread_local jmp_buf finally_return_addr = {};

#define FINALIZER(name, impl)                                     \
    jmp_buf finalizer_##name = {};                                \
    if (setjmp(finalizer_##name) != 0) {                          \
        impl                                                      \
        longjmp(finally_return_addr, -1);                         \
    }

#define CALL_FINALIZER(finalizer)                                 \
    do {                                                          \
        if (setjmp(finally_return_addr) == 0)                     \
            longjmp(finalizer_##finalizer, -1);                   \
    } while(false)                                                \


#define FINALIZE_AND_FAIL(finalizer, ...)                         \
    CATCH({                                                       \
        CALL_FINALIZER(finalizer);                                \
        return PASS_FAILURE(__trace, RUNTIME_ERROR, __VA_ARGS__); \
    })


typedef int element_index_t;

template <typename E>
struct element {
    element_index_t next_index;
    element_index_t prev_index;
    bool is_free;

    E element;
};

template <typename E>
struct linked_list {
    element<E>* elements;
    size_t capacity, used;

    element_index_t free;
    bool is_linearized;
};


template <typename E>
inline element<E>* linked_list_next(linked_list<E>* list, element<E>* current) {
    return &list->elements[current->next_index];
}

template <typename E>
inline element<E>* linked_list_prev(linked_list<E>* list, element<E>* current) {
    return &list->elements[current->prev_index];
}

const element_index_t linked_list_end_index = 0;

template <typename E>
inline element<E>* linked_list_end(linked_list<E>* list) {
    return &list->elements[linked_list_end_index];
}


template <typename E>
inline element_index_t linked_list_head_index(linked_list<E>* list) {
    return linked_list_end(list)->next_index;
}

template <typename E>
inline element<E>* linked_list_head(linked_list<E>* list) {
    return &list->elements[linked_list_head_index(list)];
}


template <typename E>
inline element_index_t linked_list_tail_index(linked_list<E>* list) {
    return linked_list_end(list)->prev_index;
}

template <typename E>
inline element<E>* linked_list_tail(linked_list<E>* list) {
    return &list->elements[linked_list_tail_index(list)];
}


template <typename E>
inline element<E>* linked_list_get_pointer(linked_list<E>* list,
                                           element_index_t actual_index) {
    return &list->elements[actual_index];
}

template <typename E>
inline element_index_t linked_list_get_index(linked_list<E>* list,
                                             element<E>* element_ptr) {
    return element_ptr - list->elements;
}


template <typename E>
stack_trace* linked_list_create(linked_list<E>* list, const size_t capacity = 10) {
    element<E>* new_space = (element<E>*)
        calloc(capacity + 2 /* For two terminal nodes */, sizeof(*new_space));

    if (new_space == NULL)
        return FAILURE(RUNTIME_ERROR, strerror(errno));

    list->elements = new_space;
    list->capacity = capacity;

    list->is_linearized = true;

    // Memory is assumed to be zeroed after calloc
    linked_list_head(list)->is_free = false;

    // Loop first free element on itself
    list->free = 1;
    *linked_list_get_pointer(list, list->free) = 
        { .next_index = list->free,
          .prev_index = list->free,
          .is_free = true, .element = (E) {} };

    // Expand doubly linked list of free elements
    for (element_index_t i = (element_index_t) capacity + 1; i > list->free; -- i)
        __linked_list_insert_after_in_place(list, (E) {}, list->free, i);

    return SUCCESS();
}


template <typename E>
static inline
stack_trace* check_index(linked_list<E>* list, element_index_t index) {
    if (index > (element_index_t) list->capacity + 1)
        return FAILURE(RUNTIME_ERROR, "Index %d overflows list capacity %d!",
                       index, list->capacity);

    if (index < 0)
        return FAILURE(RUNTIME_ERROR, "Index %d underflows list capacity %d!",
                       index, list->capacity);

    return SUCCESS();
}


template <typename E>
stack_trace* linked_list_resize(linked_list<E>* list, const size_t new_capacity) {
    element<E>* new_space = (element<E>*)
        realloc(list->elements, sizeof(*new_space) *
                (new_capacity + 2) /* For terminal nodes */);

    if (new_space == NULL)
        return FAILURE(RUNTIME_ERROR, strerror(errno));

    list->elements = new_space;

    for (element_index_t i = list->capacity + 2; i <= (element_index_t) new_capacity + 1; ++ i)
        add_free_element(list, i);

    list->capacity = new_capacity;

    return SUCCESS();
}


template <typename E>
static inline
bool free_elements_left(linked_list<E>* list) {
    return list->free != list->elements[list->free].next_index;
}

template <typename E>
stack_trace* get_free_element_on_place(linked_list<E>* list,
                                       element_index_t place_index) {

    if (!is_free_element(list, place_index))
        return FAILURE(RUNTIME_ERROR, "Element %d isn't free!", place_index);

    const element_index_t next =
        list->elements[place_index].next_index;

    if (next == place_index)
        return FAILURE(RUNTIME_ERROR, "There's no free elements left!");

    TRY linked_list_unlink(list, place_index)
        FAIL("Failed to unlink element on place %d!", place_index);

    list->free = next; // This way we won't have any edge cases

    return SUCCESS();
}

template <typename E>
stack_trace* get_free_element(linked_list<E>* list, element_index_t* element_index) {
    *element_index = list->free;
    TRY get_free_element_on_place(list, list->free)
        FAIL("Can't detach list->free (%d) element!", list->free);
    return SUCCESS();
}

template <typename E>
void add_free_element(linked_list<E>* list, element_index_t element_index) {
    __linked_list_insert_after_in_place(list, (E) {}, list->free, element_index);
}

template <typename E>
static inline
bool is_free_element(linked_list<E>* list, element_index_t element_index) {
    return list->elements[element_index].is_free;
}


template <typename E>
static inline
void __linked_list_insert_after_in_place(linked_list<E>* list, E value,
                                         element_index_t prev_index,
                                         element_index_t place_for_new_element) {

    element<E>* prev_element = linked_list_get_pointer(list, prev_index);

    // Element that will go immediately after our new element
    element_index_t next_index = prev_element->next_index;

    //          next                        next          next
    // +------+ ~~~> +------+      +------x ~~~> /------x ~~~> /------+
    // | PREV | prev | NEXT |  =>  | PREV | prev | FREE | prev | NEXT |
    // +------+ <~~~ +------+      +------/ <~~~ x------/ <~~~ x------+

    // Update neighbours
    list->elements[prev_index].next_index = place_for_new_element;
    list->elements[next_index].prev_index = place_for_new_element;

    // Construct new element in the new place
    list->elements[place_for_new_element] = {
        .next_index = next_index,
        .prev_index = prev_index,
        .is_free = prev_element->is_free, .element = value
    };
}

template <typename E>
stack_trace* linked_list_insert_after(linked_list<E>* list, E value,
                                      element_index_t    prev_index,
                                      element_index_t* actual_index = NULL) {
    // Check if prev_index is valid index of list
    TRY check_index(list, prev_index) FAIL("Illegal index passed!");

    const double GROW = 2.0; // How much list grows when it runs out of space

    if (!free_elements_left(list))
        linked_list_resize(list, list->capacity * GROW);

    // Get free space for inserting new element
    element_index_t place_for_new_element = -1;
    if (prev_index <= list->capacity && is_free_element(list, prev_index + 1)) {
        place_for_new_element = prev_index + 1;

        TRY get_free_element_on_place(list, place_for_new_element)
            FAIL("Can't get desired free element!");
    } else {
        TRY get_free_element(list, &place_for_new_element)
            FAIL("Can't get free element!");

        list->is_linearized = false;
    }

    __linked_list_insert_after_in_place(list, value, prev_index,
                                        place_for_new_element);

    if (actual_index != NULL)
        *actual_index = place_for_new_element;

    ++ list->used; // This element was successfully added, let's update size

    return SUCCESS();
}

template <typename E>
inline stack_trace* linked_list_push_front(linked_list<E>* list, E element,
                                           element_index_t* actual_index = NULL) {

    // Inserting before head will result in pushing element to front
    return linked_list_insert_after(list, element, linked_list_end_index, actual_index);
}

template <typename E>
inline stack_trace* linked_list_push_back(linked_list<E>* list, E element,
                                          element_index_t* actual_index = NULL) {
    // Inserting after tail will result in pushing element to back
    return linked_list_insert_after(list, element,
                linked_list_tail_index(list), actual_index);
}


template <typename E>
stack_trace* linked_list_unlink(linked_list<E>* list, element_index_t actual_index) {
    // Check if element has is valid index in the list
    TRY check_index(list, actual_index) FAIL("Illegal index passed!");

    element<E>* current = &list->elements[actual_index];
    element_index_t prev_index = current->prev_index,
                    next_index = current->next_index;

    //          next          next                        next
    // +------x ~~~> /------x ~~~> /------+      +------+ ~~~> +------+
    // | PREV | prev | CURR | prev | NEXT |  =>  | PREV | prev | NEXT |
    // +------/ <~~~ x------/ <~~~ x------+      +------+ <~~~ +------+

    list->elements[prev_index].next_index = current->next_index;
    list->elements[next_index].prev_index = current->prev_index;

    return SUCCESS();
}

template <typename E>
stack_trace* linked_list_delete(linked_list<E>* list, element_index_t actual_index) {
    TRY check_index(list, actual_index) FAIL("Illegal index passed!");

    element<E>* current = linked_list_get_pointer(list, actual_index);
    const element_index_t head_ind = linked_list_head_index(list);

    if (current->next_index != head_ind && current->prev_index != head_ind)
        list->is_linearized = false;

    TRY linked_list_unlink(list, actual_index) FAIL("Cannot unlink element!");

    add_free_element(list, actual_index);

    -- list->used; // Element was successfully removed, let's correct size

    return SUCCESS();
}

template <typename E>
stack_trace* linked_list_pop_back(linked_list<E>* list) {
    return linked_list_delete(list, linked_list_head_index(list));
}

template <typename E>
stack_trace* linked_list_pop_front(linked_list<E>* list) {
    return linked_list_delete(list, linked_list_head_index(list));
}


template <typename E>
static inline void swap(E* first, E* second) {
    E temp = *first;
    *first = *second;
    *second = temp;
}

/**
 * Swap physical positons of elements prev and next
 * without changing their logical order in a list. 
 */
template <typename E>
stack_trace* linked_list_swap(linked_list<E>* list,
                              element_index_t fst_index,
                              element_index_t snd_index) {

    if (fst_index == snd_index)
        return SUCCESS();

    TRY check_index(list, fst_index) FAIL("Illegal second index!");

    TRY check_index(list, snd_index) FAIL("Illegal first index!");

    //           next   +-----+   prev
    //   (PREV1) ~~~>   | 1st |   <~~~ (NEXT1)
    //                  +-----+        
    //                 (fst_ind)

    //           next   +-----+   prev
    //   (PREV2) ~~~>   | 2nd |   <~~~ (NEXT2)
    //                  +-----+        
    //                 (snd_ind)

    element<E> *first    = linked_list_get_pointer(list, fst_index),
               *second   = linked_list_get_pointer(list, snd_index);

    element<E> *fst_prev = linked_list_prev(list,  first),
               *fst_next = linked_list_next(list,  first);

    element<E> *snd_prev = linked_list_prev(list, second),
               *snd_next = linked_list_next(list, second);

    // ==> Should become:
    //             next +-----+ prev
    // +~(PREV1)  +~~~> | 2nd | <~~~+  (NEXT1)~+
    // |          |     +-----+     |          |
    // |          |    (fst_ind)    |          |
    // |          |                 |          |
    // |          |     +-----+     |          |
    // | (PREV2)~~+     | 1st |     +~~(NEXT2) |
    // |          next  +-----+  prev          |
    // +~~~~~~~~~~~~~> (snd_ind) <~~~~~~~~~~~~~+

    fst_prev->next_index = fst_next->prev_index = snd_index;
    snd_prev->next_index = snd_next->prev_index = fst_index;

    swap(first, second); // We've prepared elements, now we can swap

    return SUCCESS();
}


#define LINKED_LIST_TRAVERSE(list, type, current)         \
    for (element<type>* current = linked_list_head(list); \
            current != linked_list_end (list);            \
            current  = linked_list_next(list, current))


template <typename E>
stack_trace* linked_list_linearize(linked_list<E>* list) {
    element_index_t logical_index = 1;
    for (element<E> *current =  linked_list_head(list);
            current != linked_list_end (list);
            current =  linked_list_next(list, current), ++ logical_index) {

        element_index_t actual_index = linked_list_get_index(list, current);

        TRY linked_list_swap(list, actual_index, logical_index)
            FAIL("Failed to exchange actual index with logical one!");

        current = linked_list_get_pointer(list, logical_index);
    }

    return SUCCESS();
}


template <typename E>
inline stack_trace* linked_list_get_logical_index(linked_list<E>*  const list,
                                                  const element_index_t logical_index,
                                                  element_index_t* const element_index) {
    if (list->is_linearized) {
        // Logical order starts from zero
        *element_index = linked_list_head_index(list) + logical_index;
        return SUCCESS();
    }

    element_index_t index = 0;
    LINKED_LIST_TRAVERSE(list, E, current)
        if (index ++ == logical_index) {
            *element_index = linked_list_get_index(list, current);
            return SUCCESS();
        }
}

template <typename E>
inline stack_trace* linked_list_get_logical(linked_list<E>* const list,
                                            const element_index_t logical_index,
                                            E* const value) {

    element_index_t actual_index = -1;
    TRY linked_list_get_logical_index(list, logical_index, &actual_index)
        FAIL("Can't get actual index of this element!");

    *value = list->elements[actual_index].element;

    return SUCCESS();
}


template <typename E>
void linked_list_destroy(linked_list<E> *list) {
    if (list != NULL) {
        free(list->elements);
        *list = {}; // Zero list out
    }

    // Do nothing if list is NULL (like free)
}

// ---------------------------------------------------------------------------------------------

template <typename E>
void print_text_dump(linked_list<E>* list) {
    printf("==> free: %d\n", list->free);

    printf("+-------------------------------------+\n");
    for (int i = 0; i <= list->capacity + 1; ++ i) {
        element<E>* elem = &list->elements[i];
        printf("| %2d: (%02d) | (<-) %02d | (->) %02d | %s |\n",
               i, elem->element, elem->prev_index,
               elem->next_index, elem->is_free ? "free" : "busy");
    }
    printf("+-------------------------------------+\n");
}

template <typename E>
void linked_list_create_graph(FILE* file, linked_list<E>* list) {
    fprintf(file, "digraph { \n");

    fprintf(file, "\t\t node_000 [label = \"cycle\", fontcolor=\"blue\", shape = rectangle, style = rounded];\n");

    fprintf(file, "\t subgraph { \n"
                  "\t\t rank = same; \n"
                  "\t\t node [shape=\"plaintext\"]; \n");

    for (int i = 1; i <= (int) list->capacity + 1; ++ i) {
        const element<E>* el = &list->elements[i];
        fprintf(file, "\t\t "
                R"(node_%03d [label = <<table border="0" cellborder="1" cellspacing="0">
                       <tr> <td port="index" colspan="2"> %d </td> </tr>
                       <tr> <td> elem </td> <td port="elem"> %d </td> </tr>
                       <tr> <td> prev </td> <td port="prev"> %d </td> </tr>
                       <tr> <td> next </td> <td port="next"> %d </td> </tr>
                   </table>>];)" "\n", i, i, el->element, el->prev_index, el->next_index);
    }

    fprintf(file, "\t\t edge [constraint = true, style = \"invis\"]; \n");
    for (int i = 1; i < (int) list->capacity; ++ i)
        fprintf(file, "\t\t node_%03d -> node_%03d;\n", i, i + 1);

    fprintf(file, "\t\t edge [constraint = false, style = \"solid\"]; \n");
    for (int i = 1; i <= (int) list->capacity + 1; ++ i) {
        const element<E>* el = &list->elements[i];

        if (el->next_index != -1 && el->next_index != 0)
            fprintf(file, "\t\t node_%03d:next -> node_%03d; \n", i, el->next_index);

        if (el->prev_index != -1 && el->prev_index != 0)
            fprintf(file, "\t\t node_%03d:prev -> node_%03d; \n", i, el->prev_index);
    }

    fprintf(file, "\t } \n");

    for (int i = list->elements[0].next_index != 0? 0 : 1; i <= (int) list->capacity + 1; ++ i) {
        const element<E>* el = &list->elements[i];

        if (el->next_index == 0)
            fprintf(file, "\t\t node_%03d:next -> node_%03d; \n", i, el->next_index);

        if (el->prev_index == 0)
            fprintf(file, "\t\t node_%03d:prev -> node_%03d; \n", i, el->prev_index);
    }

    fprintf(file, "\t\t node [shape=\"rectangle\", style=\"rounded\"]; \n"
                  "\t\t free [label = \"free\", fontcolor = \"seagreen\"];"
                  "\t\t head [label = \"head\", fontcolor = \"crimson\"]; \n"
                  "\t\t tail [label = \"tail\", fontcolor = \"darkmagenta\"]; \n");

    if (list->free != 0)
        fprintf(file, "free -> node_%03d;", list->free);

    #define head list->elements[0].next_index
    if (head != 0)
        fprintf(file, "head -> node_%03d;", head);

    #define tail list->elements[0].prev_index
    if (tail != 0)
        fprintf(file, "tail -> node_%03d;", tail);

    #undef head
    #undef tail

    fprintf(file, "} \n");
}

const size_t MAX_TMP_NAME_SIZE = 128;

template <typename E>
inline char* linked_list_call_graphviz(linked_list<E>* list) {
    char* graph_tmp_name = tmpnam(NULL);

    FILE* tmp = fopen(graph_tmp_name, "w");
    linked_list_create_graph(tmp, list);

    char dot_buffer[256] = {};
    strcat(dot_buffer, "dot -Tpng ");
    strcat(dot_buffer, graph_tmp_name);

    fclose(tmp), tmp = NULL;


    char* image_tmp_name = (char*)
        calloc(MAX_TMP_NAME_SIZE, sizeof(char));
    strcat(image_tmp_name, ".png");

    tmpnam(image_tmp_name);
    strcat(dot_buffer, " -o ");
    strcat(dot_buffer, image_tmp_name);

    system(dot_buffer);

    return image_tmp_name;
}


static uint32_t  int_hash(const int   number);


template <typename K, typename V>
struct hash_table_pair {
    K key;
    V value;
};

template <typename K, typename V>
hash_table_pair<K, V> hash_table_pair_create(K key, V value) {
    return { key, value };
}

struct hash_table_bucket {
    element_index_t value_index;
    size_t size;
};

template <typename K, typename V>
struct hash_table {
    uint32_t (*key_hash_function) (K key);
    bool (*key_equals_function) (K* first, K* second);

    hash_table_bucket* hash_table;
    linked_list<hash_table_pair<K, V>> values;

    size_t buckets_used, buckets_capacity;
};

template <typename K>
bool hash_table_simple_key_equality(K* key_first, K* key_second) {
    return *key_first == *key_second;
}

template <typename K, typename V>
stack_trace* hash_table_create(hash_table<K, V>* table,
                               uint32_t (*key_hash_function) (K key),
                               size_t bucket_capacity = 32,
                               size_t value_list_size = 10, 
                               bool (*key_equals_function) (K* first, K* second) =
                                    hash_table_simple_key_equality<K>) {

    // Bucket capacity should be power of two
    bucket_capacity = (size_t) pow(2, (int) ceil(log2(bucket_capacity)));

    *table = {
        // Comparator function
        .key_hash_function = key_hash_function,

        // Function that checks for equality
        .key_equals_function = key_equals_function,

        // Initialize hash table array and linked
        // list of values with zeroes.

        // They will be initialized properly soon
        .hash_table = NULL, .values = {},

        // Number of buckets in use, useful for
        // calculating load factor of hash table
        .buckets_used = 0,
        
        // Number of buckets available for elements,
        // this can change when table gets resized
        .buckets_capacity = bucket_capacity
    };

    TRY linked_list_create(&table->values, value_list_size)
        FAIL("Linked list initialization of size %d failed!", value_list_size);

    FINALIZER(list_destroy, { linked_list_destroy(&table->values); });

    TRY safe_calloc(bucket_capacity, &table->hash_table)
        FINALIZE_AND_FAIL(list_destroy, "Hash table allocation failed!");

    // TODO: Should I rely on calloc for zeroing memory?

    return SUCCESS();
}

template<typename K, typename V>
size_t __hash_table_get_position(hash_table<K, V>* table, K key) {
    uint32_t key_hash = table->key_hash_function(key);

    // We can use fast modulo since /bucket_capacity/ is power of 2
    return key_hash & (table->buckets_capacity - 1);
}

template<typename K, typename V>
inline static
hash_table_bucket* __hash_table_lookup_bucket(hash_table<K, V>* table, K key) {
    size_t position = __hash_table_get_position(table, key);
    return &table->hash_table[position];
}

template<typename K, typename V>
inline static
element_index_t __hash_table_lookup_index(hash_table<K, V>* table, K key,
                                          hash_table_bucket** key_bucket = NULL) {

    hash_table_bucket* bucket = __hash_table_lookup_bucket(table, key);

    // Return bucket number, to avoid hashing key second time
    if (key_bucket != NULL)
        *key_bucket = bucket;

    if (bucket->size != 0) {
        element<hash_table_pair<K, V>> *current =
            linked_list_get_pointer(&table->values, bucket->value_index);

        for (size_t index = 0; index < bucket->size; ++ index) {
            if (table->key_equals_function(&current->element.key, &key))
                return linked_list_get_index(&table->values, current);

            current = linked_list_next(&table->values, current);
        }
    }

    return linked_list_end_index;
}

template<typename K, typename V>
V* hash_table_lookup(hash_table<K, V>* table, K key) {
    element_index_t index = __hash_table_lookup_index(table, key);
    if (index == linked_list_end_index)
        return NULL; // Element not found

    return &linked_list_get_pointer(&table->values, index)->element.value;
}


#define HASH_TABLE_PAIR_T(key_type, value_type) hash_table_pair<key_type, value_type>

#define HASH_TABLE_TRAVERSE(table, key_type, value_type, current)                            \
    LINKED_LIST_TRAVERSE(&(table)->values, HASH_TABLE_PAIR_T(key_type, value_type), current)

#define KEY(  current) ((current)->element.key)
#define VALUE(current) ((current)->element.value) 

template <typename K, typename V>
void hash_table_rehash(hash_table<K, V>* table,
                       const size_t new_bucket_capacity,
                       const size_t new_values_capacity) {

    hash_table<K, V> new_table;
    hash_table_create(&new_table, table->key_hash_function,
                      new_bucket_capacity,
                      new_values_capacity,
                      table->key_equals_function);

    HASH_TABLE_TRAVERSE(table, K, V, current)
        hash_table_insert(&new_table, KEY(current), VALUE(current));

    hash_table_destroy(table);
    *table = new_table; // Replace hash_table with a new one
}

template <typename K, typename V>
void hash_table_rehash_keep_size(hash_table<K, V>* table) {
    hash_table_rehash(table, table->buckets_capacity, table->values.capacity);
}

template <typename K, typename V>
bool hash_table_delete(hash_table<K, V>* table, K key) {
    hash_table_bucket* bucket = NULL;
    element_index_t index =
        __hash_table_lookup_index(table, key, &bucket);

    if (index == linked_list_end_index)
        return false;

    TRY linked_list_delete(&table->values, index)
        THROW("Value deletion failed!");

    -- bucket->size; // Since we found element
    // Bucket should be bigger than 1 in any case

    return true; // Deletion succeeded
}

template <typename K, typename V>
bool hash_table_contains(hash_table<K, V>* table, K key) {
    return __hash_table_lookup_index(table, key) != linked_list_end_index;
}

template <typename K, typename V>
bool hash_table_insert(hash_table<K, V>* table, K key, V value) {
    hash_table_bucket* bucket;
    if (__hash_table_lookup_index(table, key, &bucket) != linked_list_end_index)
        return false; // There's same key in the hash table 

    if (bucket->size > 0)
        TRY linked_list_insert_after(&table->values, { key, value },  bucket->value_index)
            THROW("Failed to insert new value in existing bucket!");
    else {
        ++ table->buckets_used;
        TRY linked_list_push_back(   &table->values, { key, value }, &bucket->value_index)
            THROW("Failed to insert new value in a new bucket (size: %d)!", bucket->size);
    }

    ++ bucket->size;

    const double GROW = 2.0;
    const double MAX_LOAD_FACTOR = 0.5;

    if ((double) table->buckets_used /
        (double) table->buckets_capacity >= MAX_LOAD_FACTOR)
        hash_table_rehash(table, table->buckets_capacity * GROW,
                                 table-> values.capacity * GROW);

    return true; // Inserted successfully
}

template <typename K, typename V>
void hash_table_destroy(hash_table<K, V>* table) {
    linked_list_destroy(&table->values);
    free(table->hash_table), table->hash_table = NULL;
}

template <typename K, typename V>
hash_table<K, V> create_hash_table(uint32_t (*key_hash_function)(K), int pair_count, ...) {
    hash_table<K, V> table;

    // This should be bigger than /pair_count/ to reduce hash clashes 
    const double target_fill_percent = 0.5;

    const size_t bucket_capacity = (size_t) (pair_count / target_fill_percent);

    TRY hash_table_create(&table, key_hash_function, bucket_capacity, pair_count)
    // This function is meant for inline initialization, we can't return trace :(
        THROW("Hash table creation failed!");

    // So we have no other way, except printing user error message, and aborting

    // Populate hash table with our values
    va_list args;
    va_start(args, pair_count);

    for (int i = 0; i < pair_count; ++ i) {
        // We need this hack to pass , in macro argument because /va_arg/
        // is a macro and <K, V> can't be surrounded with round braces ()
        #define _ ,
        hash_table_pair<K, V> pair = va_arg(args, hash_table_pair<K _ V>);
        #undef  _

        hash_table_insert(&table, pair.key, pair.value);
    }

    va_end(args);

    return table; // Table's struct will be copied, but it's small so it's ok
}

#define PAIR(key, value) hash_table_pair_create(key, value)
#define HASH_TABLE(key_type, value_type, key_hash_function, ...)                             \
    create_hash_table<key_type, value_type>(key_hash_function,                               \
                                            MACRO_UTILS_NARG(__VA_ARGS__), __VA_ARGS__)
#define MACRO_UTILS_NARG( ...) MACRO_UTILS_NARG_(__VA_ARGS__, MACRO_UTILS_RSEQ_N())

#define MACRO_UTILS_NARG_(...) MACRO_UTILS_128TH_ARG(__VA_ARGS__)

#define MACRO_UTILS_128TH_ARG(                                       \
         _001, _002, _003, _004, _005, _006, _007, _008, _009, _010, \
         _011, _012, _013, _014, _015, _016, _017, _018, _019, _020, \
         _021, _022, _023, _024, _025, _026, _027, _028, _029, _030, \
         _031, _032, _033, _034, _035, _036, _037, _038, _039, _040, \
         _041, _042, _043, _044, _045, _046, _047, _048, _049, _050, \
         _051, _052, _053, _054, _055, _056, _057, _058, _059, _060, \
         _061, _062, _063, _064, _065, _066, _067, _068, _069, _070, \
         _071, _072, _073, _074, _075, _076, _077, _078, _079, _080, \
         _081, _082, _083, _084, _085, _086, _087, _088, _089, _090, \
         _091, _092, _093, _094, _095, _096, _097, _098, _099, _100, \
         _101, _102, _103, _104, _105, _106, _107, _108, _109, _110, \
         _111, _112, _113, _114, _115, _116, _117, _118, _119, _120, \
         _121, _122, _123, _124, _125, _126, _127, N, ...) N

#define MACRO_UTILS_RSEQ_N()                                         \
                      127,  126,  125,  124,  123,  122,  121,  120, \
          119,  118,  117,  116,  115,  114,  113,  112,  111,  110, \
          109,  108,  107,  106,  105,  104,  103,  102,  101,  100, \
           99,   98,   97,   96,   95,   94,   93,   92,   91,   90, \
           89,   88,   87,   86,   85,   84,   83,   82,   81,   80, \
           79,   78,   77,   76,   75,   74,   73,   72,   71,   70, \
           69,   68,   67,   66,   65,   64,   63,   62,   61,   60, \
           59,   58,   57,   56,   55,   54,   53,   52,   51,   50, \
           49,   48,   47,   46,   45,   44,   43,   42,   41,   40, \
           39,   38,   37,   36,   35,   34,   33,   32,   31,   30, \
           29,   28,   27,   26,   25,   24,   23,   22,   21,   20, \
           19,   18,   17,   16,   15,   14,   13,   12,   11,   10, \
            9,    8,    7,    6,    5,    4,    3,    2,    1,    0


template <typename E>
stack_trace* safe_calloc(size_t number_of_members, E** allocated_space) {
    E* new_space = (E*) calloc(number_of_members, sizeof(E));

    if (new_space == NULL)
        return FAILURE(RUNTIME_ERROR, "Calloc failed due to %s!"
                       "\n\t" "    number of members: %d"
                       "\n\t" "          member size: %d",
                       "\n\t" "total requested bytes: %d",
                       strerror(errno),
                       number_of_members,  sizeof(E),
                       number_of_members * sizeof(E));

    *allocated_space = new_space; // Successfully allocated
    return SUCCESS();
}

template <typename E>
stack_trace* safe_realloc(E** old_space, size_t number_of_members) {
    E* new_space = (E*) realloc(old_space, number_of_members * sizeof(E));
    if (new_space == NULL)
        return FAILURE(RUNTIME_ERROR, "Realloc failed due to %s!"
                       "\n\t"  "  reallocated pointer: %p" "\n",
                       "\n\t"  "    number of members: %d"
                       "\n\t"  "          member size: %d",
                       "\n\t"  "total requested bytes: %d",
                       strerror(errno), old_space,
                       number_of_members,  sizeof(E),
                       number_of_members * sizeof(E));

    // TODO: Add option to zero out memory 

    *old_space = new_space; // Successfully allocated
    return SUCCESS();
}

template <typename E>
void safe_free(E** link_to_free) {
    free(*link_to_free), *link_to_free = NULL;
}


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


/** Different node placements inside of a subgraph */
enum graphviz_rank_type {
    RANK_SAME,   RANK_MIN,  RANK_MAX,
    RANK_SOURCE, RANK_SINK, RANK_NONE
};

/** Colors that can be applied to nodes and edges  */
enum graphviz_color {
    GRAPHVIZ_RED,   GRAPHVIZ_BLUE,   GRAPHVIZ_GREEN,
    GRAPHVIZ_BLACK, GRAPHVIZ_YELLOW, GRAPHVIZ_ORANGE
};

/** Styles that can be applied to nodes and edges */
enum graphviz_style {
    STYLE_FILLED,    STYLE_ROUNDED, STYLE_DASHED,
    STYLE_DIAGONALS, STYLE_INVIS,   STYLE_BOLD,
    STYLE_DOTTED,    STYLE_SOLID
};

/** Various node shapes */
enum graphviz_node_shape {
    SHAPE_BOX,           SHAPE_POLYGON,       SHAPE_ELLIPSE,
    SHAPE_OVAL,          SHAPE_CIRCLE,        SHAPE_POINT,
    SHAPE_DOUBLECIRCLE,  SHAPE_DOUBLEOCTAGON, SHAPE_TRIPLEOCTAGON,
    SHAPE_INVTRIANGLE,   SHAPE_INVTRAPEZIUM,  SHAPE_INVHOUSE,
    SHAPE_EGG,           SHAPE_TRIANGLE,      SHAPE_PLAINTEXT,
    SHAPE_PLAIN,         SHAPE_DIAMOND,       SHAPE_TRAPEZIUM,
    SHAPE_PARALLELOGRAM, SHAPE_HOUSE,         SHAPE_PENTAGON,
    SHAPE_HEXAGON,       SHAPE_SEPTAGON,      SHAPE_OCTAGON
};

static hash_table<int, const char*> graphviz_rank_names =
    HASH_TABLE(int, const char*, int_hash,
               PAIR(RANK_SAME           , "same"        ),
               PAIR(RANK_MAX            , "max"         ),
               PAIR(RANK_MIN            , "min"         ),
               PAIR(RANK_SOURCE         , "source"      ),
               PAIR(RANK_SINK           , "sink"        ));


static hash_table<int, const char*> graphviz_colors =
    HASH_TABLE(int, const char*, int_hash,
               PAIR(GRAPHVIZ_RED        , "red"         ),
               PAIR(GRAPHVIZ_YELLOW     , "yellow"      ),
               PAIR(GRAPHVIZ_GREEN      , "green"       ),
               PAIR(GRAPHVIZ_BLUE       , "blue"        ),
               PAIR(GRAPHVIZ_BLACK      , "black"       ),
               PAIR(GRAPHVIZ_ORANGE     , "orange"      ));


static hash_table<int, const char*> graphviz_styles =
    HASH_TABLE(int, const char*, int_hash,
               PAIR(STYLE_FILLED        , "filled"      ),
               PAIR(STYLE_ROUNDED       , "rounded"     ),
               PAIR(STYLE_DASHED        , "dashed"      ),
               PAIR(STYLE_DIAGONALS     , "diagonals"   ),
               PAIR(STYLE_INVIS         , "invis"       ),
               PAIR(STYLE_BOLD          , "bold"        ),
               PAIR(STYLE_DOTTED        , "dotted"      ),
               PAIR(STYLE_SOLID         , "solid"       ));


static hash_table<int, const char*> graphviz_node_shapes =
    HASH_TABLE(int, const char*, int_hash,
               PAIR(SHAPE_BOX          , "box"          ),
               PAIR(SHAPE_POLYGON      , "polygon"      ),
               PAIR(SHAPE_ELLIPSE      , "ellipse"      ),
               PAIR(SHAPE_OVAL         , "oval"         ),
               PAIR(SHAPE_CIRCLE       , "circle"       ),
               PAIR(SHAPE_POINT        , "point"        ),
               PAIR(SHAPE_EGG          , "egg"          ),
               PAIR(SHAPE_TRIANGLE     , "triangle"     ),
               PAIR(SHAPE_PLAINTEXT    , "plaintext"    ),
               PAIR(SHAPE_PLAIN        , "plain"        ),
               PAIR(SHAPE_DIAMOND      , "diamond"      ),
               PAIR(SHAPE_TRAPEZIUM    , "trapezium"    ),
               PAIR(SHAPE_PARALLELOGRAM, "parallelogram"),
               PAIR(SHAPE_HOUSE        , "house"        ),
               PAIR(SHAPE_PENTAGON     , "pentagon"     ),
               PAIR(SHAPE_HEXAGON      , "hexagon"      ),
               PAIR(SHAPE_SEPTAGON     , "septagon"     ),
               PAIR(SHAPE_OCTAGON      , "octagon"      ),
               PAIR(SHAPE_DOUBLECIRCLE , "doublecircle" ),
               PAIR(SHAPE_DOUBLEOCTAGON, "doubleoctagon"),
               PAIR(SHAPE_TRIPLEOCTAGON, "tripleoctagon"),
               PAIR(SHAPE_INVTRIANGLE  , "invtriangle"  ),
               PAIR(SHAPE_INVTRAPEZIUM , "invtrapezium" ),
               PAIR(SHAPE_INVHOUSE     , "invhouse"     ));


struct node {
    graphviz_style style;
    graphviz_color color;
    graphviz_node_shape shape;

    char* label;
};

typedef element_index_t node_id;

struct edge {
    node_id from, to;

    graphviz_color color;
    graphviz_style style;

    char* label;
};

struct subgraph {
    linked_list<node> nodes;
    linked_list<edge> edges;

    graphviz_rank_type rank;
};

struct digraph {
    linked_list<subgraph> subgraphs;
};


inline digraph digraph_create();

typedef element_index_t subgraph_id;

/**
 * Create new subgraph and immediately insert it in a graph
 *
 * @return id of newly created subgraph
 */
inline subgraph_id digraph_create_subgraph(digraph* graph, graphviz_rank_type rank);


/**
 * Get subgraph by it's id in graph, this mostly used in graphviz's
 * functions and traversal macros
 * 
 * @return Pointer to subgraph identified by id
 *
 * @note Pointer to a subgraph shouldn't be used as a subgraph
 * identifier, because list of subgraphs in graph doesn't have
 * reference stability, buffer may be reallocated and subgraph moved
 */
inline subgraph* digraph_get_subgraph(digraph* graph, subgraph_id subgraph);

inline element_index_t subgraph_insert_node(digraph* graph, subgraph_id subgraph, node new_node);
inline void  subgraph_insert_edge(digraph* graph, subgraph_id subgraph, edge new_edge);

inline node node_from_default(node default_node, const char *format, ...);
inline edge edge_from_default(edge default_edge, node_id from, node_id to,
                              const char *format, ...);

/**
 * Insert new node with style inherited from default node
 *
 * @return value that identifies node and can be used to create edges
 */
inline node_id subgraph_insert_default_node(digraph* graph, subgraph_id subgraph,
                                            node default_node, const char* format, ...);

/**
 * Insert new edge that connects two nodes, identified by their id's
 */
inline void    subgraph_insert_default_edge(digraph* graph, subgraph_id subgraph, edge default_edge,
                                            node_id from, node_id to, const char* format, ...);

/**
 * Write to @arg file description of the @arg graph in graphviz dot's lang
 */
inline void  digraph_write_to_file(FILE* file,  digraph* graph);


inline char* digraph_render(digraph* graph);

inline void digraph_destroy(digraph* graph);

inline void digraph_render_and_destory(digraph* graph);


#define NEW_GRAPH(...) ({                                                               \
        digraph __current_graph = digraph_create();                                     \
        __VA_ARGS__                                                                     \
        __current_graph;                                                                \
    })

#define NEW_SUBGRAPH(rank, ...)                                                         \
    do {                                                                                \
        subgraph_id __current_subgraph =                                                \
            digraph_create_subgraph(&__current_graph, rank);                            \
                                                                                        \
        node __default_node = {};                                                       \
        edge __default_edge = {};                                                       \
        __VA_ARGS__                                                                     \
    } while(false)

#define DEFAULT_NODE                                                                    \
    __default_node

#define DEFAULT_EDGE                                                                    \
    __default_edge

#define NODE(...)                                                                       \
    subgraph_insert_default_node(&__current_graph, __current_subgraph,                  \
                                 __default_node, __VA_ARGS__)

#define EDGE(from, to)                                                                  \
    subgraph_insert_default_edge(&__current_graph, __current_subgraph,                  \
                                 __default_edge, from, to, "")

#define LABELED_EDGE(from, to, ...)                                                     \
    subgraph_insert_default_edge(&__current_graph, __current_subgraph,                  \
                                 __default_edge, from, to, __VA_ARGS__)

#define SUBGRAPH_CONTEXT                                                                \
    digraph __current_graph,                                                            \
    subgraph_id __current_subgraph,                                                     \
    node __default_node,                                                                \
    edge __default_edge

#define CURRENT_SUBGRAPH_CONTEXT                                                        \
    __current_graph, __current_subgraph, __default_node, __default_edge

#define TRAVERSE_NODES(current)                                                         \
    LINKED_LIST_TRAVERSE(&digraph_get_subgraph(&__current_graph,                        \
                                               __current_subgraph)->nodes,              \
                         node, current)

#define NODE_ID(num_defined) ({                                                         \
        node_id __defined_node_id = linked_list_end_index;                              \
        TRY linked_list_get_logical_index(&digraph_get_subgraph(&__current_graph,       \
                                                                __current_subgraph)     \
                                          ->nodes, num_defined, &__defined_node_id)     \
            THROW("Failed to determine element by it's logical index!");                \
        __defined_node_id;                                                              \
    })


inline digraph digraph_create() {
    digraph graph = {};

    const size_t default_subgraph_count = 3;
    TRY linked_list_create(&graph.subgraphs, default_subgraph_count)
        THROW("Linked list creation failed!");

    return graph;
}


inline subgraph* digraph_get_subgraph(digraph* graph, subgraph_id subgraph) {
    return &linked_list_get_pointer(&graph->subgraphs, subgraph)->element;
}


inline subgraph_id digraph_create_subgraph(digraph* graph, graphviz_rank_type rank) {
    subgraph new_subgraph = {};
    new_subgraph.rank = rank;

    // Create new subgraph, initialize it's edge and node lists

    const size_t default_edge_list_capacity = 10;
    TRY linked_list_create(&new_subgraph.edges, default_edge_list_capacity)
        THROW("Failed to create list of edges in a new subgraph!");

    const size_t default_node_list_capacity = 10;
    TRY linked_list_create(&new_subgraph.nodes, default_node_list_capacity)
        THROW("Failed to create list of nodes in a new subgraph!");

    // Insert new subgraph
    TRY linked_list_push_back(&graph->subgraphs, new_subgraph)
        THROW("Failed to add new subgraph to the list!");

    // We inserted to the end of the list, let's get that element:
    return linked_list_tail_index(&graph->subgraphs);
}


inline node_id subgraph_insert_node(digraph* graph, subgraph_id subgraph_pos, node new_node) {
    subgraph* current_subgraph = digraph_get_subgraph(graph, subgraph_pos);

    TRY linked_list_push_back(&current_subgraph->nodes, new_node)
        THROW("Failed to insert new node!");

    // We inserted to the end of the list, let's get that element:
    return linked_list_tail_index(&current_subgraph->nodes);
}

inline node vnode_from_default(node default_node, const char* format, va_list args) {
    // Default node is copied
    default_node.label = vsprintf_to_new_buffer(format, args);

    return default_node;
}

inline node  node_from_default(node default_node, const char* format, ...) {
    va_list args;
    va_start(args, format);

    node output_node = vnode_from_default(default_node, format, args);

    va_end(args);

    return output_node;
}

inline node_id subgraph_insert_default_node(digraph* graph, subgraph_id subgraph_pos,
                                            node default_node, const char* format, ...) {
    va_list args;
    va_start(args, format);

    node_id output_node = subgraph_insert_node(graph, subgraph_pos,
        vnode_from_default(default_node, format, args));

    va_end(args);

    return output_node;
}


inline void subgraph_insert_edge(digraph* graph, subgraph_id subgraph_pos, edge new_edge) {
    subgraph* current_subgraph = digraph_get_subgraph(graph, subgraph_pos);

    TRY linked_list_push_back(&current_subgraph->edges, new_edge)
        THROW("Failed to insert new edge!");
}


inline edge vedge_from_default(edge default_edge, node_id from, node_id to,
                               const char* format, va_list args) {

    // Default edge is copied
    default_edge.from = from;
    default_edge.to   = to;

    if (format != NULL)
        default_edge.label = vsprintf_to_new_buffer(format, args);

    return default_edge;
}

inline edge edge_from_default(edge default_edge, node_id from, node_id to,
                              const char* format, ...) {


    va_list args;
    va_start(args, format);

    edge new_edge = vedge_from_default(default_edge, from, to, format, args);

    va_end(args);

    return new_edge;
}

inline void subgraph_insert_default_edge(digraph* graph, subgraph_id subgraph_pos, edge default_edge,
                                         node_id node_from, node_id node_to, const char* format, ...) {

    va_list args;
    va_start(args, format);

    edge edge_to_insert = vedge_from_default(default_edge, node_from,
                                             node_to, format, args);

    va_end(args);

    subgraph_insert_edge(graph, subgraph_pos, edge_to_insert);
}


inline void subgraph_write_to_file(FILE* file, subgraph* graph) {
    fprintf(file, "\t" "subgraph {" "\n");

    const char** rank =
        hash_table_lookup(&graphviz_rank_names, (int) graph->rank);

    if (rank != NULL)
        fprintf(file, "\t\t" "rank = %s;" "\n", *rank);

    LINKED_LIST_TRAVERSE(&graph->nodes, node, current) {
        node* current_node = &current->element;

        const char* shape =
            *hash_table_lookup(&graphviz_node_shapes, (int) current_node->shape);

        const char* color =
            *hash_table_lookup(&graphviz_colors,      (int) current_node->color);

        const char* style =
            *hash_table_lookup(&graphviz_styles,      (int) current_node->style);

        node_id node_identity = linked_list_get_index(&graph->nodes, current);

        fprintf(file, "\t\t" "node_%d [" "label = \"%s\","
                "shape = \"%s\", color = \"%s\", style = \"%s\"];" "\n",
                node_identity, current_node->label, shape, color, style);
    }

    LINKED_LIST_TRAVERSE(&graph->edges, edge, current) {
        edge* current_edge = &current->element;

        node_id from_node_id = current_edge->from, to_node_id = current_edge->to;

        const char* color =
            *hash_table_lookup(&graphviz_colors, (int) current_edge->color);

        const char* style =
            *hash_table_lookup(&graphviz_styles, (int) current_edge->style);

        fprintf(file, "\t\t" "node_%d -> node_%d [label = \" %s \","
                "color = %s, style = %s, margin = \"1.5\"];" "\n",
                from_node_id, to_node_id, current_edge->label, color, style);

    }

    fprintf(file, "\t" "}"          "\n");
}

inline void digraph_write_to_file(FILE* file, digraph* graph) {
    fprintf(file, "digraph {" "\n");

    LINKED_LIST_TRAVERSE(&graph->subgraphs, subgraph, current)
        subgraph_write_to_file(file, &current->element);

    fprintf(file, "}" "\n");
}


inline void digraph_destroy(digraph *graph) {
    LINKED_LIST_TRAVERSE(&graph->subgraphs, subgraph, current) {
        LINKED_LIST_TRAVERSE(&current->element.nodes, node, current_node)
            free(current_node->element.label);

        linked_list_destroy(&current->element.nodes);

        LINKED_LIST_TRAVERSE(&current->element.edges, edge, current_edge)
            free(current_edge->element.label);

        linked_list_destroy(&current->element.edges);
    }

    linked_list_destroy(&graph->subgraphs),
        graph->subgraphs = {};
};


inline char* digraph_render(digraph* graph) {
    char* graph_tmp_name = tmpnam(NULL);

    FILE* tmp = fopen(graph_tmp_name, "w");
    digraph_write_to_file(tmp, graph);

    char dot_buffer[256] = {};
    strcat(dot_buffer, "dot -Tpng ");
    strcat(dot_buffer, graph_tmp_name);

    fclose(tmp), tmp = NULL;

    char* image_tmp_name = (char*)
        calloc(MAX_TMP_NAME_SIZE, sizeof(char));
    strcat(image_tmp_name, ".png");

    tmpnam(image_tmp_name);
    strcat(dot_buffer, " -o ");
    strcat(dot_buffer, image_tmp_name);

    system(dot_buffer);

    return image_tmp_name;
}

inline void digraph_render_and_destory(digraph* graph) {
    char* name = digraph_render(graph);
    char buffer[256] = {};

    strcat(buffer, "sxiv ");
    strcat(buffer, name);

    system(buffer);

    free(name), name = NULL;
    digraph_destroy(graph);
}



static uint32_t int_hash(const int number) {
    uint32_t hash = (uint32_t) number;

    // Magic number has been calculated with a test
    // that calculated the avalanche effect
    const uint32_t magic_number = 0x45D9F3B;

    const uint32_t BITS_IN_BYTE = 8UL;
    const uint32_t  bits_in_int =
        sizeof(int) * BITS_IN_BYTE / 2;

    hash = ((hash >> bits_in_int) ^ hash) * magic_number;
    hash = ((hash >> bits_in_int) ^ hash) * magic_number;
    hash =  (hash >> bits_in_int) ^ hash;
    return hash;
}




static char* vsprintf_to_new_buffer(const char* format, va_list args) {
    // This allows to then restore changes to diagnostic rules
    #pragma clang diagnostic push

    // This function itself is intended for use with string
    // literals, so it should be ok to disable warning:

    #pragma clang diagnostic ignored "-Wformat-nonliteral"

    // We need to copy args, because we will use them twice
    va_list args_copy;
    va_copy(args_copy, args);

    int buffer_size = vsnprintf(NULL, 0, format, args_copy) + 1;

    va_end(args_copy); // We are done with this args

    char *buffer = (char *) calloc((size_t) buffer_size, sizeof(*buffer));

    // Now let's finally print our string!
    vsnprintf(buffer, (size_t) buffer_size, format, args);

    // And restore warning back
    #pragma clang diagnostic pop

    return buffer;
}


#define COLOR_RED     "\033[31m"
#define COLOR_GREEN   "\033[32m"
#define COLOR_YELLOW  "\033[33m"
#define COLOR_BLUE    "\033[35m"

#define COLOR_BOLD    "\033[1m"
#define COLOR_RESET   "\033[0m"

#define COLOR_INFO    COLOR_BOLD COLOR_BLUE
#define COLOR_ERROR   COLOR_BOLD COLOR_RED
#define COLOR_WARNING COLOR_BOLD COLOR_YELLOW
#define COLOR_SUCCESS COLOR_BOLD COLOR_GREEN

#define TEXT_INFO(   str)    COLOR_INFO str COLOR_RESET
#define TEXT_WARNING(str) COLOR_WARNING str COLOR_RESET
#define TEXT_ERROR(  str)   COLOR_ERROR str COLOR_RESET
#define TEXT_SUCCESS(str) COLOR_SUCCESS str COLOR_RESET

#define TAB "    "


static stack_trace* __trace_create_success() {
    // We only ever need single instance of
    // success, because success has no cause
    static stack_trace success_trace = {
        .latest_error = { .error_code = SUCCESS },
        .trace = NULL
    };

    return &success_trace;
}

static int trace_error_code(stack_trace* trace) {
    return trace->latest_error.error_code;
}

static bool trace_is_success(stack_trace* trace) {
    return trace_error_code(trace) == SUCCESS;
}


static stack_trace __trace_stack_trace_reserved_space_in_case_calloc_fails;
static char __trace_error_message_reserved_space_in_case_calloc_fails[256];

static stack_trace* __trace_create_failure(stack_trace* cause, int code, occurance occured,
                                           const char* format, ...) {

    if (cause != NULL && trace_is_success(cause))
        return FAILURE(LOGIC_ERROR, "Error can't be caused by success!");

    stack_trace* new_trace = (stack_trace*) calloc(1, sizeof(*new_trace));

    if (new_trace == NULL) {
        fprintf(stderr, "Lost and unwrapped error: %s\n", format);
        format = "Trace allocation failed, actual error was lost";

        // We could be unable to notify user about actual error with
        // trace mechanism, that's a big problem, let's show message:
        perror(format);

        // calloc failed, but we still need a way to notify user
        // about error, so we will use static space for that:
        new_trace = &__trace_stack_trace_reserved_space_in_case_calloc_fails;
    }

    va_list  vprintf_args;
    va_start(vprintf_args, format);

    #pragma clang diagnostic push

    // This function is intended for use only with string
    // literals, so disabling warning should be ok:
    #pragma clang diagnostic ignored "-Wformat-nonliteral"

    va_list args_copy_for_size_calculation;
    va_copy(args_copy_for_size_calculation, vprintf_args);

    // Calculate buffer size first, works since C99 
    int buffer_size = vsnprintf(NULL, 0, format, args_copy_for_size_calculation) + 1;
    #pragma clang diagnostic pop

    if (buffer_size < 0) {
        fprintf(stderr, "Lost and unwrapped error: %s\n", format);

        // vsprintf failed, we need to notify user about that:
        format = "Message size calculation failed, actual error was lost!";

        // We could be unable to notify user about actual error with
        // trace mechanism, that's a big problem, let's show message:
        fprintf(stderr, "%s", format);

        // Update buffer size, format is just a string now, so it's size will be just:
        buffer_size = (int) strlen(format);
    }

    char* message_buffer = (char*) calloc((size_t) buffer_size, sizeof(*message_buffer));

    if (message_buffer == NULL) {
        fprintf(stderr, "Lost and unwrapped error: %s\n", format);
        format = "Trace message allocation failed, actual error was lost";

        // We could be unable to notify user about error with
        // trace mechanism, that's a big problem, let's show message:
        perror("Trace message allocation failed, actual error was lost");

        message_buffer = __trace_error_message_reserved_space_in_case_calloc_fails;
    }

    #pragma clang diagnostic push

    // This function is intended for use only with string
    // literals, so disabling warning should be ok:
    #pragma clang diagnostic ignored "-Wformat-nonliteral"
    int written_byte = vsnprintf(message_buffer, (size_t) buffer_size,
                                 format, vprintf_args);
    #pragma clang diagnostic pop

    // Final message with const qualifier
    const char* message = message_buffer;

    if (written_byte < 0) {
        fprintf(stderr, "Lost and unwrapped error: %s\n", format);
        message = "Error message construction failed, actual error was lost!";

        // We are unable to notify our user about actual error with
        // trace mechanism, that's a big problem, let's show message:
        fprintf(stderr, "%s", message);
    }

    new_trace->trace = cause;
    new_trace->latest_error = error {
        .error_code = code,
        .description = message_buffer,
        .occured = occured
    };

    return new_trace;
}

static void __trace_print_description_indented(FILE* stream, const char* string,
                                               const char* indentation) {

    fprintf(stream, "%s", indentation);

    for (int i = 0; string[i] != '\0'; ++ i) {
        char symbol = string[i];

        fprintf(stream, "%c", symbol);

        if (symbol == '\n')
            fprintf(stream, "%s", indentation);
    }

    fprintf(stream, "\n");
}

static void __trace_print_occurance(FILE* stream, occurance* occurance) {
    fprintf(stream, TEXT_INFO("In %s:%d %s:") "\n", occurance->file,
            occurance->line, occurance->function);
}

static void trace_print_stack_trace(FILE* stream, stack_trace* trace) {
    if (trace == NULL || stream == NULL || trace_is_success(trace))
        return;

    __trace_print_occurance(stream, &trace->latest_error.occured);

    fprintf(stream, "==> " TEXT_ERROR("Error occured: ") "\n"
            COLOR_WARNING /* Color for description */);

    __trace_print_description_indented(stream,
        trace->latest_error.description, TAB);

    fprintf(stream, COLOR_RESET "\n");

    int trace_depth = 1;

    stack_trace* current_trace = trace;
    while ((current_trace = current_trace->trace) != NULL) {
        if (trace_is_success(current_trace))
            break;

        fprintf(stream, TAB "| " TEXT_SUCCESS("Depth %d") " | ",
                trace_depth ++);

        __trace_print_occurance(stream,
            &current_trace->latest_error.occured);

        fprintf(stream, TAB "| ==> " TEXT_ERROR("Caused error:") " \n");

        __trace_print_description_indented(stream,
            current_trace->latest_error.description,
            TAB COLOR_RESET "|" COLOR_WARNING TAB);

        fprintf(stream, COLOR_RESET "\n");
    }
}

static void trace_destruct(stack_trace* trace) {
    if (trace == NULL || trace == SUCCESS())
        return;

    trace_destruct(trace->trace);

    free((char*) trace->latest_error.description);
    free(trace), trace = NULL;
}

#endif
