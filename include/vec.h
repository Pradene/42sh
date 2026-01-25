#pragma once

#include <string.h>

#define vec_free(v)                                                            \
  ((v)->data ? free((v)->data) : (void)0, (v)->data = NULL, (v)->size = 0,     \
   (v)->capacity = 0)

#define vec_reserve(v, n)                                                      \
  ((n) > (v)->capacity ? ((v)->data = (typeof((v)->data))realloc(              \
                              (v)->data, (n) * sizeof(*(v)->data)))            \
                             ? ((v)->capacity = (n), (void)0)                  \
                             : (void)0                                         \
                       : (void)0)

#define vec_push(v, content)                                                   \
  ((v)->size >= (v)->capacity                                                  \
       ? vec_reserve(v, ((v)->capacity == 0) ? 8 : (v)->capacity * 2)          \
       : (void)0,                                                              \
   (v)->data[(v)->size++] = (content))

#define vec_pop(v) ((v)->size > 0 ? (v)->size-- : 0)

#define vec_at(v, i) ((v)->data[i])

#define vec_size(v) ((v)->size)

#define vec_capacity(v) ((v)->capacity)

#define vec_clear(v) ((v)->size = 0)

#define vec_begin(v) ((v)->data)

#define vec_end(v) ((v)->data + (v)->size)

#define vec_insert(v, i, val)                                                  \
  do {                                                                         \
    if ((v)->size >= (v)->capacity) {                                          \
      size_t new_cap = ((v)->capacity == 0) ? 8 : (v)->capacity * 2;           \
      vec_reserve(v, new_cap);                                                 \
    }                                                                          \
    if ((i) < (v)->size) {                                                     \
      memmove(&(v)->data[(i) + 1], &(v)->data[i],                              \
              ((v)->size - (i)) * sizeof(*(v)->data));                         \
    }                                                                          \
    (v)->data[(i)] = (val);                                                    \
    (v)->size++;                                                               \
  } while (0)

#define vec_remove(v, i)                                                       \
  do {                                                                         \
    if ((i) < (v)->size) {                                                     \
      if ((i) < (v)->size - 1) {                                               \
        memmove(&(v)->data[i], &(v)->data[(i) + 1],                            \
                ((v)->size - (i) - 1) * sizeof(*(v)->data));                   \
      }                                                                        \
      --(v)->size;                                                             \
    }                                                                          \
  } while (0)

#define vec_foreach(Type, it, vec) for (Type *it = (vec)->data; it < (vec)->data + (vec)->size; ++it)