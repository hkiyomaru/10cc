#include "9cc.h"

int INITIAL_VECTOR_SIZE = 32;

/**
 * Creates an empty vector.
 *
 * @return A vector.
 */
Vector *vec_create() {
    Vector *vec = malloc(sizeof(Vector));
    vec->data = malloc(sizeof(void *) * INITIAL_VECTOR_SIZE);
    vec->capacity = INITIAL_VECTOR_SIZE;
    vec->len = 0;
    return vec;
}

/**
 * Pushes an item to a vector.
 *
 * @param vec A vector.
 * @param item An item to be registered.
 */
void vec_push(Vector *vec, void *item) {
    if (vec->len == vec->capacity) {
        vec->capacity *= 2;
        vec->data = realloc(vec->data, sizeof(void *) * vec->capacity);
    }
    vec->data[vec->len++] = item;
}

/**
 * Gets an item from a vector.
 *
 * @param vec A vector.
 * @param index An index.
 *
 * @return An item.
 */
void *vec_get(Vector *vec, int index) {
    if (vec->len <= index) {
        error("IndexError: vector index out of range");
    }
    return vec->data[index];
}

/**
 * Creates an empty map.
 *
 * @return A map.
 */
Map *map_create() {
    Map *map = malloc(sizeof(Map));
    map->keys = vec_create();
    map->vals = vec_create();
    map->len = 0;
    return map;
}

/**
 * Inserts an item to a map.
 *
 * @param map A map.
 * @param key A key.
 * @param val A value.
 */
void map_insert(Map *map, char *key, void *val) {
    for (int i = 0; i < map->len; i++) {
        if (strcmp(vec_get(map->keys, i), key) == 0) {
            void *old_val = vec_get(map->vals, i);
            old_val = val;
            return;
        }
    }
    vec_push(map->keys, key);
    vec_push(map->vals, val);
    map->len++;
}

/**
 * Gets an item from a map.
 * NULL will be returned if the key is not in the map.
 *
 * @param map A map.
 * @param key A key.
 *
 * @return An item.
 */
void *map_at(Map *map, char *key) {
    for (int i = 0; i < map->len; i++) {
        if (strcmp(vec_get(map->keys, i), key) == 0) {
            return vec_get(map->vals, i);
        }
    }
    return NULL;
}
