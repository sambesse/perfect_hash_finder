/* simple program to more or less brute force finding a perfect hash algorithm for a fixed input */

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <time.h>
#include <errno.h>

#include "uthash.h"

struct key_value {
    uint16_t key;
    char *val;
    UT_hash_handle hh;
};

int compare(const void *a, const void *b) {
    return ((struct key_value *)a)->key - ((struct key_value *)b)->key;
}

#define N_STRESS_TEST   100000
#define MAX_CAN_ID      0x610

#define HASH(x) ((k*x % p) % m)

const uint32_t p = 66587; //randomly chosen prime larger than the universe of possible CAN ids
const uint32_t max_k = (UINT32_MAX / 4096);
const uint32_t min_k = (uint32_t)(p / 10); //smallest CAN id, so we don't search things where the x%p=x
struct key_value **hash_table;
uint32_t k = min_k;
uint32_t num_inputs = 0;

uint32_t m = 0;

struct key_value *find_hash(uint16_t key);
struct key_value *inputs;
int dumb_search(uint16_t key);
void stress_test_hash(void);
struct key_value *lib_find_hash(uint16_t key);

struct key_value *lib_hash = NULL;

int main(int argc, char **argv) {
    if (argc < 2) {
        printf("too few arguments, usage: perfect_hash_finder <num_elems>");
        return -1;
    }
    num_inputs = atoi(argv[1]);
    if (num_inputs < 0 || (num_inputs == 0 && errno != 0)) {
        perror("failed to convert number of arguments");
        return -1;
    }
    m = num_inputs;
    printf("starting with %d inputs, %d buckets\n", num_inputs, m);

    uint16_t *can_ids = malloc(num_inputs * sizeof(uint16_t));

    for (uint32_t i = 0; i < num_inputs; i++) {
        can_ids[i] = rand() % MAX_CAN_ID;
        printf("assigning %X to index %d\n", can_ids[i], i);
        uint32_t j = 0;
        for (; j < i; j++) {
            if (can_ids[j] == can_ids[i]) {
                break;
            }
        }
        if (j < i) {
            printf("found duplicate value, trying again\n");
            i--;
            continue;
        }
    }

    inputs = malloc(num_inputs * sizeof(struct key_value));
    if (!inputs) {
        perror("failed to malloc inputs");
        return -1;
    }

    for (uint32_t i = 0; i < num_inputs; i++) {
        inputs[i].key = can_ids[i];
        inputs[i].val = malloc(16);
        if(inputs[i].val)
            sprintf(inputs[i].val, "%x", can_ids[i]);
        else {
            perror("failed to malloc");
            return -1;
        }
        HASH_ADD(hh, lib_hash, key, 2, &inputs[i]);
    }

    qsort(inputs, num_inputs, sizeof(inputs[0]), compare);

    hash_table = malloc(m * sizeof(struct key_value *));
    if (!hash_table) {
        perror("failed to allocate hash table");
        return -1;
    }
    printf("find a k value with num_inputs: %d, num_buckets: %d, starting k: %d, max k: %d\n", num_inputs, m, min_k, max_k);
    memset(hash_table, 0, m * sizeof(struct key_value *));
    uint32_t i = 0;
    while(1) {
        for(; i < num_inputs; i++) {
            uint32_t index = HASH(inputs[i].key);
            if (hash_table[index] != 0) {
                break;
            } else {
                hash_table[index] = &inputs[i];
            }
        }
        if (i == num_inputs) { //congrats no collisions
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
                perror("failed to allocate hash table\n");
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

    for (uint32_t i = 0; i < num_inputs; i++) {
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
        res = bsearch(&key_struct, inputs, num_inputs, sizeof(inputs[0]), compare);
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
    for (uint32_t i = 0; i < num_inputs; i++) {
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
