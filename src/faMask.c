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
#include <ctype.h>
#include <errno.h>
#include <getopt.h>
#include <inttypes.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include "common.h"
#include "faMask.h"

const int kMaxLineLength = 512;

void usage(void) {
    fprintf(stderr, "Usage: faMask --fa fastaFile.fa --bed bedFile.bed [--softAdd]");
}
int parseArgs(int argc, char **argv, char **fasta, char **bed, bool *softAdd) {
    static const char *optString = "f:b:s:";
    static const struct option longOpts[] = {
        {"fa", required_argument, 0, 'f'},
        {"bed", required_argument, 0, 'b'},
        {"softAdd", required_argument, 0, 's'},
        {0, 0, 0, 0}
    };
    int longIndex = 0;
    int key = getopt_long(argc, argv, optString, longOpts, &longIndex);
    while (key != -1) {
        switch (key) {
        case 'f':
            *fasta = de_strdup(optarg);
            break;
        case 'b':
            *bed = de_strdup(optarg);
            break;
        case 's':
            *softAdd = true;
            break;
        default:
            usage();
            fprintf(stderr, "\nError, default message. key=%c optarg:%s\n", key, optarg);
            exit(EXIT_FAILURE);
        }
        key = getopt_long(argc, argv, optString, longOpts, &longIndex);
    }
    if (*fasta == NULL) {
        usage();
        fprintf(stderr, "\nError, specify --fa\n");
        exit(EXIT_FAILURE);
    }
    if (*bed == NULL) {
        usage();
        fprintf(stderr, "\nError, specify --bed\n");
        exit(EXIT_FAILURE);
    }
    FILE *fileHandle = de_fopen(*fasta, "r");
    fclose(fileHandle);
    fileHandle = de_fopen(*bed, "r");
    fclose(fileHandle);
    return optind;
}
bedLine_t *bedLine_construct(void) {
    bedLine_t *b = (bedLine_t*) de_malloc(sizeof(*b));
    b->line = NULL;
    b->name = NULL;
    b->start = 0;
    b->stop = 0;
    b->next = NULL;
    b->prev = NULL;
    return b;
}
bedLine_t *bedLine_copy(bedLine_t *b) {
    bedLine_t *c = bedLine_construct();
    c->line = de_strdup(b->line);
    c->name = de_strdup(b->name);
    c->start = b->start;
    c->stop = b->stop;
    return c;
}
bedLine_t *bedLineList_copy(bedLine_t *b) {
    bedLine_t *head = bedLine_construct();
    bedLine_t *c = head;
    bedLine_t *tmp = NULL;
    while (b != NULL) {
        c->line = de_strdup(b->line);
        c->name = de_strdup(b->name);
        c->start = b->start;
        c->stop = b->stop;
        c->next = bedLine_construct();
        tmp = c;
        c = c->next;
        c->prev = tmp;
        b = b->next;
    }
    return head;
}
void bedLineList_destruct(bedLine_t *b) {
    if (b == NULL) {
        return;
    }
    bedLine_t *tmp = b->next;
    bedLine_destruct(b);
    free(b);
    bedLineList_destruct(tmp);
}
void bedLine_destruct(bedLine_t *b) {
    if (b == NULL) {
        return;
    }
    free(b->line);
    free(b->name);
}
bedLine_t *parseBed(const char *bedfile) {
    FILE *fh = de_fopen(bedfile, "r");
    int nBytes = kMaxLineLength;
    char *ca = de_malloc(nBytes);
    int32_t bytesRead = de_getline(&ca, &nBytes, fh);
    bedLine_t *head = NULL;
    bedLine_t *this = NULL;
    bedLine_t *tmp = NULL;
    while (bytesRead != -1) {
        if (bytesRead > 0) {
            if (head == NULL) {
                head = bedLine_construct();
                this = head;
            } else {
                assert(this != NULL);
                tmp = this;
                this->next = (bedLine_t*) de_malloc(sizeof(*this));
                this = this->next;
                this->prev = tmp;
            }
            this->line = de_strdup(ca);
            this->name = de_strdup(ca);
            int32_t i = sscanf(ca, "%s %" PRIu32 " %" PRIu32 "", this->name, &(this->start), &(this->stop));
            assert(i == 3);
            if (this->prev != NULL) {
                if (strcmp(this->name, this->prev->name) == 0) {
                    // bed file must be sorted
                    assert(this->start > this->prev->start);
                }
            }
        }
        bytesRead = de_getline(&ca, &nBytes, fh);
    }
    free(ca);
    fclose(fh);
    return head;
}
bedLine_t *getTail(bedLine_t *bl) {
    while (bl->next != NULL) {
        bl = bl->next;
    }
    return bl;
}
deHash_t *hashifyBed(bedLine_t *head) {
    deHash_t *ht = deHash_construct(2048, (uint32_t(*)(const void *))fnv_hash_str, 
                                    (int(*)(const void *, const void *))strcmp, 
                                    (void* (*)(const void *))de_strdup,
                                    (void* (*)(const void *))bedLineList_copy, 
                                    (void (*)(const void *))free, 
                                    (void (*)(const void *))bedLineList_destruct);
    bedLine_t *b = head;
    bedLine_t *v = NULL;
    while (b != NULL) {
        v = deHash_search(ht, b->name);
        if (v == NULL) {
            deHash_insert(&ht, b->name, b);
        } else {
            bedLine_t *tmp = getTail(v);
            tmp->next = bedLine_copy(b);
            tmp->next->prev = tmp;
        }
        b = b->next;
    }
    return ht;
}
void processFasta(const char *fasta, deHash_t *bedhash, bool softAdd) {
    FILE *fh = de_fopen(fasta, "r");
    int nBytes = kMaxLineLength;
    char *line = de_malloc(nBytes);
    int32_t bytesRead = de_getline(&line, &nBytes, fh);
    uint32_t c = 0; // current position within sequence
    char *name = de_malloc(nBytes);
    char *linecopy = NULL;
    bedLine_t *b = NULL;
    while (bytesRead != -1) {
        if (bytesRead > 0) {
            if (line[0] == '>') {
                c = 0;
                int32_t j = sscanf(line, ">%s", name);
                assert(j == 1);
                printf("%s\n", line);
                b = deHash_search(bedhash, name);
            } else {
                linecopy = de_strdup(line);
                unsigned n = strlen(linecopy);
                for (unsigned i = 0; i < n; ++i) {
                    if (!softAdd) {
                        // go big or go home
                        linecopy[i] = toupper(linecopy[i]);
                    }
                    while (b != NULL && b->stop <= c) {
                        b = b->next;
                    }
                    if (b != NULL) {
                        if (b->start <= c && c < b->stop) {
                            linecopy[i] = tolower(linecopy[i]);
                        }
                    }
                    ++c;
                }
                printf("%s\n", linecopy);
                free(linecopy);
            }
        }
        bytesRead = de_getline(&line, &nBytes, fh);
    }
    free(line);
    fclose(fh);
}
int main(int argc, char **argv) {
    char *fasta = NULL;
    char *bed = NULL;
    bool softAdd = false;
    
    parseArgs(argc, argv, &fasta, &bed, &softAdd);

    bedLine_t *bedhead = parseBed(bed);
    deHash_t *bedhash = hashifyBed(bedhead);
    processFasta(fasta, bedhash, softAdd);
    
    free(fasta);
    free(bed);
    bedLineList_destruct(bedhead);
    deHash_destruct(bedhash);
    return EXIT_SUCCESS;
}
