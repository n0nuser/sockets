/*
** Fichero: utils.h
** Autores:
** Sergio García González
** Pablo Jesús González Rubio
*/

void printChars(char buf[]);
void list(char *content,char *ficheroGroup, FILE *g);
int group (char *content, char * ficheroGroup, FILE *g, char * grupo);
int article(char *content, char *pathArticulos, FILE *a, char * articulo, char * grupo);
int head(char *content, char *pathArticulos, FILE *a, char * articulo, char * grupo);
int body(char *content, char *pathArticulos, FILE *a, char * articulo, char * grupo);
void newgroups(char *content, char *ficheroGroup, FILE *g, char *date, char *time);
void newnews(char *content, char *ficheroGroup, char *pathArticulos, FILE *g, char * grupo, char *date, char *time);
int post(int socket, char *mensajeOriginal, char *ficheroGroup, char *pathArticulos, FILE *g);