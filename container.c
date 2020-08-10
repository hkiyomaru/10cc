#include "9cc.h"

int INITIAL_VECTOR_SIZE = 32;

Vector *create_vec() {
    Vector *vec = malloc(sizeof(Vector));
    vec->data = malloc(sizeof(void *) * INITIAL_VECTOR_SIZE);
    vec->capacity = INITIAL_VECTOR_SIZE;
    vec->len = 0;
    return vec;
}

void add_elem_to_vec(Vector *vec, void *elem) {
    if (vec->len == vec->capacity) {
        vec->capacity *= 2;
        vec->data = realloc(vec->data, sizeof(void *) * vec->capacity);
    }
    vec->data[vec->len++] = elem;
}

void *get_elem_from_vec(Vector *vec, int key) {
    if (vec->len <= key) {
        error("IndexError: vector index out of range");
    }
    return vec->data[key];
}

Map *create_map() {
    Map *map = malloc(sizeof(Map));
    map->keys = create_vec();
    map->vals = create_vec();
    map->len = 0;
    return map;
}

void add_elem_to_map(Map *map, char *key, void *val) {
    for (int i = 0; i < map->len; i++) {
        if (strcmp(get_elem_from_vec(map->keys, i), key) == 0) {
            void *old_val = get_elem_from_vec(map->vals, i);
            old_val = val;
            return;
        }
    }
    add_elem_to_vec(map->keys, key);
    add_elem_to_vec(map->vals, val);
    map->len++;
}

void *get_elem_from_map(Map *map, char *key) {
    for (int i = 0; i < map->len; i++) {
        if (strcmp(get_elem_from_vec(map->keys, i), key) == 0) {
            return get_elem_from_vec(map->vals, i);
        }
    }
    return NULL;
}
