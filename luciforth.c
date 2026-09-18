/* Luciforth - a satanic Forth in plain C
 * Based on the public-domain FORTH VM sources by Andreas Klimas
 * (klimas@w3group.de, 2017).
 */

#include <stdio.h>
#include <stdlib.h> // exit
#include <string.h> // strcmp
#include <ctype.h> // isspace

static void ok(void) { // print data stack, than prompt
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

int main() {
	puts("Luciforth - the Abattoir is open.");

	char *w;
	while((w=word())); // words are consumed; the interpreter follows
	return 0;
}
