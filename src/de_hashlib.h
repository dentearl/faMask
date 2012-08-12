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

#ifndef DE_HASHLIB_H_
#define DE_HASHLIB_H_
#include <stddef.h>
#include <stdbool.h>
#include "common.h"

#define length(x) (sizeof(x) / sizeof(x[0]))

// Application specific hash function
typedef uint32_t (*hash_f) (const void *key);
// Application specific key comparison function
typedef int (*cmp_f) (const void *a, const void *b);
// Application specific key copying function
typedef void* (*keydup_f) (const void *key);
// Application specific data copying function
typedef void* (*itemdup_f) (const void *item);
// Application specific key deletion function
typedef void (*keyrel_f) (const void *key);
// Application specific data deletion function
typedef void (*itemrel_f) (const void *item);

typedef struct _deHash_node {
    struct _deHash_node *next;
    const void *key;
    const void *item;
} deHash_node_t;
typedef struct _deHash_head {
    deHash_node_t *first;
    size_t size;
} deHash_head_t;
typedef struct _deHash {
    deHash_head_t **table;
    size_t size;
    size_t capacity;
    hash_f hash;
    cmp_f cmp;
    keydup_f keydup;
    itemdup_f itemdup;
    keyrel_f keyrel;
    itemrel_f itemrel;
} deHash_t;

deHash_t* deHash_construct(size_t size, hash_f hash, cmp_f cmp, keydup_f keydup, itemdup_f itemdup,
                           keyrel_f keyrel, itemrel_f itemrel);
void* deHash_search(deHash_t *htab, const void *key);
void deHash_destruct(deHash_t *ht);
void deHash_resize(deHash_t **ht);
int deHash_insert(deHash_t **htab, const void *key, const void *item); // automatically resizes
uint32_t fnv_hash(const void *key, int len);
uint32_t fnv_hash_str(const void *key);

#endif // DE_HASHLIB_H_
