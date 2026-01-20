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

#define strbuf_reserve(v, n)                                                   \
  do {                                                                         \
    if ((n) > (v)->capacity) {                                                 \
      char *tmp = (char *)realloc((v)->data, (n) * sizeof(*(v)->data));        \
      if (tmp) {                                                               \
        (v)->data = tmp;                                                       \
        (v)->capacity = (n);                                                   \
      }                                                                        \
    }                                                                          \
  } while (0)

// Push an element to the back
#define strbuf_appendc(v, c)                                                   \
  do {                                                                         \
    if ((v)->size >= (v)->capacity) {                                          \
      strbuf_reserve(v, ((v)->capacity == 0) ? 8 : (v)->capacity * 2);         \
    }                                                                          \
    (v)->data[(v)->size++] = (c);                                              \
    (v)->data[(v)->size] = '\0';                                               \
  } while (0)

// Append a cstr to the back (concat)
#define strbuf_append(v, cstr)                                                 \
  do {                                                                         \
    size_t len = strlen(cstr);                                                 \
    while ((v)->size + len + 1 >= (v)->capacity) {                             \
      strbuf_reserve(v, ((v)->capacity == 0) ? 16 : (v)->capacity * 2);        \
    }                                                                          \
    memcpy((v)->data + (v)->size, cstr, len);                                  \
    (v)->size += len;                                                          \
    (v)->data[(v)->size] = '\0';                                               \
  } while (0)

// Pop an element from the back
#define strbuf_pop(v) ((v)->size > 0 ? (v)->size-- : 0)

// Get element at index
#define strbuf_at(v, i) ((v)->data[i])

// Get size
#define strbuf_size(v) ((v)->size)

// Get pointer to first element
#define strbuf_cstr(v) ((v)->data)

// Insert at index
#define strbuf_insert(v, i, c)                                                 \
  do {                                                                         \
    if ((v)->size >= (v)->capacity) {                                          \
      size_t new_cap = ((v)->capacity == 0) ? 8 : (v)->capacity * 2;           \
      strbuf_reserve(v, new_cap);                                              \
    }                                                                          \
    if ((i) < (v)->size) {                                                     \
      memmove(&(v)->data[(i) + 1], &(v)->data[i],                              \
              ((v)->size - (i)) * sizeof(*(v)->data));                         \
    }                                                                          \
    (v)->data[i] = (c);                                                        \
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
