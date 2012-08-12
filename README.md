# faMask

August 2012

## Author
[Dent Earl](https://github.com/dentearl/)

## Description
This stripped down utility takes in two inputs, a [fasta file](http://en.wikipedia.org/wiki/FASTA_format)
and a [bed file](http://genome.ucsc.edu/FAQ/FAQformat.html#format1) and then masks
the fasta file according to intervals found in the bed file. The fasta sequence name
and the a bed record's sequence field must match for the mask to be applied. The 
<code>--softAdd</code> option may be applied to retain the masking state of the fasta,
otherwise the fasta sequence is first set to uppercase then masked soley based on the bed.

## Dependencies
* [sonLib](https://github.com/benedictpaten/sonLib)

## Installation
0. Download and sonLib, switch to the tag <code>july2012_0</code> using <code>git co july2012_0</code> and <code>make</code> the library.
1. Download the package, the sonLib directory should be in the same directory as the faMask directory (the two repos should be siblings).
2. <code>cd</code> into the directory.
3. Type <code>make</code>.

## Use
    $ Usage: faMask --fa fastaFile.fa --bed bedFile.bed [--softAdd]
