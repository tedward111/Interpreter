#include <stdbool.h>
#include "value.h"
#include "linkedlist.h"
#include "talloc.h"
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <string.h>

/*
 * Create an empty list (a new Value object of type NULL_TYPE).
 */
Value *makeNull() {
	Value *returnValue = talloc(sizeof(Value));
	returnValue->type = NULL_TYPE;
	return returnValue;
}

/*
 * Create a nonempty list (a new Value object of type CONS_TYPE).
 */
Value *cons(Value *car, Value *cdr) {
    assert(car != NULL);
    assert(cdr != NULL);
	Value *returnValue = talloc(sizeof(Value));
	returnValue->type = CONS_TYPE;
	returnValue->c.car = car;
	returnValue->c.cdr = cdr;
	return returnValue;
}

/*
 * Print a representation of the contents of a linked list.
 */
void displayHelper(Value *list, bool prefixWithSpace, bool expectingList) {
    assert(list != NULL);
    if (prefixWithSpace && (list->type != NULL_TYPE || !expectingList) && list->type != CONS_TYPE) {
        printf(" ");
    }
	if (list->type == INT_TYPE) {
        if (expectingList) {
            printf(". %i", list->i);
        } else {
            printf("%i",list->i);
        }
	}
	else if (list->type == DOUBLE_TYPE) {
        if (expectingList) {
            printf(". %f", list->d);
        } else {
		    printf("%f",list->d);
        }
	}
	else if (list->type == STR_TYPE || list->type == SYMBOL_TYPE) {
        if (expectingList) {
            printf(". %s", list->s);
        } else {
            printf("%s",list->s);
        }
	}
	else if (list->type == CONS_TYPE) {
        if (expectingList) {
        	displayHelper(list->c.car, true, false);
		    displayHelper(list->c.cdr, true, true);
        } else {
            printf("(");
            displayHelper(list->c.car, false, false);
            displayHelper(list->c.cdr, true, true);
            printf(")");
        }
	}
	else if (list->type == NULL_TYPE) {
		if (!expectingList) {
			printf("()");
		}
	} 
    else if (list->type == PTR_TYPE) {
        if (expectingList) {
            printf(". %p", list->p);
        } else {
            printf("%p", list->p);
        }
    }
}
void display(Value *list) {
	assert(list != NULL);
	//assert(list->type == CONS_TYPE || list->type == NULL_TYPE);
    if (list->type == NULL_TYPE) {
        printf("()\n");
    } else {
        displayHelper(list, false, true);
	   printf("\n");
    }
}

/*
 * Get the car value of a given list.
 * (Uses assertions to ensure that this is a legitimate operation.)
 */
Value *car(Value *list) {
	assert(list != NULL);
	assert(list->type == CONS_TYPE);
	return list->c.car;
}
/*
 * Get the cdr value of a given list.
 * (Uses assertions to ensure that this is a legitimate operation.)
 */
Value *cdr(Value *list) {
	assert(list != NULL);
	assert(list->type == CONS_TYPE);
	return list->c.cdr;
}
/*
 * Test if the given value is a NULL_TYPE value.
 * (Uses assertions to ensure that this is a legitimate operation.)
 */
bool isNull(Value *value) {
	assert(value != NULL);
	return value->type == NULL_TYPE;
}
/*
 * Compute the length of the given list.
 * (Uses assertions to ensure that this is a legitimate operation.)
 */
int length(Value *value) {
	assert(value != NULL);
	assert(value->type == CONS_TYPE || value->type == NULL_TYPE);
	if (value->type == NULL_TYPE) {
		return 0;
	}
	else {
		return 1+length(value->c.cdr);
	}
}
/*
 * Create a new linked list whose entries correspond to the given list's
 * entries, but in reverse order.  
 *
 * (Uses assertions to ensure that this is a legitimate operation.)
 */
Value *reverseHelper(Value *list, Value *soFar) {
    assert(list != NULL);
	if (list->type == NULL_TYPE) {
		return soFar;
	}
	soFar = cons(list->c.car, soFar);
	return reverseHelper(list->c.cdr, soFar);
}

Value *reverse(Value *list) {
    assert(list != NULL);
	assert(list->type == CONS_TYPE || list->type == NULL_TYPE);
	return reverseHelper(list, makeNull());	
}
