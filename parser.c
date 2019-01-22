#include "tokenizer.h"
#include "value.h"
#include "linkedlist.h"
#include "talloc.h"
#include <stdio.h>
#include <assert.h>
#include <string.h>



/*
 * Prints a simple error trace and exits the program upon discovering unparsable input
 */
void raiseParseError(char* message, bool condition) {
	if (!condition) {
		printf("Syntax error: %s\n", message);
		texit(1);
	}
}

/*
 * Adds a symbol to the parse tree, encapsulating symbols contained within parentheses
 * in sub-lists.
 */
Value *addToParseTree(Value *tree, int *depth, Value *token) {
	if (token->type == CLOSE_TYPE) {
		// recurse up the level
		(*depth)--;
		raiseParseError("too many close parentheses.", (*depth) >= 0);
		Value *tmpList = makeNull();
		Value *top = car(tree);
		// Pop off the stack
		while (top->type != OPEN_TYPE) {
			tmpList = cons(top, tmpList);
			tree = cdr(tree);
			top = car(tree);
		}
		// skip past open type we just read
		tree = cdr(tree); 
        if (tree->type != NULL_TYPE && car(tree)->type == QUOTE_TYPE) {
            //if the tree is not null, we need to check for a quote
            tree = cdr(tree); //skip past the quote
            Value *quoteExpansion = makeNull();
            quoteExpansion = cons(tmpList, quoteExpansion);
            Value *quoteSymbol = makeNull();
            quoteSymbol->type = SYMBOL_TYPE;
            quoteSymbol->s = talloc(sizeof(char) * 6);
            strcpy(quoteSymbol->s, "quote");
            tmpList = cons(quoteSymbol, quoteExpansion);

        }
		// Push tmpList onto the tree
		tree = cons(tmpList, tree);
	} else if (token->type == OPEN_TYPE) {
		// push to tree
		tree = cons(token, tree);
		// increment depth
		(*depth)++;
    } else {
		// continue push to tree, checking for quotes
        if (tree->type != NULL_TYPE && car(tree)->type == QUOTE_TYPE) {
            //skip past the quote
            tree = cdr(tree);
            Value *quoteExpansion = makeNull();
            quoteExpansion = cons(token, quoteExpansion);
            Value *quoteSymbol = makeNull();
             quoteSymbol->type = SYMBOL_TYPE;
            quoteSymbol->s = talloc(sizeof(char) * 6);
            strcpy(quoteSymbol->s, "quote");
            quoteExpansion = cons(quoteSymbol, quoteExpansion);
            tree = cons(quoteExpansion, tree);
        } else {
            tree = cons(token, tree);
        }
	}
	return tree;
}
/*
 * Takes a linked list of tokens as a parameter.
 * Returns the pointer to a parse tree representing the program.
 */
Value *parse(Value *tokens) {
	Value *tree = makeNull();
    int depth = 0;
	
	Value *current = tokens;
	assert(current != NULL && "Error (parse): null pointer");
	while (current->type != NULL_TYPE) {
        Value *token = car(current);
        tree = addToParseTree(tree, &depth, token);
        current = cdr(current);
    }
    raiseParseError("not enough close parentheses.", depth == 0);
	return reverse(tree);
}
/*
 * Recursively prints a parse tree
 */
void printTreeHelper(Value *tree, bool prefixWithSpace, bool expectingList, bool atBeginning) {
    assert(tree != NULL);
    if (prefixWithSpace && !atBeginning && (tree->type != NULL_TYPE || !expectingList) && tree->type != CONS_TYPE) {
        printf(" ");
    }
	if (tree->type == INT_TYPE) {
        if (expectingList) {
            printf(". %i", tree->i);
        } else {
            printf("%i",tree->i);
        }
	}
	else if (tree->type == DOUBLE_TYPE) {
        if (expectingList) {
            printf(". %f", tree->d);
        } else {
		    printf("%f",tree->d);
        }
	}
	else if (tree->type == STR_TYPE) {
        if (expectingList) {
            printf(". \"%s\"", tree->s);
        } else {
            printf("\"%s\"",tree->s);
        }
	}
    else if (tree->type == SYMBOL_TYPE) {
        if (expectingList) {
            printf(". %s", tree->s);
        } else {
            printf("%s",tree->s);
        }
	} 
	else if (tree->type == BOOL_TYPE) {
		if (expectingList) {
            printf(". %s", tree->s);
        } else {
            printf("%s",tree->s);
        }
	}
	else if (tree->type == CONS_TYPE) {
        if (expectingList) {
            if(atBeginning) {
                printTreeHelper(tree->c.car, true, false, true);
                printTreeHelper(tree->c.cdr, true, true, false);
            }
            else {
                printTreeHelper(tree->c.car, true, false, false);
                printTreeHelper(tree->c.cdr, true, true, false);
            }
        } else {
            if (atBeginning) {
                printf("(");
            } else {
                printf(" (");
            }
            printTreeHelper(tree->c.car, false, false, false);
            printTreeHelper(tree->c.cdr, true, true, false);
            printf(")");
        }
	}
	else if (tree->type == NULL_TYPE) {
		if (!expectingList) {
			printf("()");
		}
	}
    else if (tree->type == CLOSURE_TYPE) {
        if (expectingList) {
            printf(". #procedure");
        } else {
            printf("#procedure");
        }
    }
    else if (tree->type == PRIMITIVE_TYPE) {
        if (expectingList) {
            printf(". #procedure");
        } else {
            printf("#procedure");
        }
    }
    else if (tree->type == QUOTE_TYPE) {
        printf("'");
    }
}

/*
 * Prints a parse tree to the command line, using parentheses
 * to denote tree structure (ie it looks like scheme code)
 */
void printTree(Value *tree) {
    assert(tree != NULL);
    if (tree->type == NULL_TYPE) {
        printf("()");
    } else {
        printTreeHelper(tree, false, tree->type == CONS_TYPE, true);
    }
}
