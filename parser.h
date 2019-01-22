#include "value.h"
#ifndef PARSER_H
#define PARSER_H

/*
 * Takes a linked list of tokens as a parameter.
 * Returns the pointer to a parse tree representing the program.
 */
Value *parse(Value *tokens);

/*
 * Prints a parse tree to the command line, using parentheses
 * to denote tree structure (ie it looks like scheme code)
 */
void printTree(Value *tree);

#endif