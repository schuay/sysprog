/*********************************************
 * Module: SYSPROG WS 2010 TU Wien
 * Author: Jakob Gruber ( 0203440 )
 * Description: Compresses either files or text from stdin
 * Assignment: 1a
 * *******************************************/

#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <stdio.h>
#include <getopt.h>
#include <assert.h>

FILE
    *infile = NULL, 
    *outfile = NULL;
char *appname;

/*
Name: usage
Desc: prints usage to stderr
Args: -
Returns:-
*/
void usage(void) {
    fprintf(stderr, "usage: %s [ infile1 [ infile2 ... ]]\n", appname);
}
/*
Name: parseargs
Desc: parses and processes cli args
Args:
    argc: argument count
    argc: argument strings
Returns:
    0 on success, nonzero on error
*/
int parseargs(int argc, char **argv) {
    int opt;

    while((opt = getopt(argc, argv, "h"))) {
        if(opt < 0) break;

        switch(opt) {
            case 'h':
            case 0:
            case '?':
                usage();
                return(-1);
            default:
                assert(0);
        }
    }

    return(0); 
}
/*
Name: nrofdigits
Desc: returns count of digits in number i
Args: i : the number to process
Returns: number of digits in i
*/
int nrofdigits(int i) {
    int digits;
    for (digits = 0; i != 0; i /= 10)
        digits++;
    return digits;
}

/*
Name: compress
Desc: compresses infile to outfile
Args: infile: compression source
      outfile: compression destination
Returns: 0 on success
*/
int compress(FILE *infile, FILE *outfile) {
    int c, prevc = 0, count = 0, insize = 0, outsize = 0;

    c = fgetc(infile);
    while (c != EOF) {
        insize++;

        if (prevc != c) {
            if(prevc != 0) {
                fprintf(outfile, "%c%d", prevc, count);
                outsize += nrofdigits(count) + 1;
            }
            count = 1;
        } else {
            count++;
        }
        prevc = c;
        c = fgetc(infile);
    }
    if(prevc != 0) {
        fprintf(outfile, "%c%d", prevc, count);
        outsize += nrofdigits(count) + 1;
    }
    printf("infile: %d\n", insize);
    printf("outfile: %d\n", outsize);

    return(0);
}

/*
Name: fileclose
Desc: safely closes all file objects
Args: -
Returns: -
Globals: infile, outfile: frees file objects
*/
void fileclose(void) {
    if(infile != NULL) {
        fclose(infile);
    }
    if(outfile != NULL) {
        fclose(outfile);
    }
}

/*
Name: fileopen
Desc: opens and returns a file, checks for errors
Args:
    fname: path of the file to open
    fmode: mode to open the file with
Returns: the file object on success or NULL on error
Globals: appname
*/
FILE *fileopen(const char *fname, const char *fmode) {

    FILE *file = fopen(fname, fmode);
    if(file == NULL) {
        fprintf(stderr, "%s: %s: %s\n", appname, fname, strerror(errno));
    }
    
    return(file);
}

/*
Name: getoutfname
Desc: transforms infile name to outfile name
Args:
    path: infile path
    ret: signals success (0) or failure (!= 0) to caller
Returns: outfile name string
Globals: appname
*/
char *getoutfname(const char *path, int *ret) {
    char *infname, *basefname, *outfname;

    infname = strdup(path);
    basefname = basename(infname);

    if(asprintf(&outfname, "%s.comp", basefname) == -1) {
        fprintf(stderr, "%s: error while allocating memory\n", appname);
        *ret = 1;
    } else {
        *ret = 0;
    }

    free(infname);

    return(outfname);
}

#define EXIT_ERR() { fileclose(); return(1); }
int main(int argc, char **argv) {
    
    char *outfname;
    int arg, asprintf_ret;

    appname = argv[0];

    if(parseargs(argc, argv) != 0) {
        return(1);
    }

    if(argc < 2) {
        infile = stdin;
        outfile = fileopen("Stdin.comp", "w");
        if(!outfile || !infile) EXIT_ERR()

        compress(infile, outfile);

        fileclose();
    } else {
        for(arg = 1; arg < argc; arg++) {
            infile = fileopen(argv[arg], "r");
            if(infile == NULL) EXIT_ERR()

            outfname = getoutfname(argv[arg], &asprintf_ret);
            if(asprintf_ret != 0) EXIT_ERR()
            outfile = fileopen(outfname, "w");
            free(outfname);
            if(outfile == NULL) EXIT_ERR()

            compress(infile, outfile);
            
            fileclose();
        }
    }

    return(0); 
}
