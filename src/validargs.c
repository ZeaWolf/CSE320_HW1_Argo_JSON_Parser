#include <stdlib.h>

#include "argo.h"
#include "global.h"
#include "debug.h"
#include "utils.h"
/**
 * @brief Validates command line arguments passed to the program.
 * @details This function will validate all the arguments passed to the
 * program, returning 0 if validation succeeds and -1 if validation fails.
 * Upon successful return, the various options that were specified will be
 * encoded in the global variable 'global_options', where it will be
 * accessible elsewhere in the program.  For details of the required
 * encoding, see the assignment handout.
 *
 * @param argc The number of arguments passed to the program from the CLI.
 * @param argv The argument strings passed to the program from the CLI.
 * @return 0 if validation succeeds and -1 if validation fails.
 * @modifies global variable "global_options" to contain an encoded representation
 * of the selected program options.
 */



int validargs(int argc, char **argv) {
    // TO BE IMPLEMENTED

    /**
     * return -1 if no flag is provided.
     */
    if(argc <= 1){
        global_options=0x00000000;
        return -1;
    }

    char **ap = argv;       // argument pointer that points to the current argument
    ap++;       // first argument

    char *H_FLAG = "-h", *V_FLAG = "-v", *C_FLAG = "-c", *P_FLAG = "-p";    // pre-defined strings for flags
    int v_exist = 0, c_exist = 0, p_exist = 0;      // boolean to record if v, c, p, flags has been provided

    int num = 0;        // num of indentation for p flag
    int i = 1;      // argument index
    char *previous = "";        // string to store previous

    /**
     * while loop that check all the arguments.
     */
    while(i < argc){

        /**
         * h flag, if provided, need to be the first argument.
         * set global_options to 0x80000000.
         */
        if(compare_string(*ap, H_FLAG)){
            if(i == 1){
                global_options=0x80000000;
                return 0;
            }
            global_options=0x00000000;
            return -1;
        }

        /**
         * v flag is not allowed when another v or c flag exist.
         * set global_options to 0x40000000.
         */
        else if(compare_string(*ap, V_FLAG)){
            if(v_exist || c_exist){
                global_options=0x00000000;
                return -1;
            }
            global_options=0x40000000;
            v_exist = 1;
        }

        /**
         * c flag is not allowed when another v or c flag exist.
         * set global_options to 0x20000000.
         */
        else if(compare_string(*ap, C_FLAG)){
            if(v_exist || c_exist){
                global_options=0x00000000;
                return -1;
            }
            global_options=0x20000000;
            c_exist = 1;
        }

        /**
         * p flag can only appear after c flag.
         * p flag is not allowed when another v or p flag exist.
         * set global_options to 0x30000004. (defualt 4 indentations)
         */
        else if(compare_string(*ap, P_FLAG)){
            if(!compare_string(previous, C_FLAG)){
                global_options=0x00000000;
                return -1;
            }
            if(v_exist || p_exist){
                global_options=0x00000000;
                return -1;
            }
            global_options=0x30000004;
            p_exist = 1;
        }

        /**
         * digit string can only contain positive digit char. (positive integer only)
         * max indentation is 255 (0xFF).
         * digit string can only appear after p flag.
         * set global options to 0x300000xx (bitwise or with num).
         */
        else if(is_digit_string(*ap)){
            if(!compare_string(previous, P_FLAG)){
                global_options=0x00000000;
                return -1;
            }
            num = string_to_int(*ap);
            if(num>255){
                global_options=0x00000000;
                return -1;
            }
            global_options = 0x30000000 | num;
        }
        else{
            global_options=0x00000000;
            return -1;
        }

        /**
         * previous now points to current argument.
         * go to next argument by increasement ap and i.
         */
        previous = *ap;
        ap++;
        i++;
    }

    //abort();
    /**
     * return 0 if no error occur.
     */
    return 0;
}
