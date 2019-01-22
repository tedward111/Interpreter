#include "tokenizer.h"
#include "talloc.h"
#include <stdio.h>

int main(void) {
	Value *list = tokenize();
	displayTokens(list);
	tfree();
	return 0;
}