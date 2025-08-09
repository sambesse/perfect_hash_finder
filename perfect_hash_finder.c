/* simple program to more or less brute force finding a perfect hash algorithm for a fixed input */

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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
};

#define NUM_INPUTS (uint32_t)(sizeof(can_ids) / (sizeof(can_ids[0])))
#define NUM_BUCKETS ((uint32_t)((uint32_t)(NUM_INPUTS) * 1.5) + 1)

#define HASH(x) ((k*x % p) % NUM_BUCKETS)


const uint32_t p = 66587; //randomly chosen prime larger than the universe of possible CAN ids
const uint32_t max_k = (UINT32_MAX / 4096);
const uint32_t min_k = (uint32_t)(p / 10); //smallest CAN id, so we don't search things where the x%p=x

int main() {
    uint16_t *hash_table = malloc(NUM_BUCKETS * sizeof(uint16_t));
    if (!hash_table) {
        printf("failed to allocate hash table\n");
        return -1;
    }
    printf("find a k value with num_inputs: %d, num_buckets: %d, starting k: %d, max k: %d\n", NUM_INPUTS, NUM_BUCKETS, min_k, max_k);
    memset(hash_table, 0, sizeof(can_ids));
    uint32_t k = min_k;
    uint32_t i = 0;
    while(1) {
        for(; i < NUM_INPUTS; i++) {
            uint32_t index = HASH(can_ids[i]);
            if (hash_table[index] != 0) {
                break;
            } else {
                hash_table[index] = can_ids[i];
            }
        }
        if (i == NUM_INPUTS) { //congrats no collisions
            break; 
        }
        memset(hash_table, 0, NUM_BUCKETS * sizeof(uint16_t));
        i = 0;
        k++;
    }
    printf("here's your hash table, with k = %d, bitch\n", k);
    for (uint32_t i = 0; i < NUM_BUCKETS; i++) {
        printf("%X\n", hash_table[i]);
    }
}
