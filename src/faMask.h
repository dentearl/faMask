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
#ifndef FAMASK_H_
#define FAMASK_H_

#include <inttypes.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "common.h"
#include "de_hashlib.h"

typedef struct _bedLine {
    char *line;
    char *name;
    uint32_t start; // 0-based inclusive
    uint32_t stop; // 0-based exclusive
    struct _bedLine *next;
    struct _bedLine *prev;
} bedLine_t;

void usage(void);
int parseArgs(int argc, char **argv, char **fasta, char **bed, bool *softAdd);
bedLine_t *parseBed(const char *bedfile);
bedLine_t *bedLine_construct(void);
void bedLineList_destruct(bedLine_t *b);
void bedLine_destruct(bedLine_t *b);
bedLine_t *bedLine_copy(bedLine_t *b);
bedLine_t *bedLineList_copy(bedLine_t *b);
bedLine_t *getTail(bedLine_t *bl);
deHash_t *hashifyBed(bedLine_t *head);
void processFasta(const char *fasta, deHash_t *bedhash, bool softAdd);
#endif // FAMASK_H_
