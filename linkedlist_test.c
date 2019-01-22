#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "value.h"
#include "linkedlist.h"

int main(void) {
    Value *val1 = malloc(sizeof(Value));
    val1->type = INT_TYPE;
    val1->i = 23;

    Value *val2 = malloc(sizeof(Value));
    val2->type = STR_TYPE;
    val2->s = malloc(10 * sizeof(char));
    strcpy(val2->s, "tofu");

    Value *head = makeNull();
    head = cons(val1, head);
    head = cons(val2, head);
	
    display(head);
    printf("Length = %i\n", length(head));
    printf("Empty? %i\n", isNull(head));
    Value *reversed = reverse(head);
    display(reversed);


    Value *val3 = malloc(sizeof(Value));
    val3->type = DOUBLE_TYPE;
    val3->d = 23.2;

    Value *val4 = malloc(sizeof(Value));
    val4->type = DOUBLE_TYPE;
    val4->d = 54.2;

    Value *val5 = malloc(sizeof(Value));
    val5->type = INT_TYPE;
    val5->i = 44;

    Value *consCell = cons(val4, val5);
    Value *head2 = cons(val3, consCell);
    display(head2);
    head2 = cons(head, head2);
    display(head2);
    
    Value *emptyList = makeNull();
    display(emptyList);
    
    return 0;
}
