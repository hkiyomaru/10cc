#include "10cc.h"

int INITIAL_VECTOR_SIZE = 32;

/**
 * Creates an empty vector.
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
 * Pushes an iteger to a vector.
 * @param vec A vector.
 * @param item An item to be registered.
 */
void vec_pushi(Vector *vec, int item) { vec_push(vec, (void *)(intptr_t)item); }

/**
 * Gets an item from a vector.
 * @param vec A vector.
 * @param index An index.
 * @return An item.
 */
void *vec_at(Vector *vec, int index) {
    if (vec->len <= index) {
        error("error: out of range");
    }
    return vec->data[index];
}

/**
 * Gets an iteger from a vector.
 * @param vec A vector.
 * @param index An index.
 * @return An item.
 */
int vec_ati(Vector *vec, int index) {
    if (vec->len <= index) {
        error("error: out of range");
    }
    return (intptr_t)(vec_at(vec, index));
}

/**
 * Sets an item to a vector.
 * @param vec A vector.
 * @param index An index.
 * @param item An item.
 * @return An item.
 */
void *vec_set(Vector *vec, int index, void *item) {
    if (vec->len <= index) {
        error("error: out of range");
    }
    vec->data[index] = item;
    return item;
}

/**
 * Fetches the last item.
 * @param vec A vector.
 * @return An item.
 */
void *vec_back(Vector *vec) { return vec->data[vec->len - 1]; }

/**
 * Creates an empty map.
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
 * @param map A map.
 * @param key A key.
 * @param val A value.
 */
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

/**
 * Gets an item from a map.
 * NULL will be returned if the key is not in the map.
 * @param map A map.
 * @param key A key.
 * @return An item.
 */
void *map_at(Map *map, char *key) {
    for (int i = 0; i < map->len; i++) {
        if (!strcmp(vec_at(map->keys, i), key)) {
            return vec_at(map->vals, i);
        }
    }
    error("error: out of range");
}

/**
 * Returns 1 if there exists an item associated to a given key.
 * @param map A map.
 * @param key A key.
 * @return 1 if there exists an item associated to a given key. Otherwise, 0.
 */
int map_count(Map *map, char *key) {
    for (int i = 0; i < map->len; i++) {
        if (!strcmp(vec_at(map->keys, i), key)) {
            return 1;
        }
    }
    return 0;
}
