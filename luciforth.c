/* Luciforth - a satanic Forth in plain C
 * Based on the public-domain FORTH VM sources by Andreas Klimas
 * (klimas@w3group.de, 2017).
 */

#include <stdio.h>
#include <stdlib.h> // exit
#include <string.h> // strcmp
#include <ctype.h> // isspace

typedef struct xt_t { // Execution Token
	struct xt_t *next;
	char *name;
	void (*prim)(void);
	struct xt_t **data; // for variables and high level code (compiled words)
	int has_lit; // consumes the next cell as data (lit, branch, 0branch)
} xt_t;
typedef long long cell_t;
static xt_t *dictionary;
static xt_t *current_xt; // current xt
#define DATA_STACK_SIZE 32
static cell_t sp_base[DATA_STACK_SIZE], *sp_end=sp_base+DATA_STACK_SIZE;
static cell_t *sp=sp_base-1;

static xt_t *find(xt_t *dict, char *w) { // find word in a dictionary chain
	for(;dict;dict=dict->next) if(!strcmp(dict->name, w)) return dict;

	return 0; // not found
}

static void ok(void) { // print data stack, than prompt
	cell_t *i;
	for(i=sp_base;i<=sp;i++) printf("%lld ", *i);
	printf("666> ");
}
static int next_char(void) {
	static int last_char;
	if(last_char=='\n') ok(); // prompt
	last_char=fgetc(stdin);
	return last_char==EOF?0:last_char;
}
static int skip_space(void) {
	int ch;
	while((ch=next_char()) && isspace(ch));
	return ch;
}
static char *word(void) { // symbol might have maximal 256 bytes
	static char buffer[256], *end=buffer+sizeof(buffer)-1;
	char *p=buffer, ch;
	if(!(ch=skip_space())) return 0; // no more input
	*p++=ch;
	while(p<end && (ch=next_char()) && !isspace(ch)) *p++=ch;
	*p=0; // zero terminated string
	return buffer;
}

static void terminate(char *msg) {
	fprintf(stderr, "terminated: %s\n", msg);
	exit(1);
}
static void sp_push(cell_t value) {
	if(sp==sp_end) terminate("Data stack overflow");
	*++sp=value;
}
static cell_t sp_pop(void) {
	if(sp<sp_base) terminate("Data stack underrun");
	return *sp--;
}

static xt_t *add_word(char *name, void (*prim)(void)) {
	xt_t *xt=calloc(1, sizeof(xt_t));
	xt->next=dictionary;
	dictionary=xt;
	xt->name=strdup(name);
	xt->prim=prim;
	return xt;
}
static void f_words(void) { // display all defined words
	xt_t *w;
	for(w=dictionary;w;w=w->next) printf("%s ", w->name);
	printf("\n");
}
static void register_primitives(void) {
	add_word("grimoire", f_words); // list all defined words
}

static void interpret(char *w) {
	if((current_xt=find(dictionary, w))) {
		current_xt->prim();
	} else { // not found, may be a number
		char *end;
		cell_t number=strtol(w, &end, 0);
		if(*end) terminate("word not found");
		else sp_push(number);
	}
}

int main() {
	puts("Luciforth - the Abattoir is open.");
	register_primitives();

	char *w;
	while((w=word())) interpret(w);
	return 0;
}
