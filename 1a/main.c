/*********************************************
 * Module: SYPROG WS 2010
 * Author: Jakob Gruber ( 0203440 )
 * Description: Compresses either files or text from stdin
 * Assignment: 1a
 * *******************************************/

#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <stdio.h>

FILE
    *infile = NULL, 
    *outfile = NULL;
char *appname;

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
