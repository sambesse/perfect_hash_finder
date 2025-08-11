/* simple program to more or less brute force finding a perfect hash algorithm for a fixed input */

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "uthash.h"

const uint16_t can_ids [] = {
    0x110,
    0x600,
    0x601,
    0x602,
    0x40,
    0xA,
    0x5A,
    0x16,
    0x297,
    0x313,
    0x300,
    0x306,
    0x123,
    0x124,
    0x125,
    0x129,
    0x22,
    0x29,
    0x30,
    0x12,
    0x19,
    0x200,
    0x201,
    0x202,
    0x203,
    0x204,
    0x205,
    0x206,
    0x207,
    0x209,
    0x210,
    0x211,
    0x212,
    0x213,
    0x214,
    0x215,
    0x216,
    0x217,
    0x218,
    0x219,
    0x220,
};

char *tag[] = {
    "110",
    "600",
    "601",
    "602",
    "40",
    "A",
    "5A",
    "16",
    "297",
    "313",
    "300",
    "306",
    "123",
    "124",
    "125",
    "129",
    "22",
    "29",
    "30",
    "12",
    "19",
    "200",
    "201",
    "202",
    "203",
    "204",
    "205",
    "206",
    "207",
    "208",
    "209",
    "210",
    "211",
    "212",
    "213",
    "214",
    "215",
    "216",
    "217",
    "218",
    "219",
    "220",

};

struct key_value {
    uint16_t key;
    char *val;
    UT_hash_handle hh;
};

int compare(const void *a, const void *b) {
    return ((struct key_value *)a)->key - ((struct key_value *)b)->key;
}

#define NUM_INPUTS (uint32_t)(sizeof(can_ids) / (sizeof(can_ids[0])))
#define N_STRESS_TEST   100000

#define HASH(x) ((k*x % p) % m)

const uint32_t p = 66587; //randomly chosen prime larger than the universe of possible CAN ids
const uint32_t max_k = (UINT32_MAX / 4096);
const uint32_t min_k = (uint32_t)(p / 10); //smallest CAN id, so we don't search things where the x%p=x
struct key_value **hash_table;
uint32_t k = min_k;

uint32_t m = NUM_INPUTS;

struct key_value *find_hash(uint16_t key);
struct key_value *inputs;
int dumb_search(uint16_t key);
void stress_test_hash(void);
struct key_value *lib_find_hash(uint16_t key);

struct key_value *lib_hash = NULL;

int main() {
    inputs = malloc(NUM_INPUTS * sizeof(struct key_value));
    if (!inputs) {
        printf("failed to malloc inputs");
        return -1;
    }

    for (uint32_t i = 0; i < NUM_INPUTS; i++) {
        inputs[i].key = can_ids[i];
        inputs[i].val = tag[i];
        HASH_ADD(hh, lib_hash, key, 2, &inputs[i]);
    }

    qsort(inputs, NUM_INPUTS, sizeof(inputs[0]), compare);

    hash_table = malloc(m * sizeof(struct key_value *));
    if (!hash_table) {
        printf("failed to allocate hash table\n");
        return -1;
    }
    printf("find a k value with num_inputs: %d, num_buckets: %d, starting k: %d, max k: %d\n", NUM_INPUTS, m, min_k, max_k);
    memset(hash_table, 0, m * sizeof(struct key_value *));
    uint32_t i = 0;
    while(1) {
        for(; i < NUM_INPUTS; i++) {
            uint32_t index = HASH(inputs[i].key);
            if (hash_table[index] != 0) {
                break;
            } else {
                hash_table[index] = &inputs[i];
            }
        }
        if (i == NUM_INPUTS) { //congrats no collisions
            break;
        }
        i = 0;
        k++;
        memset(hash_table, 0, m * sizeof(struct key_value *));
        if (k >= max_k) {
            k = 0;
            printf("failed to find acceptable k with m = %d\n", m);
            // TODO: here inc m and go again, so we can find smallest possible m
            m++;
            free(hash_table);
            hash_table = malloc(m * sizeof(struct key_value *));
            if (!hash_table) {
                printf("failed to allocate hash table\n");
                return -1;
            }
            memset(hash_table, 0, m * sizeof(struct key_value *));
        }
    }
    printf("here's your hash table, with k = %d, bitch\n", k);
    for (uint32_t i = 0; i < m; i++) {
        if (hash_table[i])
            printf("%X\n", hash_table[i]->key);
        else 
            printf("%X\n", 0);
    }

    for (uint32_t i = 0; i < NUM_INPUTS; i++) {
        struct key_value *ret;
        printf("i = %d\n", i);
        printf("testing value %x\n", inputs[i].key);
        if ((ret = find_hash(inputs[i].key)) != NULL) {
            printf("found val \"%s\"\n", ret->val);
        } else {
            printf("miss\n");
        }
    }
    printf("dont sanity testing\n");
    stress_test_hash();
}

void stress_test_hash(void) {
    double total_time_hash = 0;
    double total_time_bsearch = 0;
    double total_time_linear = 0;
    double total_time_lib_hash = 0;
    uint16_t keys[N_STRESS_TEST];
    struct timespec start, end;
    struct key_value *res;
    for (int i = 0; i < N_STRESS_TEST; i++) {
        keys[i] = rand() % 0x602;
        struct key_value key_struct = {keys[i], "blah"};

        clock_gettime(CLOCK_MONOTONIC, &start);
        find_hash(keys[i]);
        clock_gettime(CLOCK_MONOTONIC, &end);
        total_time_hash += (end.tv_sec - start.tv_sec)*1e9 + (end.tv_nsec - start.tv_nsec);

        clock_gettime(CLOCK_MONOTONIC, &start);
        res = bsearch(&key_struct, inputs, NUM_INPUTS, sizeof(inputs[0]), compare);
        clock_gettime(CLOCK_MONOTONIC, &end);
        total_time_bsearch += (end.tv_sec - start.tv_sec)*1e9 + (end.tv_nsec - start.tv_nsec);

        clock_gettime(CLOCK_MONOTONIC, &start);
        dumb_search(keys[i]);
        clock_gettime(CLOCK_MONOTONIC, &end);
        total_time_linear += (end.tv_sec - start.tv_sec)*1e9 + (end.tv_nsec - start.tv_nsec);

        clock_gettime(CLOCK_MONOTONIC, &start);
        lib_find_hash(keys[i]);
        clock_gettime(CLOCK_MONOTONIC, &end);
        total_time_lib_hash += (end.tv_sec - start.tv_sec)*1e9 + (end.tv_nsec - start.tv_nsec);
    }
    printf("average hash search time: %f\n", total_time_hash/N_STRESS_TEST);
    printf("average binary search time: %f\n", total_time_bsearch/N_STRESS_TEST);
    printf("average linear search time: %f\n", total_time_linear/N_STRESS_TEST);
    printf("average lib hash search time: %f\n", total_time_lib_hash/N_STRESS_TEST);
}

int dumb_search(uint16_t key) {
    for (uint32_t i = 0; i < NUM_INPUTS; i++) {
        if (inputs[i].key == key)
            return i;
    }
    return -1;
}

struct key_value *find_hash(uint16_t key) {
    uint32_t index = HASH(key);
    if (!hash_table[index])
        return NULL;
    if (hash_table[index]->key == key)
        return hash_table[index];
    return NULL;
}

struct key_value *lib_find_hash(uint16_t key) {
    struct key_value *res;
    HASH_FIND(hh, lib_hash, &key, 2, res);
    return res;
}
