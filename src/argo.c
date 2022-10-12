#include <stdlib.h>
#include <stdio.h>

#include "argo.h"
#include "global.h"
#include "debug.h"
#include "utils.h"

/**
 * @brief  Read JSON input from a specified input stream, parse it,
 * and return a data structure representing the corresponding value.
 * @details  This function reads a sequence of 8-bit bytes from
 * a specified input stream and attempts to parse it as a JSON value,
 * according to the JSON syntax standard.  If the input can be
 * successfully parsed, then a pointer to a data structure representing
 * the corresponding value is returned.  See the assignment handout for
 * information on the JSON syntax standard and how parsing can be
 * accomplished.  As discussed in the assignment handout, the returned
 * pointer must be to one of the elements of the argo_value_storage
 * array that is defined in the const.h header file.
 * In case of an error (these include failure of the input to conform
 * to the JSON standard, premature EOF on the input stream, as well as
 * other I/O errors), a one-line error message is output to standard error
 * and a NULL pointer value is returned.
 *
 * @param f  Input stream from which JSON is to be read.
 * @return  A valid pointer if the operation is completely successful,
 * NULL if there is any error.
 */
ARGO_VALUE *argo_read_value(FILE *f) {

    if(argo_next_value >= NUM_ARGO_VALUES){
        fprintf(stderr,"[%d, %d] Number of ARGO Value Exceeds Limit. \n", argo_lines_read, argo_chars_read);
        return NULL;
    }

    // allocate space
    ARGO_VALUE *av = argo_value_storage + argo_next_value;
    argo_next_value++;

    av->type = ARGO_NO_TYPE;
    av->next = NULL;
    av->prev = NULL;
    av->name.capacity = 0;
    av->name.length = 0;
    av->name.content = NULL;

    int c;
    c = fgetc(f);
    argo_chars_read++;

    while(c != EOF){
        if(argo_is_whitespace(c)){
            if(c == ARGO_LF){
                argo_lines_read++;
                argo_chars_read = 0;
            }
            c = fgetc(f);
            argo_chars_read++;
            continue;
        }
        else if(c == ARGO_QUOTE){
            av->type = ARGO_STRING_TYPE;
            if(ungetc(c,f) == EOF){
                fprintf(stderr,"[%d, %d] Fail to unget. \n", argo_lines_read, argo_chars_read);
                return NULL;
            }
            argo_chars_read--;
            if(argo_read_string(&(av->content.string), f)){
                fprintf(stderr,"[%d, %d] Invalid string. \n", argo_lines_read, argo_chars_read);
                return NULL;
            }
            else{
                return av;
            }
        }
        else if(c == ARGO_MINUS || argo_is_digit(c)){
            av->type = ARGO_NUMBER_TYPE;
            if(ungetc(c,f) == EOF){
                fprintf(stderr,"[%d, %d] Fail to unget. \n", argo_lines_read, argo_chars_read);
                return NULL;
            }
            argo_chars_read--;
            if(argo_read_number(&(av->content.number), f)){
                fprintf(stderr,"[%d, %d] Invalid number. \n", argo_lines_read, argo_chars_read);
                return NULL;
            }
            else{
                return av;
            }
        }
        else if(argo_maybe_basic(c)){
            av->type = ARGO_BASIC_TYPE;
            if(ungetc(c,f) == EOF){
                fprintf(stderr,"[%d, %d] Fail to unget. \n", argo_lines_read, argo_chars_read);
                return NULL;
            }
            argo_chars_read--;
            if(argo_read_basic(&(av->content.basic), f)){
                fprintf(stderr,"[%d, %d] Invalid basic. \n", argo_lines_read, argo_chars_read);
                return NULL;
            }
            else{
                return av;
            }
        }
        else if(c == ARGO_LBRACK){
            av->type = ARGO_ARRAY_TYPE;
            if(ungetc(c,f) == EOF){
                fprintf(stderr,"[%d, %d] Fail to unget. \n", argo_lines_read, argo_chars_read);
                return NULL;
            }
            argo_chars_read--;
            if(argo_read_array(&(av->content.array), f)){
                fprintf(stderr,"[%d, %d] Invalid array. \n", argo_lines_read, argo_chars_read);
                return NULL;
            }
            else{
                return av;
            }
        }
        else if(c == ARGO_LBRACE){
            av->type = ARGO_OBJECT_TYPE;
            if(ungetc(c,f) == EOF){
                fprintf(stderr,"[%d, %d] Fail to unget. \n", argo_lines_read, argo_chars_read);
                return NULL;
            }
            argo_chars_read--;
            if(argo_read_object(&(av->content.object), f)){
                fprintf(stderr,"[%d, %d] Invalid object. \n", argo_lines_read, argo_chars_read);
                return NULL;
            }
            else{
                return av;
            }
        }
        else{
            fprintf(stderr, "[%d, %d] Invalid Token (%d)\n", argo_lines_read, argo_chars_read, c);
            return NULL;
        }

        c = fgetc(f);
        argo_chars_read++;
    }

    fprintf(stderr, "JSON Value not found\n");
    return NULL;
}


/**
 * @brief  Read JSON input from a specified input stream, attempt to
 * parse it as a JSON string literal, and return a data structure
 * representing the corresponding string.
 * @details  This function reads a sequence of 8-bit bytes from
 * a specified input stream and attempts to parse it as a JSON string
 * literal, according to the JSON syntax standard.  If the input can be
 * successfully parsed, then a pointer to a data structure representing
 * the corresponding value is returned.
 * In case of an error (these include failure of the input to conform
 * to the JSON standard, premature EOF on the input stream, as well as
 * other I/O errors), a one-line error message is output to standard error
 * and a NULL pointer value is returned.
 *
 * @param f  Input stream from which JSON is to be read.
 * @return  Zero if the operation is completely successful,
 * nonzero if there is any error.
 */
int argo_read_string(ARGO_STRING *s, FILE *f) {

    ARGO_CHAR c = fgetc(f);
    argo_chars_read++;
    if( c != ARGO_QUOTE){
        fprintf(stderr,"[%d, %d] Invalid string\n", argo_lines_read, argo_chars_read);
        return-1;
    }

    int k;
    int ucode;
    c = fgetc(f);
    argo_chars_read++;
    while(c != EOF){

        // end of string
        if(c == ARGO_QUOTE){
            return 0;
        }

        // control characters
        else if(argo_is_control(c)){
            fprintf(stderr, "[%d, %d] Illegal character (%d) in string\n", argo_lines_read, argo_chars_read, c);
            return -1;
        }

        // \ is read
        else if(c == ARGO_BSLASH){
            c = fgetc(f);
            argo_chars_read++;
            if(c == ARGO_QUOTE){
                if(argo_append_char(s, ARGO_QUOTE)){
                    return -1;
                }
            }

            else if(c == ARGO_BSLASH){
                if(argo_append_char(s, ARGO_BSLASH)){
                    return -1;
                }
            }

            else if(c == ARGO_FSLASH){
                if(argo_append_char(s, ARGO_FSLASH)){
                    return -1;
                }
            }

            else if(c == ARGO_B){
                if(argo_append_char(s, ARGO_BS)){
                    return -1;
                }
            }

            else if(c == ARGO_F){
                if(argo_append_char(s, ARGO_FF)){
                    return -1;
                }
            }

            else if(c == ARGO_N){
                if(argo_append_char(s, ARGO_LF)){
                    return -1;
                }
            }

            else if(c == ARGO_R){
                if(argo_append_char(s, ARGO_CR)){
                    return -1;
                }
            }

            else if(c == ARGO_T){
                if(argo_append_char(s, ARGO_HT)){
                    return -1;
                }
            }

            else if(c == ARGO_U){
                ucode = 0;
                for(k=0; k<4; k++){
                    c = fgetc(f);
                    argo_chars_read++;
                    if(argo_is_hex(c)){
                        ucode = ucode * 16;
                        if(argo_is_digit(c)){
                            ucode = ucode + (c-ARGO_DIGIT0);
                        }
                        if(c >= 'A' && c <= 'F'){
                            ucode = ucode + (c-'A'+10);
                        }
                        if(c >= 'a' && c <= 'f'){
                            ucode = ucode + (c-'a'+10);
                        }
                    }
                    else{
                        fprintf(stderr, "[%d, %d] Illegal escape (\\%d) in string\n", argo_lines_read, argo_chars_read, c);
                        return -1;
                    }
                }
                if(argo_append_char(s, ucode)){
                    return -1;
                }
            }

            else{
                fprintf(stderr, "[%d, %d] Illegal escape (\\%d) in string\n", argo_lines_read, argo_chars_read, c);
                return -1;
            }

        }

        else if(argo_append_char(s, c)){
            return -1;
        }

        c = fgetc(f);
        argo_chars_read++;
    }
    fprintf(stderr, "[%d, %d] Expect \" in string but seen (%d)\n", argo_lines_read, argo_chars_read, c);
    return -1;
}



/**
 * @brief  Read JSON input from a specified input stream, attempt to
 * parse it as a JSON number, and return a data structure representing
 * the corresponding number.
 * @details  This function reads a sequence of 8-bit bytes from
 * a specified input stream and attempts to parse it as a JSON numeric
 * literal, according to the JSON syntax standard.  If the input can be
 * successfully parsed, then a pointer to a data structure representing
 * the corresponding value is returned.  The returned value must contain
 * (1) a string consisting of the actual sequence of characters read from
 * the input stream; (2) a floating point representation of the corresponding
 * value; and (3) an integer representation of the corresponding value,
 * in case the input literal did not contain any fraction or exponent parts.
 * In case of an error (these include failure of the input to conform
 * to the JSON standard, premature EOF on the input stream, as well as
 * other I/O errors), a one-line error message is output to standard error
 * and a NULL pointer value is returned.
 *
 * @param f  Input stream from which JSON is to be read.
 * @return  Zero if the operation is completely successful,
 * nonzero if there is any error.
 */
int argo_read_number(ARGO_NUMBER *n, FILE *f) {

    ARGO_STRING *sv = &(n->string_value);
    sv->capacity = 0;
    sv->length=0;
    sv->content=NULL;

    ARGO_CHAR c = fgetc(f);
    argo_chars_read++;
    int neg_flag = 0;
    int dec_flag = 0;
    int exp_flag = 0;
    int exp_neg = 0;

    if( !(argo_is_digit(c) || c==ARGO_MINUS)){
        fprintf(stderr, "[%d, %d] Invalid number char (%d)\n", argo_lines_read, argo_chars_read, c);
        return-1;
    }

    if(c == ARGO_MINUS){
        neg_flag = 1;
        if(argo_append_char(sv, ARGO_MINUS)){
            return -1;
        }
        c = fgetc(f);
        argo_chars_read++;
        if(!argo_is_digit(c)){
            fprintf(stderr, "[%d, %d] Invalid number char (%d)\n", argo_lines_read, argo_chars_read, c);
            return-1;
        }
    }

    if(c == ARGO_DIGIT0){
        if(argo_append_char(sv, ARGO_DIGIT0)){
            return -1;
        }
        c = fgetc(f);
        argo_chars_read++;
        if(c == ARGO_PERIOD && (!dec_flag)){
            if(argo_append_char(sv, ARGO_PERIOD)){
                return -1;
            }
            c = fgetc(f);
            argo_chars_read++;
            if(!argo_is_digit(c)){
                fprintf(stderr, "[%d, %d] Digit expected in number but seen (%d)\n", argo_lines_read, argo_chars_read, c);
                return -1;
            }
            if(ungetc(c, f)==EOF){
                fprintf(stderr,"[%d, %d] Fail to unget. \n", argo_lines_read, argo_chars_read);
                return -1;
            }
            argo_chars_read--;
            dec_flag = 1;
        }
        else{
            if(ungetc(c, f)==EOF){
                fprintf(stderr,"[%d, %d] Fail to unget. \n", argo_lines_read, argo_chars_read);
                return -1;
            }
            n->int_value = 0;
            n->float_value = neg_flag? -0.0 : 0.0;
            n->valid_string = 1;
            n->valid_int = 1;
            n->valid_float = 1;
            return 0;
        }
    }

    double frac_num = 0;
    long int_sum = 0;
    double float_sum = 0;
    int exp_sum = 0;
    int k = 0;


    while(c != EOF){
        if(argo_is_digit(c)){
            if(argo_append_char(sv, c)){
                return -1;
            }
            if(exp_flag){
                exp_sum = (exp_sum*10)+(c-ARGO_DIGIT0);
            }
            else if(dec_flag){
                frac_num = c-ARGO_DIGIT0;
                for(k=0; k<dec_flag; k++){
                    frac_num = frac_num/10.0;
                }
                float_sum = float_sum+frac_num;
                dec_flag++;
            }
            else{
                int_sum = (int_sum*10) + (c-ARGO_DIGIT0);
                float_sum = (float_sum*10) + (c-ARGO_DIGIT0);
            }
        }

        else if( c == ARGO_PERIOD && (!dec_flag)){
            if(argo_append_char(sv, ARGO_PERIOD)){
                return -1;
            }
            c = fgetc(f);
            argo_chars_read++;
            if(!argo_is_digit(c)){
                fprintf(stderr, "[%d, %d] Digit expected in number but seen (%d)\n", argo_lines_read, argo_chars_read, c);
                return -1;
            }
            if(ungetc(c, f)==EOF){
                fprintf(stderr,"[%d, %d] Fail to unget. \n", argo_lines_read, argo_chars_read);
                return -1;
            }
            argo_chars_read--;
            int_sum = 0;
            dec_flag = 1;
        }

        else if( argo_is_exponent(c) && (!exp_flag)){
            if(argo_append_char(sv, c)){
                return -1;
            }
            c = fgetc(f);
            argo_chars_read++;
            if(!(argo_is_digit(c) || c == ARGO_PLUS || c == ARGO_MINUS)){
                fprintf(stderr, "[%d, %d] Digit expected in number but seen (%d)\n", argo_lines_read, argo_chars_read, c);
                return -1;
            }
            if(c == ARGO_PLUS){
                if(argo_append_char(sv, ARGO_PLUS)){
                    return -1;
                }
                c = fgetc(f);
                argo_chars_read++;
            }
            else if(c == ARGO_MINUS){
                if(argo_append_char(sv, ARGO_MINUS)){
                    return -1;
                }
                exp_neg = 1;
                c = fgetc(f);
                argo_chars_read++;
            }
            if(argo_is_digit(c)){
                if(ungetc(c, f)==EOF){
                    fprintf(stderr,"[%d, %d] Fail to unget. \n", argo_lines_read, argo_chars_read);
                    return -1;
                }
                argo_chars_read--;
            }
            else{
                fprintf(stderr, "[%d, %d] Digit expected in number but seen (%d)\n", argo_lines_read, argo_chars_read, c);
                return -1;
            }
            int_sum = 0;
            exp_flag = 1;
        }

        else{
            if(ungetc(c, f)==EOF){
                fprintf(stderr,"[%d, %d] Fail to unget. \n", argo_lines_read, argo_chars_read);
                return -1;
            }
            argo_chars_read--;
            break;
        }


        c = fgetc(f);
        argo_chars_read++;

    }

    if(neg_flag){
        int_sum = -int_sum;
        float_sum = -float_sum;
    }

    if(exp_flag){
        for(k=0; k<exp_sum; k++){
            if(exp_neg){
                float_sum = float_sum/10.0;
            }
            else{
                float_sum = float_sum*10.0;
            }
        }
    }

    char vals, vali, valf;

    if(exp_flag||dec_flag){
        vals = 1;
        vali = 0;
        valf = 1;
    }
    else{
        vals = 1;
        vali = 1;
        valf = 1;
    }

    n->int_value = int_sum;
    n->float_value = float_sum;
    n->valid_string = vals;
    n->valid_int = vali;
    n->valid_float = valf;

    return 0;
}

/**
 * @brief  Write canonical JSON representing a specified value to
 * a specified output stream.
 * @details  Write canonical JSON representing a specified value
 * to specified output stream.  See the assignment document for a
 * detailed discussion of the data structure and what is meant by
 * canonical JSON.
 *
 * @param v  Data structure representing a value.
 * @param f  Output stream to which JSON is to be written.
 * @return  Zero if the operation is completely successful,
 * nonzero if there is any error.
 */
int argo_write_value(ARGO_VALUE *v, FILE *f) {

    int p = global_options & 0x000000FF;

    if(v->type == ARGO_BASIC_TYPE){
        if((v->content).basic == ARGO_NULL){
            if(argo_write_basic(ARGO_NULL_TOKEN, f)){
                fprintf(stderr, "Error in write basic\n");
                return -1;
            }
        }
        if((v->content).basic == ARGO_TRUE){
            if(argo_write_basic(ARGO_TRUE_TOKEN, f)){
                fprintf(stderr, "Error in write basic\n");
                return -1;
            }
        }
        if((v->content).basic == ARGO_FALSE){
            if(argo_write_basic(ARGO_FALSE_TOKEN, f)){
                fprintf(stderr, "Error in write basic\n");
                return -1;
            }
        }
    }

    if(v->type == ARGO_NUMBER_TYPE){
        if(argo_write_number(&((v->content).number), f)){
            fprintf(stderr, "Error in write number\n");
            return -1;
        }
    }

    if(v->type == ARGO_STRING_TYPE){
        if(argo_write_string(&((v->content).string), f)){
            fprintf(stderr, "Error in write string\n");
            return -1;
        }
    }

    if(v->type == ARGO_OBJECT_TYPE){
        if(argo_write_object(&((v->content).object), f)){
            fprintf(stderr, "Error in write object\n");
            return -1;
        }
    }

    if(v->type == ARGO_ARRAY_TYPE){
        if(argo_write_array(&((v->content).array), f)){
            fprintf(stderr, "Error in write array\n");
            return -1;
        }
    }

    if(v->type == ARGO_NO_TYPE){
        fprintf(stderr, "Error no type to write\n");
        return -1;
    }

    if(p){
        if(indent_level == 0){
            if(fputc(ARGO_LF, f) == EOF){
                fprintf(stderr, "Error EOF\n");
                return -1;
            }
        }
    }

    return 0;
}


/**
 * @brief  Write canonical JSON representing a specified string
 * to a specified output stream.
 * @details  Write canonical JSON representing a specified string
 * to specified output stream.  See the assignment document for a
 * detailed discussion of the data structure and what is meant by
 * canonical JSON.  The argument string may contain any sequence of
 * Unicode code points and the output is a JSON string literal,
 * represented using only 8-bit bytes.  Therefore, any Unicode code
 * with a value greater than or equal to U+00FF cannot appear directly
 * in the output and must be represented by an escape sequence.
 * There are other requirements on the use of escape sequences;
 * see the assignment handout for details.
 *
 * @param v  Data structure representing a string (a sequence of
 * Unicode code points).
 * @param f  Output stream to which JSON is to be written.
 * @return  Zero if the operation is completely successful,
 * nonzero if there is any error.
 */
int argo_write_string(ARGO_STRING *s, FILE *f) {

    if( s==NULL || f==NULL ){
        fprintf(stderr, "Invalid argument(s) for write string\n");
        return -1;
    }

    size_t cap = s->capacity;
    size_t len = s->length;
    ARGO_CHAR *str = s->content;

    if(len > cap){
        fprintf(stderr, "Invalid argument(s) for write string\n");
        return -1;
    }

    if(fputc(ARGO_QUOTE, f) == EOF){
        fprintf(stderr, "Error EOF\n");
        return -1;
    }

    size_t i;
    ARGO_CHAR c;
    for(i = 0; i < len; i++, str++){
        c = *str;
        if(argo_is_control(c)){
            // '\b'
            if(c == ARGO_BS){
                if(fputc(ARGO_BSLASH, f) == EOF){
                    fprintf(stderr, "Error EOF\n");
                    return -1;
                }
                if(fputc(ARGO_B, f) == EOF){
                    fprintf(stderr, "Error EOF\n");
                    return -1;
                }
            }
            // '\t'
            else if(c == ARGO_HT){
                if(fputc(ARGO_BSLASH, f) == EOF){
                    fprintf(stderr, "Error EOF\n");
                    return -1;
                }
                if(fputc(ARGO_T, f) == EOF){
                    fprintf(stderr, "Error EOF\n");
                    return -1;
                }
            }
            // '\n'
            else if(c == ARGO_LF){
                if(fputc(ARGO_BSLASH, f) == EOF){
                    fprintf(stderr, "Error EOF\n");
                    return -1;
                }
                if(fputc(ARGO_N, f) == EOF){
                    fprintf(stderr, "Error EOF\n");
                    return -1;
                }
            }
            // '\f'
            else if(c == ARGO_FF){
                if(fputc(ARGO_BSLASH, f) == EOF){
                    fprintf(stderr, "Error EOF\n");
                    return -1;
                }
                if(fputc(ARGO_F, f) == EOF){
                    fprintf(stderr, "Error EOF\n");
                    return -1;
                }
            }
            // '\r'
            else if(c == ARGO_CR){
                if(fputc(ARGO_BSLASH, f) == EOF){
                    fprintf(stderr, "Error EOF\n");
                    return -1;
                }
                if(fputc(ARGO_R, f) == EOF){
                    fprintf(stderr, "Error EOF\n");
                    return -1;
                }
            }
            // "\u"
            else{
                if(fputc(ARGO_BSLASH, f) == EOF){
                    fprintf(stderr, "Error EOF\n");
                    return -1;
                }
                if(fputc(ARGO_U, f) == EOF){
                    fprintf(stderr, "Error EOF\n");
                    return -1;
                }

                // "xxxx hex num"
                if(write_hex_to_file(c, f)){
                    return -1;
                }
            }

        }
        else if(c > 0x001f && c <= 0x00ff){
            if(c == ARGO_BSLASH){
                if(fputc(ARGO_BSLASH, f) == EOF){
                    fprintf(stderr, "Error EOF\n");
                    return -1;
                }
                if(fputc(ARGO_BSLASH, f) == EOF){
                    fprintf(stderr, "Error EOF\n");
                    return -1;
                }
            }
            else if(c == ARGO_QUOTE){
                if(fputc(ARGO_BSLASH, f) == EOF){
                    fprintf(stderr, "Error EOF\n");
                    return -1;
                }
                if(fputc(ARGO_QUOTE, f) == EOF){
                    fprintf(stderr, "Error EOF\n");
                    return -1;
                }
            }
            else{
                if(fputc(c, f) == EOF){
                    fprintf(stderr, "Error EOF\n");
                    return -1;
                }
            }
        }
        else if(c > 0x00ff && c <= 0xffff){
            if(fputc(ARGO_BSLASH, f) == EOF){
                fprintf(stderr, "Error EOF\n");
                return -1;
            }
            if(fputc(ARGO_U, f) == EOF){
                fprintf(stderr, "Error EOF\n");
                return -1;
            }

            // "xxxx hex num"
            if(write_hex_to_file(c, f)){
                return -1;
            }
        }
        else{
            fprintf(stderr, "Invalid char in write string\n");
            return -1;
        }
    }
    if(fputc(ARGO_QUOTE, f) == EOF){
        fprintf(stderr, "Error EOF\n");
        return -1;
    }

    return 0;
}

/**
 * @brief  Write canonical JSON representing a specified number
 * to a specified output stream.
 * @details  Write canonical JSON representing a specified number
 * to specified output stream.  See the assignment document for a
 * detailed discussion of the data structure and what is meant by
 * canonical JSON.  The argument number may contain representations
 * of the number as any or all of: string conforming to the
 * specification for a JSON number (but not necessarily canonical),
 * integer value, or floating point value.  This function should
 * be able to work properly regardless of which subset of these
 * representations is present.
 *
 * @param v  Data structure representing a number.
 * @param f  Output stream to which JSON is to be written.
 * @return  Zero if the operation is completely successful,
 * nonzero if there is any error.
 */
int argo_write_number(ARGO_NUMBER *n, FILE *f) {

    if( n==NULL || f==NULL ){
        fprintf(stderr, "Invalid argument(s) for write number\n");
        return -1;
    }

    //ARGO_STRING sv = n->string_value;
    long iv = n->int_value;
    double fv = n->float_value;

    if(n->valid_int != 0){
        if(write_long_to_file(iv, f)){
            return -1;
        }
    }

    else if(n->valid_float != 0){
        if(write_double_to_file(fv, f)){
            return -1;
        }
    }

    else{
        fprintf(stderr, "Invalid argument(s) for write number\n");
        return -1;
    }

    return 0;
}
