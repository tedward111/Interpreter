#include <stdlib.h>
#include "value.h"
#include "talloc.h"
#include <stdio.h>
#include <stdbool.h>
#include "linkedlist.h"

bool debugGC = false;
/*
 * Print an error message indicating the program is out of memory.
 */
void outOfMemoryError() {
    printf("Out of memory!");
    texit(1);
}
/*
 * Create an empty list (a new Value object of type NULL_TYPE).
 */
Value *makeNullTalloc() {
	Value *returnValue = malloc(sizeof(Value));
    if (returnValue == NULL) {
        outOfMemoryError();
    }
	returnValue->type = NULL_TYPE;
	return returnValue;
}

/*
 * Create a nonempty list (a new Value object of type CONS_TYPE).
 */
Value *consTalloc(Value *car, Value *cdr) {
	Value *returnValue = malloc(sizeof(Value));
    if (returnValue == NULL) {
        outOfMemoryError();
    }
	returnValue->type = CONS_TYPE;
	returnValue->c.car = car;
	returnValue->c.cdr = cdr;
	return returnValue;
}

/*
 * The head of our list's memory.
 */
static Value *head;

/*
 * A malloc-like function that allocates memory, tracking all allocated
 * pointers in the "active list."  
 */
void *talloc(size_t size) {
	if (head == NULL) {
		head = makeNullTalloc();
	} 
	Value *memoryRecord = malloc(sizeof(Value));
    if (memoryRecord == NULL) {
        outOfMemoryError();
    }
	memoryRecord->type = PTR_TYPE;
	memoryRecord->p = malloc(size);
	head = consTalloc(memoryRecord, head);
	return memoryRecord->p;
}
Value *getReachableInTree(Value *toAddTo, Value *tree);
Value *getReachableInFrame(Value *toAddTo, Frame *frame) {
    while (frame != NULL) {
        toAddTo = getReachableInTree(toAddTo, frame->bindings);
        Value *newFrameRecord = makeNullTalloc();
        newFrameRecord->type = PTR_TYPE;
        newFrameRecord->p = frame;
        toAddTo = consTalloc(newFrameRecord, toAddTo);
        frame = frame->parent;
    }
    return toAddTo;
}
bool containsSamePointer(Value *toCheck, Value *list) {
    while (list->type == CONS_TYPE) {
        if (toCheck->p == list->c.car->p) {
            return true;
        }
        list = list->c.cdr;
    }
    return false;
}

Value *getReachableInTree(Value *toAddTo, Value *tree) {
    Value *newRecord = makeNullTalloc();
    newRecord->type = PTR_TYPE;
    newRecord->p = tree;
    //Avoid doubly recursing
    if (containsSamePointer(newRecord, toAddTo)) {
        //won't be a part of our end list, so we need to free it now
        free(newRecord);
        return toAddTo;
    }
    toAddTo = consTalloc(newRecord, toAddTo);
    if (tree->type == CONS_TYPE) {
        toAddTo = getReachableInTree(toAddTo, tree->c.car);
        toAddTo = getReachableInTree(toAddTo, tree->c.cdr);
    } else if (tree->type == STR_TYPE || tree->type == SYMBOL_TYPE || tree->type == BOOL_TYPE) {
        Value *newStringRecord = makeNullTalloc();
        newStringRecord->type = PTR_TYPE;
        newStringRecord->p = tree->s;
        toAddTo = consTalloc(newStringRecord, toAddTo);
    } else if (tree->type == CLOSURE_TYPE) {
        toAddTo = getReachableInTree(toAddTo, tree->cl.parameters);
        toAddTo = getReachableInTree(toAddTo, tree->cl.body);
        toAddTo = getReachableInFrame(toAddTo, tree->cl.frame); 
    }
    return toAddTo;
}
/*
 * Sweep through all pointers in the given tree and frame, freeing unreachable ones
 */
void sweep(Value *tree, Frame *frame) {
    if (debugGC) {
        printf("Beginning GC..\n");
    }
    Value *reachable = makeNullTalloc();
    reachable = getReachableInTree(reachable, tree);
    reachable = getReachableInFrame(reachable, frame);
    //we now have all the reachable pointers in a list, so we will crossreference
    //this list with our list of allocated pointers
    if (debugGC) {
        printf("Found reachable elements, removing unreachable.\n");
    }
    Value *remaining = head;
    Value *last = NULL;
    int freed = 0;
    int notFreed = 0;
    int total = 0;
    while (remaining->type == CONS_TYPE) {
        if (!containsSamePointer(remaining->c.car, reachable)) {
            //remove this entry
            freed++;
            free(remaining->c.car->p);
            free(remaining->c.car);
            Value *tmp = remaining;
            if (last == NULL) {
                //we're still at the head of the list
                head = remaining->c.cdr;
            } else {
                last->c.cdr = remaining->c.cdr;
            }
            remaining = remaining->c.cdr;
            free(tmp);
        } else {
            notFreed++;
            last = remaining;
            remaining = remaining->c.cdr;
        }
        total++;
    }
    if (debugGC) {
        printf("%i freed, %i remaining out of %i\n", freed, notFreed, total);
    }
    //tear down the temporary list we used to keep track of the pointers
    //re-use remaining
    remaining = reachable;
    while (remaining->type == CONS_TYPE) {
        free(remaining->c.car);
        Value *tmp = remaining;
        remaining = remaining->c.cdr;
        free(tmp);
    }
    free(remaining); //remove last null in list
}
/*
 * Free all pointers allocated by talloc, as well as whatever memory you
 * malloc'ed to create/update the active list.
 */
void tfree() {
	while (head->type == CONS_TYPE) {
		free(head->c.car->p);		
		free(head->c.car);
		Value *tmp = head;
		head = head->c.cdr;
		free(tmp);
	}
	free(head);
	head = NULL;
}

/*
 * A simple two-line function to stand in the C function "exit", which calls
 * tfree() and then exit().  
 */
void texit(int status) {
	tfree();
	exit(status);
}
