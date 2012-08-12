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
#include "sonLib.h"
#include "bioioC.h" // benLine
#include "faMask.h"

const int kMaxLineLength = 512;

void usage(void) {
    fprintf(stderr, "Usage: faMask --fa fastaFile.fa --bed bedFile.bed [--soft --softAdd]\n\n"
            "faMask takes a fasta file and masks its output according to the provided \n"
            "bed file. Using --hard will mask residues to 'N' and unmasked residues are\n"
            "made upper-case. Using --soft will mask by making the residues lower-case.\n"
            "Using --softAdd will perserve whatever lower-case residues already exist in\n"
            "the file.\n");
}
FILE* de_fopen(const char *filename, char const *mode) {
    FILE *f = fopen(filename, mode);
    if (f == NULL) {
        if (errno == ENOENT) {
            fprintf(stderr, "ERROR, file %s does not exist.\n", filename);
        } else {
            fprintf(stderr, "ERROR, unable to open file %s for mode \"%s\"\n", filename, mode);
        }
        exit(EXIT_FAILURE);
    }
    return f;
}
int parseArgs(int argc, char **argv, char **fasta, char **bed, bool *hard, bool *soft, bool *softAdd) {
    static const char *optString = "f:b:s:";
    static const struct option longOpts[] = {
        {"fa", required_argument, 0, 'f'},
        {"bed", required_argument, 0, 'b'},
        {"softAdd", no_argument, 0, 'a'},
        {"soft", no_argument, 0, 's'},
        {"hard", no_argument, 0, 'h'},
        {0, 0, 0, 0}
    };
    int longIndex = 0;
    int key = getopt_long(argc, argv, optString, longOpts, &longIndex);
    while (key != -1) {
        switch (key) {
        case 'f':
            *fasta = stString_copy(optarg);
            break;
        case 'b':
            *bed = stString_copy(optarg);
            break;
        case 'a':
            *softAdd = true;
            break;
        case 's':
            *soft = true;
            break;
        case 'h':
            *hard = true;
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
    bedLine_t *b = (bedLine_t*) st_malloc(sizeof(*b));
    b->line = NULL;
    b->name = NULL;
    b->start = 0;
    b->stop = 0;
    return b;
}
bedLine_t *bedLine_copy(bedLine_t *b) {
    bedLine_t *c = bedLine_construct();
    c->line = stString_copy(b->line);
    c->name = stString_copy(b->name);
    c->start = b->start;
    c->stop = b->stop;
    return c;
}
void bedLine_destruct(bedLine_t *b) {
    if (b == NULL) {
        return;
    }
    free(b->line);
    free(b->name);
    free(b);
}
int bedLine_cmp(const void *a, const void *b) {
    assert(a != NULL);
    assert(b != NULL);
    bedLine_t *ap = (bedLine_t*) a;
    bedLine_t *bp = (bedLine_t*) b;
    int i;
    if (ap->start < bp->start) {
        i = -1;
    } else if (ap->start > bp->start) {
        i = 1;
    } else {
        if (ap->stop < bp->stop) {
            i = -1;
        } else if (ap->stop > bp->stop) {
            i = 1;
        } else {
            i = 0;
        }
    }
    return i;
}
stHash *parseBed(const char *bedfile) {
    FILE *fh = de_fopen(bedfile, "r");
    int nBytes = kMaxLineLength;
    char *ca = st_malloc(nBytes);
    int32_t bytesRead = benLine(&ca, &nBytes, fh);
    bedLine_t *b = NULL;
    unsigned linenumber = 1;
    stHash *ht = stHash_construct3(stHash_stringKey, stHash_stringEqualKey, 
                                   free, (void(*)(void *)) stSortedSet_destruct);
    stSortedSet *intervals = NULL;
    while (bytesRead != -1) {
        if (bytesRead > 0) {
            b = bedLine_construct();
            b->line = stString_copy(ca);
            b->name = stString_copy(ca);
            int32_t i = sscanf(ca, "%s %" PRIu32 " %" PRIu32 "", b->name, &(b->start), &(b->stop));
            assert(i == 3);
            intervals = stHash_search(ht, b->name);
            if (intervals == NULL) {
                intervals = stSortedSet_construct3((int(*)(const void *, const void *)) bedLine_cmp,
                                                   (void(*)(void *)) bedLine_destruct);
                stHash_insert(ht, stString_copy(b->name), intervals);
            }
            stSortedSet_insert(intervals, b);
        }
        bytesRead = benLine(&ca, &nBytes, fh);
        ++linenumber;
    }
    free(ca);
    fclose(fh);
    return ht;
}
void processFasta(const char *fasta, stHash *bedhash, bool hard, bool soft, bool softAdd) {
    FILE *fh = de_fopen(fasta, "r");
    int nBytes = kMaxLineLength;
    char *line = st_malloc(nBytes);
    int32_t bytesRead = benLine(&line, &nBytes, fh);
    uint32_t c = 0; // current position within sequence
    char *name = st_malloc(nBytes);
    char *linecopy = NULL;
    stSortedSet *sortSet = NULL;
    bedLine_t *b = NULL, *tmp = NULL;
    bedLine_t *faux = bedLine_construct();
    while (bytesRead != -1) {
        if (bytesRead > 0) {
            if (line[0] == '>') {
                c = 0;
                int32_t j = sscanf(line, ">%s", name);
                assert(j == 1);
                printf("%s\n", line);
                sortSet = stHash_search(bedhash, name);
                if (sortSet != NULL) {
                    // fprintf(stderr, "%s: %"PRIi32"\n", name, stSortedSet_size(sortSet));
                }
            } else {
                linecopy = stString_copy(line);
                unsigned n = strlen(linecopy);
                for (unsigned i = 0; i < n; ++i, ++c) {
                    if (!softAdd) {
                        // no Add
                        linecopy[i] = toupper(linecopy[i]);
                    } 
                    if (sortSet != NULL) {
                        faux->start = c + 1;
                        faux->stop = c + 2;
                        b = stSortedSet_getFirst(sortSet);
                        if (b != NULL) {
                            stSortedSetIterator *sit = stSortedSet_getIteratorFrom(sortSet, b);
                            while ((tmp = stSortedSet_getNext(sit)) != NULL) {
                                if (tmp->start > c) {
                                    break;
                                }
                                if (tmp->start <= c && c < tmp->stop) {
                                    if (softAdd || soft) {
                                        linecopy[i] = tolower(linecopy[i]);
                                        break;
                                    } else {
                                        if (hard) {
                                            linecopy[i] = 'N';
                                            break;
                                        }
                                    }
                                }
                            }
                            stSortedSet_destructIterator(sit);
                        }
                    }
                }
                printf("%s\n", linecopy);
                free(linecopy);
            }
        }
        bytesRead = benLine(&line, &nBytes, fh);
    }
    // fprintf(stderr, "count: %u\n", c);
    free(line);
    free(name);
    bedLine_destruct(faux);
    fclose(fh);
}
int main(int argc, char **argv) {
    char *fasta = NULL;
    char *bed = NULL;
    bool softAdd = false;
    bool soft = false;
    bool hard = false;
    
    parseArgs(argc, argv, &fasta, &bed, &hard, &soft, &softAdd);

    stHash *bedhash = parseBed(bed);
    processFasta(fasta, bedhash, hard, soft, softAdd);
    
    free(fasta);
    free(bed);
    stHash_destruct(bedhash);
    return EXIT_SUCCESS;
}
