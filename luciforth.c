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
static xt_t *macros; // compile time dictionary
static xt_t **definitions=&dictionary; // where to store new words
static xt_t **ip; // instruction pointer
#define RETURN_STACK_SIZE 32
static xt_t **rp_base[RETURN_STACK_SIZE], ***rp_end=rp_base+RETURN_STACK_SIZE;
static xt_t ***rp=rp_base-1;
static xt_t *current_xt; // current xt
static xt_t *xt_dup, *xt_drop, *xt_interpreting, *xt_word, *xt_bye, *xt_lit, *xt_leave, *xt_0branch, *xt_branch; // execution tokens needed for compiling
#define DATA_STACK_SIZE 32
static cell_t sp_base[DATA_STACK_SIZE], *sp_end=sp_base+DATA_STACK_SIZE;
static cell_t *sp=sp_base-1;
static int is_compile_mode; // we are either interpreting or compiling
#define CODE_SIZE 65536
static xt_t *code_base[CODE_SIZE], **code=code_base, **code_end=code_base+CODE_SIZE;

static xt_t *compile(xt_t *xt);
static void interpreting(char *w);

static xt_t *find(xt_t *dict, char *w) { // find word in a dictionary chain
	for(;dict;dict=dict->next) if(!strcmp(dict->name, w)) return dict;

	return 0; // not found
}

static void ok(void) { // print data stack, than prompt
	cell_t *i;
	for(i=sp_base;i<=sp;i++) printf("%lld ", *i);
	printf(is_compile_mode?"compile> ":"666> ");
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
	if(ch=='"') { // string handling
		while(p<end && (ch=next_char()) && ch!='"') *p++=ch;
	} else {
		while(p<end && (ch=next_char()) && !isspace(ch)) *p++=ch;
	}
	*p=0; // zero terminated string
	return buffer;
}

static void terminate(char *msg) {
	fprintf(stderr, "terminated: %s\n", msg);
	exit(1);
}
static void rp_push(xt_t **ip) {
	if(rp==rp_end) terminate("return stack overflow");
	*++rp=ip;
}
static xt_t **rp_pop(void) {
	if(rp<rp_base) terminate("return stack underrun");
	return *rp--;
}
static void sp_push(cell_t value) {
	if(sp==sp_end) terminate("Data stack overflow");
	*++sp=value;
}
static cell_t sp_pop(void) {
	if(sp<sp_base) terminate("Data stack underrun");
	return *sp--;
}

static void f_mul(void) { cell_t v1=sp_pop(); *sp*=v1; } // TOS * NOS => TOS
static void f_add(void) { cell_t v1=sp_pop(); *sp+=v1; } // TOS + NOS => TOS
static void f_hail(void) {
	printf("HAIL LUCIFER\n");
}

static xt_t *add_word(char *name, void (*prim)(void)) {
	xt_t *xt=calloc(1, sizeof(xt_t));
	xt->next=*definitions;
	*definitions=xt;
	xt->name=strdup(name);
	xt->prim=prim;
	xt->data=code; // current high level code pointer, compilation target
	return xt;
}
static void f_drop(void) {sp_pop();} // discard top of stack
static void f_words(void) { // display all defined words
	xt_t *w;
	for(w=dictionary;w;w=w->next) printf("%s ", w->name);
	printf("\n");
}
static void f_type(void){ // print string at addr
	fputs((void*)sp_pop(), stdout);
}
static void f_cr(void){ // newline
	fputc('\n', stdout);
}
static void f_docol(void) { // VM: enter function (word)
	rp_push(ip); // at runtime push current ip on return stack
	ip=current_xt->data; // and continue at the high level code
}
static void f_colon(void) { // define a new word
	char *w=word(); // read next word which becomes the word name
	add_word(strdup(w), f_docol);
	is_compile_mode=1; // switch to compile mode
}
static void f_semis(void) { // macro, end of definition
	compile(xt_leave); // compile return from subroutine
	is_compile_mode=0; // switch back to interpret mode
}
static void f_leave(void) { // return from subroutine
	ip=rp_pop();
}
static void f_lit(void) {
	sp_push((cell_t)*ip++);
}
static void f_dot(void) { // output number
	printf("%lld ", sp_pop());
}
static void f_bye(void) { // close the Abattoir politely
	puts("The Abattoir falls silent.");
	exit(0);
}
static void f_branch(void) { ip=(void*)*ip; } // unconditional jump
static void f_0branch(void) { // jump if top of stack is zero
	if(sp_pop()) ip++;
	else         ip=(void*)*ip;
}
static void f_word(void) { sp_push((cell_t)word()); }
static void f_interpreting(void) {
	interpreting((void*)sp_pop());
}
static void f_dup(void){cell_t t=*sp; sp_push(t);}
static void register_primitives(void) {
	add_word("+", f_add);
	add_word("*", f_mul);
	add_word("hail", f_hail);
	xt_drop=add_word("sacrifice", f_drop);
	xt_dup=add_word("dup", f_dup);
	add_word("grimoire", f_words);
	add_word("type", f_type); // output string
	add_word(".", f_dot);
	add_word("cr", f_cr);
	add_word(":", f_colon); // define new word, enter compile mode
	xt_bye=add_word("bye", f_bye);
	xt_leave=add_word("leave", f_leave);
	xt_lit=add_word("lit", f_lit);
	xt_0branch=add_word("0branch", f_0branch);
	xt_branch=add_word("branch", f_branch);
	xt_word=add_word("word", f_word);
	xt_interpreting=add_word("interpreting", f_interpreting);

	definitions=&macros;
	add_word(";", f_semis); // end of new word, leave compile mode

	definitions=&dictionary;
}
static char *to_pad(char *str) { // copy str into the scratch pad
	static char scratch[1024];
	int len=strlen(str);
	if(len>sizeof(scratch)-1) len=sizeof(scratch)-1;
	memcpy(scratch, str, len);
	scratch[len]=0; // zero byte at string end
	return scratch;
}
static xt_t *compile(xt_t *xt) {
	if(code>=code_end) terminate("code space full");
	return *code++=xt;
}
static void literal(cell_t value) {
	compile(xt_lit); // call f_lit when executed
	*code++=(xt_t*)value;
}
static void compiling(char *w) {
	if(*w=='"') { // string handling
		literal((cell_t)strdup(w+1)); // compile a string literal
	} else if((current_xt=find(macros, w))) { // if word is a macro execute it immediatly
		current_xt->prim();
	} else if((current_xt=find(dictionary, w))) { // if word is in regular dictionary, compile it
		*code++=current_xt;
	} else { // not found, may be a number
		char *end;
		cell_t number=strtol(w, &end, 0);
		if(*end) terminate("word not found");
		else literal(number); // compile a number literal
	}
}

static void interpreting(char *w) {
	if(is_compile_mode) return compiling(w);

	if(*w=='"') { // string handling
		sp_push((cell_t)to_pad(w+1));
	} else if((current_xt=find(dictionary, w))) {
		current_xt->prim();
	} else { // not found, may be a number
		char *end;
		cell_t number=strtol(w, &end, 0);
		if(*end) terminate("word not found");
		else sp_push(number);
	}
}
static void vm(void) {
	for(;;) {
		current_xt=*ip++;
		current_xt->prim();
	}
}
int main() {
	puts("Luciforth - the Abattoir is open.");
	register_primitives();

	/* the interpreter is compiled by hand */
	add_word("shell", f_docol); // define a new high level word
	xt_t **begin=code;       // save current code pointer for loop back
	compile(xt_word);        // get the next word on data stack
	compile(xt_dup);
	compile(xt_0branch);     // jump to end if top of stack is null
	xt_t **here=code++;      // forward jump reference
	compile(xt_interpreting);// interpret the word on top of stack
	compile(xt_branch);      // loop back to begin of this word
	*code++=(void*)begin;    // Loop back address
	*here=(void*)code;       // resolve reference
	*code++=xt_drop;
	*code++=xt_bye;          // leave VM

	ip=begin;                // set instruction pointer
	vm();                    // and run the vm

	return 0;
}
