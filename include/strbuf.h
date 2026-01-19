#pragma once

#include <stddef.h>
#include <string.h>

typedef struct {
  char *data;
  size_t size;
  size_t capacity;
} StringBuffer;

// Free vector memory
#define strbuf_free(v)                                                         \
  ((v)->data ? free((v)->data) : (void)0, (v)->data = NULL, (v)->size = 0,     \
   (v)->capacity = 0)

// Reserve capacity (internal helper)
#define strbuf_reserve(v, n)                                                   \
  ((n) > (v)->capacity                                                         \
       ? (typeof((v)->data))realloc((v)->data, (n) * sizeof(*(v)->data))       \
             ? ((v)->data = (typeof((v)->data))realloc(                        \
                    (v)->data, (n) * sizeof(*(v)->data)),                      \
                (v)->capacity = (n), (void)0)                                  \
             : (void)0                                                         \
       : (void)0)

// Push an element to the back
#define strbuf_push(v, val)                                                    \
  ((v)->size >= (v)->capacity                                                  \
       ? strbuf_reserve(v, ((v)->capacity == 0) ? 8 : (v)->capacity * 2)       \
       : (void)0,                                                              \
   (v)->data[(v)->size++] = (val))

// Append a cstr to the back (concat)
#define strbuf_append(v, cstr)                                                 \
  do {                                                                         \
    size_t len = strlen(cstr);                                                 \
    while ((v)->size + len >= (v)->capacity) {                                 \
      strbuf_reserve(v, ((v)->capacity == 0) ? 16 : (v)->capacity * 2);        \
    }                                                                          \
    for (size_t i = 0; i < len; ++i) {                                         \
      (v)->data[(v)->size++] = cstr[i];                                        \
    }                                                                          \
  } while (0)

// Pop an element from the back
#define strbuf_pop(v) ((v)->size > 0 ? (v)->size-- : 0)

// Get element at index
#define strbuf_at(v, i) ((v)->data[i])

// Get size
#define strbuf_size(v) ((v)->size)

// Get capacity
#define strbuf_capacity(v) ((v)->capacity)

// Clear the strbuf (keep capacity)
#define strbuf_clear(v) ((v)->size = 0)

// Get pointer to first element
#define strbuf_begin(v) ((v)->data)

// Get pointer to one past last element
#define strbuf_end(v) ((v)->data + (v)->size)

// Insert at index
#define strbuf_insert(v, i, val)                                               \
  do {                                                                         \
    if ((v)->size >= (v)->capacity) {                                          \
      size_t new_cap = ((v)->capacity == 0) ? 8 : (v)->capacity * 2;           \
      strbuf_reserve(v, new_cap);                                              \
    }                                                                          \
    if ((i) < (v)->size) {                                                     \
      memmove(&(v)->data[(i) + 1], &(v)->data[i],                              \
              ((v)->size - (i)) * sizeof(*(v)->data));                         \
    }                                                                          \
    (v)->data[i] = (val);                                                      \
    (v)->size++;                                                               \
  } while (0)

// Remove at index
#define strbuf_remove(v, i)                                                    \
  do {                                                                         \
    if ((i) < (v)->size) {                                                     \
      if ((i) < (v)->size - 1) {                                               \
        memmove(&(v)->data[i], &(v)->data[(i) + 1],                            \
                ((v)->size - (i) - 1) * sizeof(*(v)->data));                   \
      }                                                                        \
      (v)->size--;                                                             \
    }                                                                          \
  } while (0)
