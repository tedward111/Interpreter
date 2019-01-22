#ifndef VALUE_H
#define VALUE_H

#include "interpreter.h"

typedef enum {
   PTR_TYPE,
   INT_TYPE,
   DOUBLE_TYPE,
   STR_TYPE,
   CONS_TYPE,
   NULL_TYPE,
   OPEN_TYPE,
   CLOSE_TYPE,
   BOOL_TYPE,
   SYMBOL_TYPE,
   CLOSURE_TYPE,
   PRIMITIVE_TYPE,
   VOID_TYPE,
   QUOTE_TYPE,
} valueType;
struct Value {
   valueType type;
   union {
      void *p;
      int i;
      double d;
      char *s;
      struct ConsCell {
         struct Value *car;
         struct Value *cdr;
      } c;
      struct Closure {
         struct Value *parameters;
         struct Value *body;
         struct Frame *frame;
      } cl;
	  /* A pointer to a C implementation of a Scheme primitive function.
	   * Note: 'pf' is the variable name I chose for the function pointer.
	   */ 
	  struct Value *(*pf)(struct Value *);
   };
};

typedef struct Value Value;

#endif
