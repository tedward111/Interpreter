#include "value.h"
#ifndef TOKENIZER_H
#define TOKENIZER_H

/* 
 * Takes in a character stream, returns a list of every tokenizable symbol
 * in the stream or gracefully exits if a non-tokenizable symbol is encountered.
 */
Value *tokenize();

/* 
 * Prints each found token to the terminal
 */
void displayTokens(Value *list);

#endif
