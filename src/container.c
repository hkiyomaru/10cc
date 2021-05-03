#include "10cc.h"

int INITIAL_VECTOR_SIZE = 32;

// Create an empty vector.
Vector *vec_create() {
    Vector *vec = malloc(sizeof(Vector));
    vec->data = malloc(sizeof(void *) * INITIAL_VECTOR_SIZE);
    vec->capacity = INITIAL_VECTOR_SIZE;
    vec->len = 0;
    return vec;
}

// Push an item to a vector.
void vec_push(Vector *vec, void *item) {
    if (vec->len == vec->capacity) {
        vec->capacity *= 2;
        vec->data = realloc(vec->data, sizeof(void *) * vec->capacity);
    }
    vec->data[vec->len++] = item;
}

// Push an iteger to a vector.
void vec_pushi(Vector *vec, int item) { vec_push(vec, (void *)(intptr_t)item); }

// Get an item from a vector. If no such item exists, raise an error.
void *vec_at(Vector *vec, int index) {
    if (vec->len <= index) {
        error("out of range");
    }
    return vec->data[index];
}

// Get an integer from a vector. If no such item exists, raise an error.
int vec_ati(Vector *vec, int index) { return (intptr_t)(vec_at(vec, index)); }

// Set an item to a vector. If no such item exists, raise an error.
void *vec_set(Vector *vec, int index, void *item) {
    if (vec->len <= index) {
        error("out of range");
    }
    vec->data[index] = item;
    return item;
}

// Get the last item of a vector. Calling vec_back on an empty container causes undefined behavior.
void *vec_back(Vector *vec) { return vec->data[vec->len - 1]; }

// Create an empty map.
Map *map_create() {
    Map *map = malloc(sizeof(Map));
    map->keys = vec_create();
    map->vals = vec_create();
    map->len = 0;
    return map;
}

// Insert an item to a map.
void map_insert(Map *map, char *key, void *val) {
    for (int i = 0; i < map->len; i++) {
        if (!strcmp(vec_at(map->keys, i), key)) {
            map->vals->data[i] = val;
            return;
        }
    }
    vec_push(map->keys, key);
    vec_push(map->vals, val);
    map->len++;
}

// Get an item from a map. If no such item exists, raise an error.
void *map_at(Map *map, char *key) {
    for (int i = 0; i < map->len; i++) {
        if (!strcmp(vec_at(map->keys, i), key)) {
            return vec_at(map->vals, i);
        }
    }
    error("out of range");
    return NULL;  // unreachable
}

// Check if there is an element with key.
bool map_contains(Map *map, char *key) {
    for (int i = 0; i < map->len; i++) {
        if (!strcmp(vec_at(map->keys, i), key)) {
            return true;
        }
    }
    return false;
}
