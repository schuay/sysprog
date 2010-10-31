/*********************************************
 * Module: SYSPROG WS 2010 TU Wien
 * Author: Jakob Gruber ( 0203440 )
 * Description: Compresses either files or text from stdin
 * Assignment: 1a
 * Date: 2010-10-31
 * *******************************************/

#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <stdio.h>
#include <assert.h>
#include <libgen.h>

#define CHKPRINTF(a) { if(a < 0) { \
    (void)fprintf(stderr, "%s: printf failed", appname); return(-1); } }

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
    (void)fprintf(stderr, "usage: %s [ infile1 [ infile2 ... ]]\n", appname);
}
/*
Name: parseargs
Desc: parses and processes cli args (in this case, prints 
      usage if any argument is present and exits)
Args:
    argc: argument count
    argc: argument strings
Returns:
    0 on success, nonzero on error
*/
int parseargs(int argc, char **argv) {
    int opt;

    while((opt = getopt(argc, argv, "h")) > -1) {
        switch(opt) {
            case 'h':
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
    for(digits = 0; i != 0; i /= 10) digits++;
    return digits;
}

/*
Name: compress
Desc: compresses infile to outfile
Args: infnm: compression source name
      infile: compression source
      outfnm: compression destination name
      outfile: compression destination
Returns: 0 on success, nonzero on error
*/
int compress(const char *infnm, FILE *infile, const char *outfnm, FILE *outfile) {
    int c, ret;
    int prevc = 0,      /* processed char from last loop iteration */
        count = 0,      /* length of current char streak */
        insize = 0,     /* nr of chars in infile */
        outsize = 0;    /* nr of chars in outfile */

    /* read infile one char at a time, and write compressed version to
     * outfile */
    c = fgetc(infile);
    while (c != EOF) {
        insize++;

        /* if processed char has changed (and we are not on the
         * first char of file), write to outfile and reset streak counter*/
        if (prevc != c) {
            if(prevc != 0) {
                ret = fprintf(outfile, "%c%d", prevc, count);
                CHKPRINTF(ret)
                outsize += nrofdigits(count) + 1;
            }
            count = 1;
        /* else increase streak counter */
        } else {
            count++;
        }
        /* remember processed char and get the next one */
        prevc = c;
        c = fgetc(infile);
    }
    if(ferror(infile) != 0) {
        (void)fprintf(stderr, "%s: error reading from infile", appname);
        return(1);
    }
    /* write out final char */
    if(prevc != 0) {
        ret = fprintf(outfile, "%c%d", prevc, count);
        CHKPRINTF(ret)
        outsize += nrofdigits(count) + 1;
    }
    /* print final file size summary */
    ret = printf("%s: %d Zeichen\n%s: %d Zeichen\n", infnm, insize, outfnm, outsize);
    CHKPRINTF(ret)

    return(0);
}

/*
Name: fileclose
Desc: safely closes all file objects
Args: -
Returns: -
Globals: infile, outfile
*/
void fileclose(void) {
    if(infile != NULL) (void)fclose(infile);
    if(outfile != NULL) {
        if(fclose(outfile) != 0) {
            (void)fprintf(stderr, "%s: fclose failed\n", appname);
        }
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
        (void)fprintf(stderr, "%s: %s: %s\n", appname, fname, strerror(errno));
    }
    return(file);
}

/*
Name: getoutfname
Desc: transforms infile name to outfile name
Args:
    path: infile path
    ret: signals success (0) or failure (!= 0) to caller
Returns: outfile name string (must be freed!)
Globals: appname
*/
char *getoutfname(const char *path, int *ret) {
    char *infname, *basefname, *outfname;

    /* copy path string because POSIX basename modifies it */
    infname = strdup(path);
    if(infname == NULL) {
        (void)fprintf(stderr, "%s: error while allocating memory\n", appname);
        *ret = 1;
        return(NULL);
    }
    basefname = basename(infname);
    /* length = length of basename + length of ".comp" + '\0' */
    outfname = malloc(strlen(basefname) + 5 + 1);
    if(outfname == NULL) {
        (void)fprintf(stderr, "%s: error while allocating memory\n", appname);
        *ret = 1;
    /* concat our path components */
    } else {
        strcpy(outfname, basefname);
        strcat(outfname, ".comp");
        *ret = 0;
    }

    free(infname);
    return(outfname);
}

#define EXIT_ERR() { fileclose(); return(1); }
int main(int argc, char **argv) {
    
    char *infnm, *outfnm;
    int arg, ret;

    appname = argv[0];

    if(parseargs(argc, argv) != 0) {
        return(1);
    }

    if(argc < 2) {
        infile = stdin;
        infnm = "stdin";
        outfnm = "Stdin.comp";
        outfile = fileopen(outfnm, "w");
        if(!outfile || !infile) EXIT_ERR()
        if(compress(infnm, infile, outfnm, outfile) != 0) EXIT_ERR()
        fileclose();
    } else {
        for(arg = 1; arg < argc; arg++) {
            infnm = argv[arg];
            infile = fileopen(infnm, "r");
            if(infile == NULL) EXIT_ERR()

            outfnm = getoutfname(argv[arg], &ret);
            if(ret != 0) EXIT_ERR()
            outfile = fileopen(outfnm, "w");
            if(outfile == NULL) { free(outfnm); EXIT_ERR() }

            if(compress(infnm, infile, outfnm, outfile) != 0) {
                free(outfnm);
                EXIT_ERR()
            }
            
            free(outfnm);
            fileclose();
        }
    }

    return(0); 
}
