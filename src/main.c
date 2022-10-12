#include <stdio.h>
#include <stdlib.h>

#include "argo.h"
#include "global.h"
#include "debug.h"

#ifdef _STRING_H
#error "Do not #include <string.h>. You will get a ZERO."
#endif

#ifdef _STRINGS_H
#error "Do not #include <strings.h>. You will get a ZERO."
#endif

#ifdef _CTYPE_H
#error "Do not #include <ctype.h>. You will get a ZERO."
#endif

int main(int argc, char **argv)
{
    /**
     * Initialize variables.
     */
    argo_lines_read = 0;
    argo_chars_read = 0;
    argo_next_value = 0;
    ARGO_VALUE *argo_root = NULL;
    int write_error = 0;
    indent_level = 0;
    global_options = 0x00000000;

    /**
     * If validargs returns -1 indicating failure, your program must call
     * USAGE(program_name, return_code) and return EXIT_FAILURE.
     */
    if(validargs(argc, argv)){
        USAGE(*argv, EXIT_FAILURE);
    }

    /**
     * If validargs sets the most-significant bit of global_options to 1
     * (i.e. the -h flag was passed), your program must call USAGE(program_name, return_code)
     * and return EXIT_SUCCESS.
     */
    if(global_options == HELP_OPTION){
        USAGE(*argv, EXIT_SUCCESS);
    }

    /**
     * If the -v flag is provided, then the program will read data from standard input
     * (stdin) and validate that it is syntactically correct JSON. If so, the program
     * exits with an EXIT_SUCCESS return code, otherwise the program exits with an
     * EXIT_FAILURE return code.  In the latter case, the program will print to
     * standard error (stderr) an error message describing the error that was discovered.
     * No other output is produced.
     */
    if(global_options == VALIDATE_OPTION){
        argo_root = argo_read_value(stdin);
        if(argo_root == NULL){
            exit(EXIT_FAILURE);
        }
        else{
            exit(EXIT_SUCCESS);
        }
    }

    /*
     * If the -c flag is provided, then the program performs the same function as
     * described for -v, but after validating the input, the program will also output
     * to standard output (stdout) a "canonicalized" version of the input.
     * "Canonicalized" means that the output is in a standard form in which possibilities
     * for variation have been eliminated.  This is described in more detail below.
     * Unless -p has also been specified, then the produced output contains no whitespace
     * (except within strings that contain whitespace characters).
     */
    if(global_options == CANONICALIZE_OPTION){
        argo_root = argo_read_value(stdin);
        if(argo_root == NULL){
            exit(EXIT_FAILURE);
        }
        else{
            write_error = argo_write_value(argo_root, stdout);
            if(write_error){
                exit(EXIT_FAILURE);
            }
            else{
                exit(EXIT_SUCCESS);
            }
        }
    }

    /**
     * If the -p flag is provided, then the -c flag must also have been provided.
     * In this case, newlines and spaces are used to format the canonicalized output
     * in a more human-friendly way.  See below for the precise requirements on where
     * this whitespace must appear.  The INDENT is an optional nonnegative integer argument
     * that specifies the number of additional spaces to be output at the beginning of a line
     * for each increase in indentation level.  The format of this argument must be
     * the same as for a nonnegative integer number in the JSON specification.
     * If -p is provided without any INDENT, then a default value of 4 is used.
     */
    if((global_options >> 8 << 8) == (CANONICALIZE_OPTION|PRETTY_PRINT_OPTION)){
        argo_root = argo_read_value(stdin);
        if(argo_root == NULL){
            exit(EXIT_FAILURE);
        }
        else{
            write_error = argo_write_value(argo_root, stdout);
            if(write_error){
                exit(EXIT_FAILURE);
            }
            else{
                exit(EXIT_SUCCESS);
            }
        }
    }

    return EXIT_FAILURE;
}

/*
 * Just a reminder: All non-main functions should
 * be in another file not named main.c
 */
