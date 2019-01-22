#include "tokenizer.h"
#include "value.h"
#include "talloc.h"
#include <stdio.h>
#include "parser.h"
#include "linkedlist.h"

int main(void) {
    Value *list = tokenize();
    Value *tree = parse(list);
    printTree(tree);
    printf("\n");
    tfree();
    return 0;
}