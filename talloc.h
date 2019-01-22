#include <stdlib.h>
#include "value.h"
#include "interpreter.h"

#ifndef TALLOC_H
#define TALLOC_H

/*
 * A malloc-like function that allocates memory, tracking all allocated
 * pointers in the "active list."  (You can choose your implementation of the
 * active list, but whatever it is, your talloc code should NOT call functions
 * in linkedlist.h; instead, implement any list-like behavior directly here.
 * Otherwise you'll end up with circular dependencies, since you're going to
 * modify the linked list to use talloc instead of malloc.)
 */
void *talloc(size_t size);

/*
 * Free all pointers allocated by talloc, as well as whatever memory you
 * malloc'ed to create/update the active list.
 */
void tfree();

/*
 * A simple two-line function to stand in the C function "exit", which calls
 * tfree() and then exit().  (You'll use this later to allow a clean exit from
 * your interpreter when you encounter an error: so memory can be automatically
 * cleaned up when exiting.)
 */
void texit(int status);

/*
 * A function that sweeps through a frame and a value to find all reachable
 * elements, and garbage collects all other unreachable elements allocated
 * through talloc.
 */
void sweep(Value *tree, Frame *frame);
#endif
