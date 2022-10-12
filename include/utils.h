#ifndef UTILS_H
#define UTILS_H

int compare_string(char *str1, char *str2);

int is_digit_string(char *str);

int string_to_int(char *str);

// read functions
#define argo_maybe_basic(c) ((c) == 't' || (c) == 'f' || (c) == 'n')

int argo_read_array(ARGO_ARRAY *a, FILE *f);

int argo_read_object(ARGO_OBJECT *o, FILE *f);

int argo_read_basic(ARGO_BASIC *b, FILE *f);

// write functions
int write_hex_to_file(int num, FILE *f);

int write_long_to_file(long num, FILE *f);

int write_double_to_file(double num, FILE *f);

int argo_write_basic(char *str, FILE *f);

int argo_write_array(ARGO_ARRAY *a, FILE *f);

int argo_write_object(ARGO_OBJECT *o, FILE *f);

#endif