#include "tokenizer.h"
#include "parser.h"
#include "interpreter.h"
#include "talloc.h"
#include "value.h"
#include "linkedlist.h"
#include <unistd.h>
#include <stdio.h>
#include <stdbool.h>

/* 
 * Returns true if the number of open and close parentheses match in an expression
 */
bool parenthesesMatch(Value *tokens) {
	int open = 0;
	int close = 0;
	while (tokens->type != NULL_TYPE) {
		if (car(tokens)->type == OPEN_TYPE) {
			open++;
		} else if (car(tokens)->type == CLOSE_TYPE) {
			close++;
		}
		tokens = cdr(tokens);
	}
	if (open == close) {
		return true;
	} else if (open < close) {
		printf("Syntax Error: too many close parentheses.\n");
        texit(1);
	}
	return false;
}

/* 
 * Takes in two linked lists, returns a list consisting of the contents of the
 * first list followed by the contents of the second list.
 */
 Value *joinList(Value *list1, Value *list2) {
	Value *toReturn = list1;
	while (cdr(list1)->type != NULL_TYPE) {
		list1 = cdr(list1);
	}
	list1->c.cdr = list2;
	return toReturn;
}

int main(void) {
	if (isatty(fileno(stdin)) == 1) {
		// We're in a terminal!
		printf("> ");
		char next = fgetc(stdin);
		while (next != EOF) {
			ungetc(next, stdin);
			Value *list = tokenize();
			// check if list has even parentheses. If not, continue.
			while (!parenthesesMatch(list)) {
				printf(". ");
				Value *moreTokens = tokenize();
				list = joinList(list, moreTokens);
			}
		    Value *tree = parse(list);
		    interpret(tree);
		    printf("> ");
		    next = fgetc(stdin);
		}
		tfree();
	} else {
		Value *list = tokenize();
	    Value *tree = parse(list);
	    interpret(tree);
	    tfree();
	    return 0;
	}
}
