#include <stdlib.h>
#include <stdio.h>

#include "argo.h"
#include "global.h"
#include "debug.h"
#include "utils.h"

int compare_string(char *str1, char *str2){
	int len1=0, len2=0;
	char *s;
	/*
	*	Find the length of s1
	*/
    for(s=str1; *s!='\0'; s++){
        len1++;
    }

    /*
	*	Find the length of s2
	*/
    for(s=str2; *s!='\0'; s++){
        len2++;
    }

    /*
	*	Unequal if lengths are different.
	*/
    if(len1!=len2){
    	return 0;
    }

    while(*str1!='\0' && *str2!='\0'){
    	if(*str1 != *str2){
    		return 0;
    	}
    	str1++;
    	str2++;
    }
    return 1;
}

int is_digit_string(char *str){
	char *s;
	for(s=str; *s; s++){
		if(*s<'0' || *s>'9'){
			return 0;
		}
	}
	return 1;
}

int string_to_int(char *str){
	int sum=0;
	char c;
	char *s;
	for(s=str; *s; s++){
		c=*s;
		sum=sum*10 + (c-48);
	}
	return sum;
}

// argo read helper functions
int argo_read_array(ARGO_ARRAY *a, FILE *f){

	ARGO_CHAR c = fgetc(f);
	argo_chars_read++;

	if(c != ARGO_LBRACK){
		fprintf(stderr, "[%d, %d] Expect '[' in array but seen (%d)\n", argo_lines_read, argo_chars_read, c);
		return -1;
	}


	c = fgetc(f);
	argo_chars_read++;

	ARGO_VALUE *prev_value = NULL;
	ARGO_VALUE *new_value = NULL;

	ARGO_VALUE *head = argo_value_storage + argo_next_value;
	argo_next_value++;

	head->type = ARGO_NO_TYPE;
	head->next = head;
	head->prev = head;
	head->name.capacity = 0;
	head->name.length = 0;
	head->name.content = NULL;

	a->element_list = head;

	prev_value = head;

	int has_comma = 0;

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
		else if(c == ARGO_RBRACK){
			if(has_comma){
				fprintf(stderr, "[%d, %d] Expect Value but seen (%d)\n", argo_lines_read, argo_chars_read, c);
				return -1;
			}
			return 0;
		}
		else if(c == ARGO_COMMA){
			if(prev_value == head || has_comma){
				fprintf(stderr, "[%d, %d] Expect Value but seen (%d)\n", argo_lines_read, argo_chars_read, c);
				return -1;
			}
			else{
				has_comma = 1;
				new_value = argo_read_value(f);
				if(new_value == NULL){
					return -1;
				}
				else{
					if(prev_value == head){
						prev_value->prev = new_value;
					}
					prev_value->next = new_value;
					new_value->next = head;
					new_value->prev = prev_value;
					prev_value = new_value;
					has_comma = 0;
				}
			}
		}
		else{
			if(!(prev_value == head || has_comma)){
				fprintf(stderr, "[%d, %d] Expect , but seen (%d)\n", argo_lines_read, argo_chars_read, c);
				return -1;
			}
			if(ungetc(c,f) == EOF){
				fprintf(stderr,"[%d, %d] Fail to unget. \n", argo_lines_read, argo_chars_read);
                return -1;
            }
            argo_chars_read--;
			new_value = argo_read_value(f);
			if(new_value == NULL){
				return -1;
			}
			else{
				if(prev_value == head){
					prev_value->prev = new_value;
				}
				prev_value->next = new_value;
				new_value->next = head;
				new_value->prev = prev_value;
				prev_value = new_value;
				has_comma = 0;
			}
		}
		c = fgetc(f);
		argo_chars_read++;

	}

	fprintf(stderr, "[%d, %d] Expect ']' in array but seen (%d)\n", argo_lines_read, argo_chars_read, c);
	return -1;
}

int argo_read_object(ARGO_OBJECT *o, FILE *f){

	ARGO_CHAR c = fgetc(f);
	argo_chars_read++;

	if(c != ARGO_LBRACE){
		fprintf(stderr, "[%d, %d] Expect '{' in object but seen (%d)\n", argo_lines_read, argo_chars_read, c);
		return -1;
	}


	c = fgetc(f);
	argo_chars_read++;

	ARGO_VALUE *head = argo_value_storage + argo_next_value;
	argo_next_value++;
	head->type = ARGO_NO_TYPE;
	head->next = head;
	head->prev = head;
	head->name.capacity = 0;
	head->name.length = 0;
	head->name.content = NULL;

	o->member_list = head;

	ARGO_VALUE *prev_value = head;
	ARGO_VALUE *new_value = NULL;
	int has_name = 0;
	int has_comma = 0;

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
		else if(c == ARGO_RBRACE){
			if(has_comma){
				fprintf(stderr, "[%d, %d] Expect member in object but seen (%d)\n", argo_lines_read, argo_chars_read, c);
				return -1;
			}
			if(has_name){
				fprintf(stderr, "[%d, %d] Expect : in object but seen (%d)\n", argo_lines_read, argo_chars_read, c);
				return -1;
			}
			head->name.capacity = 0;
			head->name.length = 0;
			head->name.content = NULL;
			has_name = 0;
			return 0;
		}

		else if(c == ARGO_QUOTE){
			if(!(has_comma||prev_value == head)){
				fprintf(stderr,"[%d, %d] Expect , in object but seen (%d)\n", argo_lines_read, argo_chars_read, c);
	             return -1;
			}
			else if(!has_name){
				if(ungetc(c,f) == EOF){
					fprintf(stderr,"[%d, %d] Fail to unget. \n", argo_lines_read, argo_chars_read);
	                return -1;
	            }
	            argo_chars_read--;
				if(argo_read_string(&(head->name), f)){
					return -1;
				}
				has_name = 1;
				has_comma = 0;
			}
			else{
				fprintf(stderr, "[%d, %d] Expect : in object but seen (%d)\n", argo_lines_read, argo_chars_read, c);
				return -1;
			}
		}
		else if(c == ARGO_COLON){
			if(has_name){
				new_value = argo_read_value(f);
				if(new_value == NULL){
					return -1;
				}
				else{
					new_value->name.capacity = head->name.capacity;
					new_value->name.length = head->name.length;
					new_value->name.content = head->name.content;
					head->name.capacity = 0;
					head->name.length = 0;
					head->name.content = NULL;
					has_name = 0;
					if(prev_value == head){
						prev_value->prev = new_value;
					}
					prev_value->next = new_value;
					new_value->next = head;
					new_value->prev = prev_value;
					prev_value = new_value;
				}
			}
			else{
				fprintf(stderr, "[%d, %d] Expect name in object but seen (%d)\n", argo_lines_read, argo_chars_read, c);
				return -1;
			}
		}
		else if(c == ARGO_COMMA){
			if(prev_value == head || has_comma){
				fprintf(stderr, "[%d, %d] Expect member in object but seen (%d)\n", argo_lines_read, argo_chars_read, c);
				return -1;
			}
			else{
				has_comma = 1;
			}
		}
		else{
			fprintf(stderr, "[%d, %d] Invalid object (%d)\n", argo_lines_read, argo_chars_read, c);
			return -1;
		}

		c = fgetc(f);
		argo_chars_read++;

	}

	fprintf(stderr, "[%d, %d] Expect '}' in object but seen (%d)\n", argo_lines_read, argo_chars_read, c);
	return -1;
}

int argo_read_basic(ARGO_BASIC *b, FILE *f){
	ARGO_CHAR c = fgetc(f);
	argo_chars_read++;
	if(c == 't'){
		c = fgetc(f);
		argo_chars_read++;
		if(c == 'r'){
			c = fgetc(f);
			argo_chars_read++;
			if(c == 'u'){
				c = fgetc(f);
				argo_chars_read++;
				if(c == 'e'){
					*b = ARGO_TRUE;
					return 0;
				}
				else{
					fprintf(stderr, "[%d, %d] Invalid Token\n", argo_lines_read, argo_chars_read);
					return -1;
				}
			}
			else{
				fprintf(stderr, "[%d, %d] Invalid Token\n", argo_lines_read, argo_chars_read);
				return -1;
			}
		}
		else{
			fprintf(stderr, "[%d, %d] Invalid Token\n", argo_lines_read, argo_chars_read);
			return -1;
		}
	}
	else if(c == 'f'){
		c = fgetc(f);
		argo_chars_read++;
		if(c == 'a'){
			c = fgetc(f);
			argo_chars_read++;
			if(c == 'l'){
				c = fgetc(f);
				argo_chars_read++;
				if(c == 's'){
					c = fgetc(f);
					argo_chars_read++;
					if(c == 'e'){
						*b = ARGO_FALSE;
						return 0;
					}
					else{
						fprintf(stderr, "[%d, %d] Invalid Token\n", argo_lines_read, argo_chars_read);
						return -1;
					}
				}
				else{
					fprintf(stderr, "[%d, %d] Invalid Token\n", argo_lines_read, argo_chars_read);
					return -1;
				}
			}
			else{
				fprintf(stderr, "[%d, %d] Invalid Token\n", argo_lines_read, argo_chars_read);
				return -1;
			}
		}
		else{
			fprintf(stderr, "[%d, %d] Invalid Token\n", argo_lines_read, argo_chars_read);
			return -1;
		}
	}
	else if(c == 'n'){
		c = fgetc(f);
		argo_chars_read++;
		if(c == 'u'){
			c = fgetc(f);
			argo_chars_read++;
			if(c == 'l'){
				c = fgetc(f);
				argo_chars_read++;
				if(c == 'l'){
					*b = ARGO_NULL;
					return 0;
				}
				else{
					fprintf(stderr, "[%d, %d] Invalid Token\n", argo_lines_read, argo_chars_read);
					return -1;
				}
			}
			else{
				fprintf(stderr, "[%d, %d] Invalid Token\n", argo_lines_read, argo_chars_read);
				return -1;
			}
		}
		else{
			fprintf(stderr, "[%d, %d] Invalid Token\n", argo_lines_read, argo_chars_read);
			return -1;
		}
	}
	else{
		fprintf(stderr, "[%d, %d] Invalid Token\n", argo_lines_read, argo_chars_read);
		return -1;
	}
}


// argo write helper functions
int write_hex_to_file(int num, FILE *f){
	if(num < 0){
		fprintf(stderr, "Invalid Hex\n");
		return -1;
	}

	int digit;
	// loop to write four hex digit
	int k;
	for(k=3; k>=0; k--){
		// shift right 12, 8, 4, 0 bits and mask to obtain digit on that position
		digit = (num>>(k*4)) & 0x000F;

		// put digit char
        if((digit)>=0 && (digit)<10){
			if(fputc(digit+ARGO_DIGIT0,f) == EOF){
				fprintf(stderr, "Error EOF\n");
				return -1;
			}
		}

		// put hex a b c d e f char
		else if((digit)>=10 && (digit)<16){
			if(fputc((digit-15)+ARGO_F,f) == EOF){
				fprintf(stderr, "Error EOF\n");
				return -1;
			}
		}
		else{
			fprintf(stderr, "Invalid Hex\n");
			return -1;
		}
    }

	return 0;
}

int write_long_to_file(long num, FILE *f){
	if(num < 0){
		if(fputc(ARGO_MINUS,f) == EOF){
			fprintf(stderr, "Error EOF\n");
			return -1;
		}
		num = -num;
	}

	//recursion until first digit
	if(num/10){
		if(write_long_to_file((num/10), f)){
			return -1;
		}
	}

	if((num%10)>=0 && (num%10)<10){
		if(fputc((num%10)+ARGO_DIGIT0,f) == EOF){
			fprintf(stderr, "Error EOF\n");
			return -1;
		}
	}
	else{
		fprintf(stderr, "Invalid integer to write\n");
		return -1;
	}
	return 0;
}

int write_double_to_file(double num, FILE *f){
	if(num == 0){
		if(fputc(ARGO_DIGIT0,f) == EOF){
			fprintf(stderr, "Error EOF\n");
			return -1;
		}
		if(fputc(ARGO_PERIOD,f) == EOF){
			fprintf(stderr, "Error EOF\n");
			return -1;
		}
		if(fputc(ARGO_DIGIT0,f) == EOF){
			fprintf(stderr, "Error EOF\n");
			return -1;
		}
		return 0;
	}

	if(num < 0){
		if(fputc(ARGO_MINUS,f) == EOF){
			fprintf(stderr, "Error EOF\n");
			return -1;
		}
		num = -num;
	}

	long exp = 0;
	while(exp <= 1023 && exp >= -1022){
		if(num >= 1){
			num = num/10;
			exp++;
		}
		else if(num < 0.1){
			num = num * 10;
			exp--;
		}
		else{
			break;
		}
	}

	if(num >= 1 || num < 0.1 || exp > 1023 || exp < -1022){
		fprintf(stderr, "Invalid float number to write\n");
		return -1;
	}

	//0.
	if(fputc(ARGO_DIGIT0,f) == EOF){
		fprintf(stderr, "Error EOF\n");
		return -1;
	}
	if(fputc(ARGO_PERIOD,f) == EOF){
		fprintf(stderr, "Error EOF\n");
		return -1;
	}

	int r;
	int i;
	for(i=0; i<=ARGO_PRECISION; i++){
		num = num * 10;
		r = (int) num;
		num = num - r;
		if(fputc(r+ARGO_DIGIT0,f) == EOF){
			fprintf(stderr, "Error EOF\n");
			return -1;
		}
		if(num == 0){
			break;
		}
	}

	if(exp != 0){
		if(fputc(ARGO_E,f) == EOF){
			fprintf(stderr, "Error EOF\n");
			return -1;
		}
		if(write_long_to_file(exp, f)){
			return -1;
		}
	}

	return 0;
}

int argo_write_basic(char *str, FILE *f){

    if( str==NULL || f==NULL ){
    	fprintf(stderr, "Invalid argument(s) for write basic\n");
        return -1;
    }

    char *s;
    for(s=str; *s; s++){
    	if(fputc(*s, f) == EOF){
    		fprintf(stderr, "Error EOF\n");
	        return -1;
	    }
    }

   return 0;
}

int argo_write_array(ARGO_ARRAY *a, FILE *f){

    if( a==NULL || f==NULL ){
    	fprintf(stderr, "Invalid argument(s) for write array\n");
        return -1;
    }

    int p = global_options & 0x000000FF;
    int i;

    // head
    ARGO_VALUE *list_ptr = a->element_list;
    // first element
    list_ptr = list_ptr->next;

    // put '['
    if(fputc(ARGO_LBRACK, f) == EOF){
    	fprintf(stderr, "Error EOF\n");
        return -1;
    }

    indent_level++;

    // if empty, then indent level decreased by 1
    if(list_ptr->type == ARGO_NO_TYPE){
    	indent_level--;
    }

    if(p){
        if(fputc(ARGO_LF, f) == EOF){
        	fprintf(stderr, "Error EOF\n");
            return -1;
        }
        for(i=0; i<(p*indent_level); i++){
            if(fputc(ARGO_SPACE, f) == EOF){
            	fprintf(stderr, "Error EOF\n");
                return -1;
            }
        }
    }

    // iterate through the circular linked list
    while(list_ptr->type){
        if(argo_write_value(list_ptr, f)){
            return -1;
        }
        if(list_ptr->next->type){
            if(fputc(ARGO_COMMA, f) == EOF){
            	fprintf(stderr, "Error EOF\n");
                return -1;
            }
        }
        else{
            indent_level--;
        }
        if(p){
            if(fputc(ARGO_LF, f) == EOF){
            	fprintf(stderr, "Error EOF\n");
                return -1;
            }
            for(i=0; i<(p*indent_level); i++){
                if(fputc(ARGO_SPACE, f) == EOF){
                	fprintf(stderr, "Error EOF\n");
                    return -1;
                }
            }
        }
        list_ptr = list_ptr->next;
    }
    if(fputc(ARGO_RBRACK, f) == EOF){
    	fprintf(stderr, "Error EOF\n");
        return -1;
    }

    return 0;
}

int argo_write_object(ARGO_OBJECT *o, FILE *f){

    if( o==NULL || f==NULL ){
    	fprintf(stderr, "Invalid argument(s) for write object\n");
        return -1;
    }

    int p = global_options & 0x000000FF;
    int i;

    ARGO_VALUE *list_ptr = o->member_list;

    list_ptr = list_ptr->next;
    if(fputc(ARGO_LBRACE, f) == EOF){
    	fprintf(stderr, "Error EOF\n");
        return -1;
    }
    indent_level++;
    // if empty, then indent level decreased by 1
    if(list_ptr->type == ARGO_NO_TYPE){
    	indent_level--;
    }
    if(p){
        if(fputc(ARGO_LF, f) == EOF){
        	fprintf(stderr, "Error EOF\n");
            return -1;
        }
        for(i=0; i<(p*indent_level); i++){
            if(fputc(ARGO_SPACE, f) == EOF){
            	fprintf(stderr, "Error EOF\n");
                return -1;
            }
        }
    }
    while(list_ptr->type){
        if(argo_write_string(&(list_ptr->name), f)){
            return -1;
        }
        if(fputc(ARGO_COLON, f) == EOF){
        	fprintf(stderr, "Error EOF\n");
            return -1;
        }
        if(p){
            if(fputc(ARGO_SPACE, f) == EOF){
            	fprintf(stderr, "Error EOF\n");
                return -1;
            }
        }
        if(argo_write_value(list_ptr, f)){
            return -1;
        }
        if(list_ptr->next->type){
            if(fputc(ARGO_COMMA, f) == EOF){
            	fprintf(stderr, "Error EOF\n");
                return -1;
            }
        }
        else{
            indent_level--;
        }
        if(p){
            if(fputc(ARGO_LF, f) == EOF){
            	fprintf(stderr, "Error EOF\n");
                return -1;
            }
            for(i=0; i<(p*indent_level); i++){
                if(fputc(ARGO_SPACE, f) == EOF){
                	fprintf(stderr, "Error EOF\n");
                    return -1;
                }
            }
        }
        list_ptr = list_ptr->next;
    }
    if(fputc(ARGO_RBRACE, f) == EOF){
    	fprintf(stderr, "Error EOF\n");
        return -1;
    }

    return 0;
}
