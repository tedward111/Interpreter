#ifndef INTERPRETER_H
#define INTERPRETER_H
#include "value.h"

struct Frame {
    struct Value *bindings;
    struct Frame *parent;
};
typedef struct Frame Frame;

/*
 * Handles a list of S-expressions (ie a Scheme program), calls eval()
 * on each S-expression in the top-level (global) environment, and
 * prints each result in turn.
 */
void interpret(struct Value *tree);

/*
 * Takes a parse tree of a single S-exrpression and an environment frame in
 * which to evaluate the expression, then returns a pointer to a Value 
 * representing the evaluated value. 
 */
struct Value *eval(struct Value *expr, Frame *frame);

#endif
