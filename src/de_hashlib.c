/* 
 * Copyright (C) 2012 by 
 * Dent Earl (dearl@soe.ucsc.edu, dentearl@gmail.com)
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE. 
 */

#include <assert.h>
#include <inttypes.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "de_hashlib.h"

const double kd_LOAD = (3.0 / 4.0); // fractional hash table load factor

deHash_t* deHash_construct(size_t capacity, hash_f hash, cmp_f cmp, keydup_f keydup, itemdup_f itemdup,
                           keyrel_f keyrel, itemrel_f itemrel) {
    deHash_t *htab = (deHash_t *) de_malloc(sizeof(*htab));
    // fprintf(stderr, "newHashTable: %p\n", htab);
    htab->table = (deHash_head_t **) de_malloc(capacity * sizeof(*htab->table));
    size_t i;
    // empty chains have no heads
    for (i = 0; i < capacity; ++i)
        htab->table[i] = NULL;
    htab->size = 0;
    htab->capacity = capacity;
    htab->hash = hash;
    htab->cmp = cmp;
    htab->keydup = keydup;
    htab->itemdup = itemdup;
    htab->keyrel = keyrel;
    htab->itemrel = itemrel;
    return htab;
}
void* deHash_search(deHash_t *htab, const void *key) {
    uint32_t h = htab->hash(key) & (htab->capacity - 1);
    /* Search the chain only if it exists */
    if (htab->table[h] != NULL) {
        deHash_node_t *this = htab->table[h]->first;
        for ( ; this != NULL; this = this->next ) {
            if (htab->cmp(key, this->key) == 0)
                return (void*)this->item;
        }
    }
    return NULL;
}
static deHash_node_t* de_newNode(const void *key, const void *item, deHash_node_t *next) {
    deHash_node_t *node = (deHash_node_t*) de_malloc(sizeof(*node));
    node->key = key;
    node->item = item;
    node->next = next;
    return node;
}
static deHash_head_t* de_newChain(void) {
    deHash_head_t *chain = (deHash_head_t*) de_malloc(sizeof *chain);
    chain->first = NULL;
    chain->size = 0;
    return chain;
}
void deHash_destruct(deHash_t *ht) {
    // fprintf(stderr, "destroying hash with size %zu, cap %zu, add %p\n", ht->size, ht->capacity, ht);
    deHash_node_t *save, *this;
    
    // release each chain 
    for (size_t i = 0; i < ht->capacity; ++i) {
        if (ht->table[i] == NULL)
            continue;
        this = ht->table[i]->first;
        for ( ; this != NULL; this = save) {
            save = this->next;
            ht->keyrel(this->key);
            ht->itemrel(this->item);
            free(this);
        }
        free(ht->table[i]);
    }
    ht->capacity = 0;
    ht->size = 0;
    // fprintf(stderr, "oldhash size: %zu, cap: %zu\n", ht->size, ht->capacity);
    free(ht->table);
    free(ht);
}
void deHash_resize(deHash_t **ht) {
    // resizes table in place
    // fprintf(stderr, "Resize called, cur size: %zu, cur cap: %zu, add: %p\n", 
    //         (*ht)->size, (*ht)->capacity, *ht);
    deHash_t *newHt;
    deHash_node_t *this;
    newHt = deHash_construct((*ht)->capacity << 1, (*ht)->hash, (*ht)->cmp, (*ht)->keydup, 
                             (*ht)->itemdup, (*ht)->keyrel, (*ht)->itemrel);
    assert(newHt != *ht);
    // printf("newHt add: %p\n", newHt);
    for (size_t i = 0; i < (*ht)->capacity; ++i) {
        if ((*ht)->table[i] == NULL)
            continue;
        for (this = (*ht)->table[i]->first; this != NULL; this = this->next)
            deHash_insert(&newHt, this->key, this->item);
    }
    // fprintf(stderr, "oldhash cap: %zu, add: %p\n", (*ht)->capacity, *ht);
    deHash_destruct(*ht);
    *ht = newHt;
    // fprintf(stderr, "newhash cap: %zu, add: %p\n", (*ht)->capacity, *ht);
}
int deHash_insert(deHash_t **ht, const void *key, const void *item) {
    // automatically resizes table when size of table reaches 50% capacity.
    deHash_t *htab = *ht;
    if ((htab->capacity * kd_LOAD) < htab->size + 1) {
        // printf("\npre resize size: %zu, cap: %zu, add: %p\n", htab->size, htab->capacity, htab);
        deHash_resize(ht);
        htab = *ht;
        // printf("post resize size: %zu, cap: %zu, add: %p\n", htab->size, htab->capacity, htab);
        assert(0 < htab->capacity);
    }
    uint32_t h = htab->hash(key) & (htab->capacity - 1);
    void *dupkey, *dupitem;
    deHash_node_t *newItem;
    /* Disallow duplicate keys */
    if (deHash_search(htab, key) != NULL)
        return 0;
    /* Attempt to create a new item */
    dupkey = htab->keydup(key);
    dupitem = htab->itemdup(item);
    newItem = de_newNode(dupkey, dupitem, NULL);
    if (newItem == NULL)
        return 0;
    /* Create a chain if the bucket is empty */
    if (htab->table[h] == NULL) {
        htab->table[h] = de_newChain();
        if (htab->table[h] == NULL) {
            htab->keyrel(newItem->key);
            htab->itemrel(newItem->item);
            free(newItem);
            return 0;
        }
    }
    /* Insert at the front of the chain */
    newItem->next = htab->table[h]->first;
    htab->table[h]->first = newItem;
    ++htab->table[h]->size;
    ++htab->size;
    return 1;
}
uint32_t fnv_hash(const void *key, int len) {
    // Fowler/Noll/Vo, good starting point
    const unsigned char *p = key;
    uint32_t h = 2166136261;
    int i;
    for (i = 0; i < len; ++i)
        h = (h * 16777619) ^ p[i];
    return h;
}
uint32_t fnv_hash_str(const void *key) {
    return fnv_hash(key, strlen(key));
}
