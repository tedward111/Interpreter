#include "parser.h"
#include "tokenizer.h"
#include "linkedlist.h"
#include "value.h"
#include "talloc.h"
#include <stdio.h>
#include <string.h>
#include "interpreter.h"
#include <stdbool.h>

// Top frame. Global variable for interpreter accessibility.
Frame * top;
// Flag that toggles whether using the else operator will throw an error
bool inCond = false;

/*
 * Prints a simple error trace and exits the program upon discovering unparsable input
 */
void raiseEvalError(char* message, bool condition) {
	if (condition) {
		printf("Evaluation Error: %s\n", message);
		texit(1);
	}
}

/*
 * Mutates an inputted BOOL_TYPE value to represent 'true'
 */
void makeTrue(Value *boolean) {
    boolean->s = talloc(3*sizeof(char));
    strcpy(boolean->s, "#t");
}

/*
 * Mutates an inputted BOOL_TYPE value to represent 'false'
 */
void makeFalse(Value *boolean) {
    boolean->s = talloc(3*sizeof(char));
    strcpy(boolean->s, "#f");
}

/*
 * Prints a given value.
 */
void printValue(Value *value) {
	if (value->type == CONS_TYPE) {
		printf("(");
		printTree(value);
		printf(")\n");
	} else if (value->type != VOID_TYPE) {
		printTree(value);
		printf("\n");
	}
}

/*
 * Returns the value associated with a symbol
 */
Value *lookUpSymbol(Value *symbol, Frame *frame) {
	// Traverse the lookup tree in frame
	Value *remaining = frame->bindings;
	while (remaining->type != NULL_TYPE) {
		Value *pair = car(remaining);
		if (!strcmp(car(pair)->s, symbol->s)) {
			//values are same
			return cdr(pair); //no car since this is a cons pair, not a list
		}
		remaining = cdr(remaining);
	}
	if (frame->parent == NULL) {
		char* errorString = talloc(sizeof(char) * 32);
		sprintf(errorString, "Undefined symbol '%s'", symbol->s);
		raiseEvalError(errorString, true);
	}
	return lookUpSymbol(symbol, frame->parent);
}

/* 
 * Binds a symbol name to a primitive function
 */
void bind(char *name, Value *(*function)(Value *), Frame *frame) {
	Value *value = makeNull();
	value->type = PRIMITIVE_TYPE;
	value->pf = function;
	Value *symbol = makeNull();
	symbol->type = SYMBOL_TYPE;
	symbol->s = talloc((strlen(name) + 1) * sizeof(char));
    strcpy(symbol->s, name);
	Value *binding = cons(symbol, value);
	frame->bindings = cons(binding, frame->bindings);
}

/*
 * Evaluates if statements
 */ 
Value *evalIf(Value *args, Frame *frame) {
	raiseEvalError("Expected at least 2 arguments, recieved 0", args->type == NULL_TYPE);
	Value *condition = car(args);
	raiseEvalError("Expected at least 2 arguments, recieved 0", condition->type == NULL_TYPE);
	raiseEvalError("Expected at least 2 arguments, recieved 1", cdr(args)->type == NULL_TYPE);
	Value *evalCondition = eval(condition, frame);
	if (evalCondition->type == BOOL_TYPE && !(strcmp(evalCondition->s, "#f"))) {
		if (cdr(cdr(args))->type == NULL_TYPE) {
			Value *returnValue = makeNull();
            returnValue->type = VOID_TYPE;
            return returnValue;
		} else {
			return eval(car(cdr(cdr(args))), frame);
		}
	} else {
		return eval(car(cdr(args)), frame);
	}
}

/*
 * Checks for duplicate parameters in flat lambda expression
 */
bool checkDuplicates(Value *parameters) {
    Value *remainingToCheck = parameters;
    while (remainingToCheck->type != NULL_TYPE) {
        Value *remainingToCheckAgainst = cdr(remainingToCheck);
        raiseEvalError("Non-symbol as parameter name", car(remainingToCheck)->type != SYMBOL_TYPE);
        while (remainingToCheckAgainst->type != NULL_TYPE) {
            raiseEvalError("Non-symbol as parameter name", car(remainingToCheckAgainst)->type != SYMBOL_TYPE);
            if (strcmp(car(remainingToCheck)->s, car(remainingToCheckAgainst)->s) == 0) {
                return true;
            }
            remainingToCheckAgainst = cdr(remainingToCheckAgainst);
        }
        remainingToCheck = cdr(remainingToCheck);
    }
    return false;
}

/*
 * Evaluates lambda statements
 */
Value *evalLambda(Value *args, Frame *frame) {
    raiseEvalError("Expected at least 2 arguments, recieved 0", args->type == NULL_TYPE);
    Value *newProcedure = makeNull();
    newProcedure->type = CLOSURE_TYPE;
    Value *parameters = car(args);
    raiseEvalError("Expected at least 2 arguments, recieved 1", cdr(args)->type == NULL_TYPE);
    Value *body = car(cdr(args));
    raiseEvalError("Parameters should be list or symbol", parameters->type != CONS_TYPE && parameters->type != SYMBOL_TYPE && parameters->type != NULL_TYPE);
    raiseEvalError("Duplicate argument names", parameters->type == CONS_TYPE && checkDuplicates(parameters));
    newProcedure->cl.parameters = parameters;
    newProcedure->cl.body = body;
    newProcedure->cl.frame = frame;
    return newProcedure;
}

/* 
 * Evaluates quote statements
 */
Value *evalQuote(Value *args, Frame *frame) {
	raiseEvalError("Expected at least 1 argument, recieved 0", args->type == NULL_TYPE);
	return car(args);
}

/*
 * Checks whether a given symbol is already bound within a set of bindings
 */ 
bool alreadyBound(Value *symbol, Value* bindings) {
    while (bindings->type != NULL_TYPE) {
        if (strcmp(car(car(bindings))->s, symbol->s) == 0) {
            return true;
        }
        bindings = cdr(bindings);
    }
    return false;
}

/*
 * Evaluates let statements
 */
Value *evalLet(Value *args, Frame *frame) {
	raiseEvalError("Expected at least 2 arguments, recieved 0", args->type == NULL_TYPE);
	Frame *childFrame = talloc(sizeof(Frame));
	childFrame->parent = frame;
	childFrame->bindings = makeNull();
	Value *remaining = car(args);
	while (remaining->type != NULL_TYPE) {
		Value *pair = car(remaining);
		Value *varSymbol = car(pair);
		raiseEvalError("Assignment to non-symbol value", varSymbol->type != SYMBOL_TYPE);
		raiseEvalError("No value to assign", cdr(pair)->type == NULL_TYPE);
        raiseEvalError("Duplicate symbol in let", alreadyBound(varSymbol, childFrame->bindings));
		Value *varValue = eval(car(cdr(pair)), frame);
		Value *varPair = cons(varSymbol, varValue);
		childFrame->bindings = cons(varPair, childFrame->bindings);
		remaining = cdr(remaining);
	}
	Value *toReturn;
	remaining = cdr(args);
	raiseEvalError("let: bad syntax (missing binding pairs or body)", remaining->type == NULL_TYPE);
	while (true) {
		toReturn = eval(car(remaining), childFrame);
		remaining = cdr(remaining);
		if (remaining->type == NULL_TYPE) {
			break;
		}
	}
	return toReturn;
}

/* 
 * Applies a procedure to arguments.
 */
Value *apply(Value *function, Value *args) {
    raiseEvalError("Applying non-procedure", function->type != CLOSURE_TYPE && function->type != PRIMITIVE_TYPE);
	if (function->type == CLOSURE_TYPE) {
		Frame *childFrame = talloc(sizeof(Frame));
		childFrame->parent = function->cl.frame;
		childFrame->bindings = makeNull();
		Value *argLabels = function->cl.parameters;
        if (argLabels->type == CONS_TYPE || argLabels->type == NULL_TYPE) {
            Value *body = function->cl.body;
            Value *labelRemaining = argLabels;
            Value *valueRemaining = args;
            while (labelRemaining->type != NULL_TYPE && valueRemaining->type != NULL_TYPE) {
                raiseEvalError("Attempting to bind to non-symbol", car(labelRemaining)->type != SYMBOL_TYPE);
                Value *bindingPair = cons(car(labelRemaining), car(valueRemaining));
                childFrame->bindings = cons(bindingPair, childFrame->bindings);
                labelRemaining = cdr(labelRemaining);
                valueRemaining = cdr(valueRemaining);
            }
            raiseEvalError("Argument-parameter mismatch!", labelRemaining->type != NULL_TYPE || valueRemaining->type != NULL_TYPE);
            return eval(body, childFrame);
        } else {
            raiseEvalError("Expected symbol as variadic parameter label", argLabels->type != SYMBOL_TYPE);
            Value *body = function->cl.body;
            Value *bindingPair = cons(argLabels, args);
            childFrame->bindings = cons(bindingPair, childFrame->bindings);
            return eval(body, childFrame);
        }
	} else {
		return (function->pf)(args);
	} 
}

/*
 * Evaluates define statements
 */
Value *evalDefine(Value *args) {
    raiseEvalError("Expected at least 2 arguments, recieved 0", args->type == NULL_TYPE);
    Value *label = car(args);
    raiseEvalError("Assignment to non-symbol value", label->type != SYMBOL_TYPE);
    Value *value = eval(car(cdr(args)), top);
    Value *varPair = cons(label, value);
    top->bindings = cons(varPair, top->bindings);
    Value *returnValue = makeNull();
    returnValue->type = VOID_TYPE;
    return returnValue;
}

/* 
 * Evaluates let* statements
 */
Value *evalLetStar(Value *args, Frame *frame) {
	raiseEvalError("Expected at least 2 arguments, recieved 0", args->type == NULL_TYPE);
	Frame *childFrame = talloc(sizeof(Frame));
	childFrame->parent = frame;
	childFrame->bindings = makeNull();
	Value *totalBindings = makeNull();
	Value *remaining = car(args);
	while (remaining->type != NULL_TYPE) {
		Value *pair = car(remaining);
		Value *varSymbol = car(pair);
		raiseEvalError("Assignment to non-symbol value", varSymbol->type != SYMBOL_TYPE);
		raiseEvalError("No value to assign", cdr(pair)->type == NULL_TYPE);
        raiseEvalError("Duplicate symbol in let*", alreadyBound(varSymbol, totalBindings));
		Value *varValue = eval(car(cdr(pair)), childFrame);
		Value *varPair = cons(varSymbol, varValue);
		childFrame->bindings = cons(varPair, childFrame->bindings);
		totalBindings = cons(varPair, totalBindings);
		Frame *tmpFrame = childFrame;
		childFrame = talloc(sizeof(Frame));
		childFrame->parent = tmpFrame;
		childFrame->bindings = makeNull();
		remaining = cdr(remaining);
	}
	Value *toReturn;
	remaining = cdr(args);
	raiseEvalError("let*: bad syntax (missing binding pairs or body)", remaining->type == NULL_TYPE);
	while (true) {
		toReturn = eval(car(remaining), childFrame);
		remaining = cdr(remaining);
		if (remaining->type == NULL_TYPE) {
			break;
		}
	}
	return toReturn;
}

/* 
 * Evaluates letrec statements
 */
Value *evalLetrec(Value *args, Frame *frame) {
	raiseEvalError("Expected at least 2 arguments, recieved 0", args->type == NULL_TYPE);
	Frame *childFrame = talloc(sizeof(Frame));
	childFrame->parent = frame;
	childFrame->bindings = makeNull();
	Value *remaining = car(args);
	Value *parameters = makeNull();
	while (remaining->type != NULL_TYPE) {
		Value *pair = car(remaining);
		Value *varSymbol = car(pair);
		raiseEvalError("Assignment to non-symbol value", varSymbol->type != SYMBOL_TYPE);
		raiseEvalError("No value to assign", cdr(pair)->type == NULL_TYPE);
        raiseEvalError("Duplicate symbol in let", alreadyBound(varSymbol, childFrame->bindings));
		Value *varValue = car(cdr(pair)); // Don't evaluate yet!
		Value *varPair = cons(varSymbol, makeNull());
		parameters = cons(varValue, parameters);
		childFrame->bindings = cons(varPair, childFrame->bindings);
		remaining = cdr(remaining);
	}
	remaining = childFrame->bindings;
	while (parameters->type != NULL_TYPE) {
		Value *varBinding = car(remaining);
		Value *varValue = eval(car(parameters), childFrame);
		varBinding->c.cdr = varValue;
		remaining = cdr(remaining);
		parameters = cdr(parameters);
	}
	Value *toReturn;
	remaining = cdr(args);
	raiseEvalError("letrec: bad syntax (missing binding pairs or body)", remaining->type == NULL_TYPE);
	while (true) {
		toReturn = eval(car(remaining), childFrame);
		remaining = cdr(remaining);
		if (remaining->type == NULL_TYPE) {
			break;
		}
	}
	return toReturn;
}

/*
 * Evaluates set! statements
 */
Value *evalSet(Value *args, Frame *frame) {
	raiseEvalError("Expected 2 arguments, recieved 0", args->type == NULL_TYPE);
	raiseEvalError("Expected 2 arguments, recieved 1", cdr(args)->type == NULL_TYPE);
	raiseEvalError("Expected 2 arguments, recieved more", cdr(cdr(args))->type != NULL_TYPE);
	// Traverse the lookup tree in frame
	Value *remaining = frame->bindings;
	while (remaining->type != NULL_TYPE) {
		Value *pair = car(remaining);
		if (!strcmp(car(args)->s, car(pair)->s)) {
			pair->c.cdr = eval(car(cdr(args)), frame);
			Value* toReturn = makeNull();
			toReturn->type = VOID_TYPE;
			return toReturn;
		}
		remaining = cdr(remaining);
	}
	if (frame->parent == NULL) {
		char* errorString = talloc(sizeof(char) * 32);
		sprintf(errorString, "Cannot set! undefined symbol '%s'", car(args)->s);
		raiseEvalError(errorString, true);
	}
	return evalSet(args, frame->parent);
}

/* 
 * Evaluates and statements
 */
Value *evalAnd(Value *args, Frame *frame) {
	raiseEvalError("Expected 2 arguments, recieved 0", args->type == NULL_TYPE);
	raiseEvalError("Expected 2 arguments, recieved 1", cdr(args)->type == NULL_TYPE);
	raiseEvalError("Expected 2 arguments, recieved more", cdr(cdr(args))->type != NULL_TYPE);
	Value* arg1 = eval(car(args), frame);
	Value* arg2 = eval(car(cdr(args)), frame);
	if (arg1->type == BOOL_TYPE && !strcmp(arg1->s, "#f")) {
		// arg1 is false!
		return arg1;
	} else {
		return arg2;
	}
}

/* 
 * Evaluates or statements
 */
Value *evalOr(Value *args, Frame *frame) {
	raiseEvalError("Expected 2 arguments, recieved 0", args->type == NULL_TYPE);
	raiseEvalError("Expected 2 arguments, recieved 1", cdr(args)->type == NULL_TYPE);
	raiseEvalError("Expected 2 arguments, recieved more", cdr(cdr(args))->type != NULL_TYPE);
	Value* arg1 = eval(car(args), frame);
	Value* arg2 = eval(car(cdr(args)), frame);
	if (arg1->type == BOOL_TYPE && !strcmp(arg1->s, "#f")) {
		return arg2;
	} else {
		return arg1;
	}
}

/*
 * Evaluates begin statements
 */ 
Value *evalBegin(Value *args, Frame *frame) {
	Value *toReturn = makeNull();
	while (args->type != NULL_TYPE) {
		toReturn = eval(car(args), frame);
		args = cdr(args);
	}
	return toReturn;
}

/*
 * Evaluates cond statements
 */ 
Value *evalCond(Value *args, Frame *frame) {
	while (args->type != NULL_TYPE) {
		inCond = true;
		Value* statement = car(args);
		raiseEvalError("Expected cons type in cond", statement->type != CONS_TYPE);
		if (cdr(statement)-> type == NULL_TYPE) {
			inCond = false;
			return eval(car(statement), frame);
		} else {
			Value* test = eval(car(statement), frame);
			if (!(test->type == BOOL_TYPE && !strcmp(test->s, "#f")) || (test->type == SYMBOL_TYPE && !strcmp(test->s, "else"))) {
				inCond = false;
				return eval(car(cdr(statement)), frame);
			}
		}
		args = cdr(args);
	}
	// No valid options, return void type.
	Value *toReturn = makeNull();
	toReturn->type = VOID_TYPE;
	inCond = false;
	return toReturn;
}

/* 
 * Binds a special form to the top level environment as a symbol
 */
Value *makeSpecialForm(char *form) {
	Value *symbol = makeNull();
	symbol->type = SYMBOL_TYPE;
	symbol->s = talloc((strlen(form) + 1) * sizeof(char));
    strcpy(symbol->s, form);
	return cons(symbol, symbol);
}

/*
 * Implements the variatic add procedure
 */ 
Value *primitiveAdd(Value* args) {
	Value *remaining = args;
	Value *count = makeNull();
	bool isInt = true;
	int resultI = 0;
	double resultD = 0;
	while (remaining->type != NULL_TYPE) {
		Value *summand = car(remaining);
		raiseEvalError("Expected number in add", summand->type != INT_TYPE && summand->type != DOUBLE_TYPE);
		if (summand->type == DOUBLE_TYPE) {
			isInt = false;
			resultD += summand->d;
		} else {
			resultD += summand->i;
			resultI += summand->i;
		}
		remaining = cdr(remaining);
	}
	if (isInt) {
		count->type = INT_TYPE;
		count->i = resultI;
	} else {
		count->type = DOUBLE_TYPE;
		count->d = resultD;
	}
	return count;
}

/*
 * Evaluates modulo statements
 */
Value *primitiveModulo(Value *args) {
    Value *toReturn = makeNull();
    raiseEvalError("Expected 2 arguments, recieved 0", args->type == NULL_TYPE);
    raiseEvalError("Expected 2 arguments, recieved 1", cdr(args)->type == NULL_TYPE);
    raiseEvalError("Expected 2 arguments, recieved more", cdr(cdr(args))->type != NULL_TYPE);
    raiseEvalError("Expected integer", car(args)->type != INT_TYPE);
    raiseEvalError("Expected integer", car(cdr(args))->type != INT_TYPE);
    toReturn->type = INT_TYPE;
    int modded = car(args)->i % car(cdr(args))->i;
    if (modded < 0 && car(cdr(args))->i > 0) {
        modded = modded + car(cdr(args))->i;
    }
    toReturn->i = modded;
    return toReturn;
}

/*
 * Evaluates zero? statements
 */
Value *primitiveZero(Value *args) {
    Value *toReturn = makeNull();
    raiseEvalError("Expected 1 argument, recieved 0", args->type == NULL_TYPE);
    raiseEvalError("Expected 1 argument, recieved more", cdr(args)->type != NULL_TYPE);
    Value *toCheck = car(args);
    toReturn->type = BOOL_TYPE;
    if (toCheck->type == INT_TYPE) {
        if (toCheck->i == 0) {
            makeTrue(toReturn);
        } else {
            makeFalse(toReturn);
        }
    } else if (toCheck->type == DOUBLE_TYPE) {
        if (toCheck->d == 0.0) {
            makeTrue(toReturn);
        } else {
            makeFalse(toReturn);
        }
    } else {
        raiseEvalError("Expected int or double, recieved neither.", true); 
    }
    return toReturn;
}

/*
 * Helper function for equal? that checks if two trees are equal
 */
bool treesAreEqual(Value *tree1, Value *tree2) {
    if (tree1->type != tree2->type) {
        return false;
    } else {
        switch(tree1->type) {
            case NULL_TYPE: return true;
            case BOOL_TYPE: return strcmp(tree1->s, tree2->s) == 0;
            case INT_TYPE: return tree1->i == tree2->i;
            case DOUBLE_TYPE: return tree1->d == tree2->i;
            case SYMBOL_TYPE: return strcmp(tree1->s, tree2->s) == 0;
            case STR_TYPE: return strcmp(tree1->s, tree2->s) == 0;
            case CONS_TYPE: return treesAreEqual(car(tree1), car(tree2)) && treesAreEqual(cdr(tree1), cdr(tree2));
            case CLOSURE_TYPE: return tree1 == tree2;
            case PTR_TYPE: return tree1->p == tree2->p;
            case OPEN_TYPE: return true;
            case CLOSE_TYPE: return true;
            case VOID_TYPE: return true;
            case PRIMITIVE_TYPE: return tree1->pf == tree2->pf;
            case QUOTE_TYPE: return true;
        }
    }
    return true;
}

/*
 * Evaluates equal? statements
 */
Value *primitiveEqual(Value *args) {
    raiseEvalError("Expected 2 arguments, recieved 0", args->type == NULL_TYPE);
    raiseEvalError("Expected 2 arguments, recieved 1", cdr(args)->type == NULL_TYPE);
    raiseEvalError("Expected 2 arguments, recieved more", cdr(cdr(args))->type != NULL_TYPE);
    Value *toReturn = makeNull();
    toReturn->type = BOOL_TYPE;
    if (treesAreEqual(car(args), car(cdr(args)))) {
        makeTrue(toReturn);
    } else {
        makeFalse(toReturn);
    }
    return toReturn;
}

/*
 * Evaluates list statements
 */
Value *primitiveList(Value *args) {
    //look at this incredible code
    return args;
}

/*
 * Finds the end of a list (the last non-null cell)
 */
Value *findListEnd(Value *list) {
    raiseEvalError("Internal error calling findEnd on null list", list->type == NULL_TYPE);
    while (list->type == CONS_TYPE && cdr(list)->type != NULL_TYPE) {
        list = cdr(list);
    }
    return list;
}

/*
 * Evaluates append statements
 */
Value *primitiveAppend(Value *args) {
    Value *toReturn = makeNull();
    Value *end = toReturn;
    while (args->type != NULL_TYPE) {
        Value *toAdd = car(args);
        if (toAdd->type != NULL_TYPE) {
            if (end->type == NULL_TYPE) {
                toReturn = toAdd;
                end = findListEnd(toReturn);
            } else if (end->type == CONS_TYPE) {
                end->c.cdr = toAdd;
                end = findListEnd(toAdd);
            } else {
                raiseEvalError("Contract violation, expected pair.", true);
            }
        }
        args = cdr(args);
    }
    return toReturn;
}

/*
 * Evaluates = statements
 */
Value *primitiveEqualsSign(Value *args) {
	raiseEvalError("Expected at least 2 arguments, got 0", args->type == NULL_TYPE);
	raiseEvalError("Expected at least 2 arguments, got 1", cdr(args)->type == NULL_TYPE);
    Value *toReturn = makeNull();
    toReturn->type = BOOL_TYPE;
    bool equals = true;
    Value *last = NULL;
    while (args->type != NULL_TYPE) {
        Value *now = car(args);
        if (last != NULL) {
            raiseEvalError("Expected number", now->type != INT_TYPE && now->type != DOUBLE_TYPE);
            equals = ((now->type == INT_TYPE) ? now->i : now->d) == ((last->type == INT_TYPE) ? last->i : last->d);
        }
        args = cdr(args);
        last = now;
    }
    if (equals) {
        makeTrue(toReturn);
    } else {
        makeFalse(toReturn);
    }
    return toReturn;
}

/*
 * Implements the primitive cons procedure
 */ 
Value *primitiveCons(Value *args) {
	raiseEvalError("Expected 2 arguments, got 0", args->type == NULL_TYPE);
	raiseEvalError("Expected 2 arguments, got 1", cdr(args)->type == NULL_TYPE);
	raiseEvalError("Expected 2 arguments, got more than 2", cdr(cdr(args))->type != NULL_TYPE);
	return cons(car(args), car(cdr(args)));
}

/* 
 * Implements the primitive car procedure
 */
Value *primitiveCar(Value *args) {
	// FLINTSTONES
	raiseEvalError("Expected 1 argument, got 0", args->type == NULL_TYPE);
	raiseEvalError("Expected cons type", car(args)->type != CONS_TYPE);
	raiseEvalError("Expected 1 argument, got more", cdr(args)->type != NULL_TYPE);
	return car(car(args));
}

/*
 * Implements the primitive cdr procedure
 */ 
Value *primitiveCdr(Value *args) {
	raiseEvalError("Expected 1 argument, got 0", args->type == NULL_TYPE);
	raiseEvalError("Expected cons type", car(args)->type != CONS_TYPE);
	raiseEvalError("Expected 1 argument, got more", cdr(args)->type != NULL_TYPE);
	return cdr(car(args));
}

/* 
 * Implements the primitive null? procedure
 */
Value *primitiveNull(Value *args) {
	raiseEvalError("Expected 1 argument, got 0", args->type == NULL_TYPE);
	raiseEvalError("Expected 1 argument, got more", cdr(args)->type != NULL_TYPE);
	Value* returnValue = makeNull();
	returnValue->type = BOOL_TYPE;
	if (car(args)->type == NULL_TYPE) {
        makeTrue(returnValue);
	} else {
        makeFalse(returnValue);
	}
	return returnValue;
}

/*  
 * Implements the primitive apply procedure
 */ 
Value *primitiveApply(Value *args) {
    raiseEvalError("Expected 2 arguments, got 0", args->type == NULL_TYPE);
    raiseEvalError("Expected 2 arguments, got 1", cdr(args)->type == NULL_TYPE);
    raiseEvalError("Expected 2 arguments, got more", cdr(cdr(args))->type != NULL_TYPE);
    raiseEvalError("Expected procedure/primitive", car(args)->type != CLOSURE_TYPE && car(args)->type != PRIMITIVE_TYPE);
    return apply(car(args), car(cdr(args)));
}

/*
 * Implements the primitive error procedure, which throws an error if called
 */
Value *primitiveError(Value *args) {
    raiseEvalError("Expected 1 argument, got 0", args->type == NULL_TYPE);
    raiseEvalError("Expected string argument, did not recieve", car(args)->type != STR_TYPE);
    raiseEvalError(car(args)->s, true);
    return args;
}

/*
 * Implements the variatic multiply procedure
 */ 
Value *primitiveMultiply(Value* args) {
	Value *remaining = args;
	Value *count = makeNull();
	bool isInt = true;
	int resultI = 1;
	double resultD = 1;
	while (remaining->type != NULL_TYPE) {
		Value *toMultiply = car(remaining);
		raiseEvalError("Expected number in multiply", toMultiply->type != INT_TYPE && toMultiply->type != DOUBLE_TYPE);
		if (toMultiply->type == DOUBLE_TYPE) {
			isInt = false;
			resultD *= toMultiply->d;
		} else {
			resultD *= toMultiply->i;
			resultI *= toMultiply->i;
		}
		remaining = cdr(remaining);
	}
	if (isInt) {
		count->type = INT_TYPE;
		count->i = resultI;
	} else {
		count->type = DOUBLE_TYPE;
		count->d = resultD;
	}
	return count;
}

/*
 * Implements the variatic subtract procedure
 */ 
Value *primitiveSubtract(Value* args) {
	Value *remaining = args;
    raiseEvalError("Subtraction requires at least one argument.", remaining->type == NULL_TYPE);
	Value *count = makeNull();
    bool oneDigit = true;
    bool firstIsInt = true;
	double result = 0;
    Value *first = car(remaining);
    remaining = cdr(remaining);
    if (first->type == DOUBLE_TYPE) {
        firstIsInt = false;
    }
	while (remaining->type != NULL_TYPE) {
        oneDigit = false;
		Value *toSubtract = car(remaining);
		raiseEvalError("Expected number in subtract", toSubtract->type != INT_TYPE && toSubtract->type != DOUBLE_TYPE);
		if (toSubtract->type == DOUBLE_TYPE) {
			result += toSubtract->d;
		} else {
			result += toSubtract->i;
		}
		remaining = cdr(remaining);
	}
    if (oneDigit) {
        if (firstIsInt) {
            count->type = INT_TYPE;
            count->i = -first->i;
        }
        else {
            count->type = DOUBLE_TYPE;
            count->d = -first->d;
        }
    }
    else {
        count->type = DOUBLE_TYPE;
        if (firstIsInt){
            count->d = first->i - result;
        }
        else {
            count->d = first->d - result;
        }
    }
	return count;
}

/*
 * Implements the variatic divide procedure
 */ 
Value *primitiveDivide(Value* args) {
	Value *remaining = args;
    raiseEvalError("Division requires at least one argument.", remaining->type == NULL_TYPE);
	Value *count = makeNull();
    bool oneDigit = true;
    bool firstIsInt = true;
	double result = 1;
    Value *first = car(remaining);
    remaining = cdr(remaining);
    if (first->type == DOUBLE_TYPE) {
        firstIsInt = false;
    }
	while (remaining->type != NULL_TYPE) {
        oneDigit = false;
		Value *toDivide = car(remaining);
		raiseEvalError("Expected number in divide", toDivide->type != INT_TYPE && toDivide->type != DOUBLE_TYPE);
		if (toDivide->type == DOUBLE_TYPE) {
            raiseEvalError("Division by 0.", toDivide->d == 0);
			result *= toDivide->d;
		} else {
            raiseEvalError("Division by 0.", toDivide->i == 0);
			result *= toDivide->i;
		}
		remaining = cdr(remaining);
	}
    if (oneDigit) {
        if (firstIsInt) {
            count->type = DOUBLE_TYPE;
            count->d = 1.0/first->i;
        }
        else {
            count->type = DOUBLE_TYPE;
            count->d = 1.0/first->d;
        }
    }
    else {
        count->type = DOUBLE_TYPE;
        if (firstIsInt){
            count->d = first->i / result;
        }
        else {
            count->d = first->d / result;
        }
    }
	return count;
}

/*
 * Implements the variatic less than or equal to procedure
 */ 
Value *primitiveLeq(Value* args) {
	Value *remaining = args;
    raiseEvalError("Expected at least 2 arguments, got 0.", car(remaining)->type == NULL_TYPE);
    raiseEvalError("Expected at least 2 arguments, got 1.", cdr(remaining)->type == NULL_TYPE);
	Value *previous = car(remaining);
    raiseEvalError("Expected number in less than or equal to", previous->type != INT_TYPE && previous->type != DOUBLE_TYPE);
    remaining = cdr(remaining);
    Value *result = makeNull();
    result->type = BOOL_TYPE;
    result->s = "#t";
	while (remaining->type != NULL_TYPE) {
		Value *current = car(remaining);
		raiseEvalError("Expected number in less than or equal to", current->type != INT_TYPE && current->type != DOUBLE_TYPE);
		if (current->type == DOUBLE_TYPE) {
            if (previous->type == DOUBLE_TYPE){
                if (previous->d > current->d) {
                    result->s = "#f";
                }
            }
            else {
                if (previous->i > current->d) {
                    result->s = "#f";
                }
            }
		} else {
			if (previous->type == DOUBLE_TYPE){
                if (previous->d > current->i) {
                    result->s = "#f";
                }
            }
            else {
                if (previous->i > current->i) {
                    result->s = "#f";
                }
            }
		}
		remaining = cdr(remaining);
	}
	return result;
}

/*
 * Implements the primitive pair? procedure
 */ 
Value *primitivePair(Value *args) {
	raiseEvalError("Expected 1 argument, got 0", args->type == NULL_TYPE);
	raiseEvalError("Expected 1 argument, got more than 1", cdr(args)->type != NULL_TYPE);
    Value *result = makeNull();
    result->type = BOOL_TYPE;
    result->s = "#f";
    if (car(args)->type == CONS_TYPE) {
        if (cdr(car(args))->type != NULL_TYPE) {
            result->s = "#t";
        }
    }
	return result;
}

/*
 * Recursively evaluate all the arguments in a combination and returns them
 * in order
 */
Value *recursiveEval(Value *remaining, Frame *frame) {
    if (remaining->type == NULL_TYPE) {
        return makeNull();
    } else {
        return cons(eval(car(remaining), frame), recursiveEval(cdr(remaining), frame));
    }
}

/*
 * Implements the primitive eq? procedure
 */ 
Value *primitiveEq(Value *args) {
	raiseEvalError("Expected 2 arguments, got 0", args->type == NULL_TYPE);
	raiseEvalError("Expected 2 arguments, got 1", cdr(args)->type == NULL_TYPE);
	raiseEvalError("Expected 2 arguments, got more than 2", cdr(cdr(args))->type != NULL_TYPE);
    Value *eq = makeNull();
    eq->type = BOOL_TYPE;
    if (car(args)->type != car(cdr(args))->type) {
        eq->s = "#f";
    }
    else if (car(args)->type == NULL_TYPE) {
        eq->s = "#t";
    }
    else if (car(args)->type == CONS_TYPE) {
        if (&car(args)->c.car == &car(cdr(args))->c.car && &car(args)->c.cdr == &car(cdr(args))->c.cdr){
            eq->s = "#t";
        }
        else {
            eq->s = "#f";
        }
    }
    else if (car(args)->type == PRIMITIVE_TYPE || car(args)->type == CLOSURE_TYPE) {
        if (car(args)->s == car(cdr(args))->s) {
            eq->s = "#t";
        }
        else {
            eq->s = "#f";
        }
    }
    else if (car(args)->type == BOOL_TYPE || car(args)->type == SYMBOL_TYPE || car(args)->type == STR_TYPE) {
        if (strcmp(car(args)->s,car(cdr(args))->s)) {
            eq->s = "#f";
        }
        else {
            eq->s = "#t";
        }
    }
    else if (car(args)->type == INT_TYPE) {
        if (car(args)->i == car(cdr(args))->i) {
            eq->s = "#t";
        }
        else {
            eq->s = "#f";
        }
    }
    else if (car(args)->type == DOUBLE_TYPE) {
        if (car(args)->d == car(cdr(args))->d) {
            eq->s = "#t";
        }        
    }
	return eq;
}

/*
 * Sets up builtins and special forms to be bound properly
 */
Frame *setUpBindings() {
	Frame *top = talloc(sizeof(Frame));
	top->parent = NULL;
	Value *head = makeNull();
    head = cons(makeSpecialForm("if"), head);
	head = cons(makeSpecialForm("quote"), head);
	head = cons(makeSpecialForm("let"), head);
    head = cons(makeSpecialForm("define"), head);
    head = cons(makeSpecialForm("lambda"), head);
    head = cons(makeSpecialForm("let*"), head);
    head = cons(makeSpecialForm("letrec"), head);
    head = cons(makeSpecialForm("cond"), head);
    head = cons(makeSpecialForm("set!"), head);
    head = cons(makeSpecialForm("and"), head);
    head = cons(makeSpecialForm("or"), head);
    head = cons(makeSpecialForm("begin"), head);
    head = cons(makeSpecialForm("else"), head);
	top->bindings = head;
	bind("+", primitiveAdd, top);
	bind("cons", primitiveCons, top);
	bind("car", primitiveCar, top);
	bind("cdr", primitiveCdr, top);
	bind("null?", primitiveNull, top);
    bind("apply", primitiveApply, top);
    bind("modulo", primitiveModulo, top);
    bind("zero?", primitiveZero, top);
    bind("equal?", primitiveEqual, top);
    bind("list", primitiveList, top);
    bind("append", primitiveAppend, top);
    bind("=", primitiveEqualsSign, top);
    bind("error", primitiveError, top);
    bind("*", primitiveMultiply, top);
    bind("-", primitiveSubtract, top);
    bind("/", primitiveDivide, top);
    bind("<=", primitiveLeq, top);
    bind("eq?", primitiveEq, top);
    bind("pair?", primitivePair, top);
    bind("apply", primitiveApply, top);
	return top;
}

/*
 * Handles a list of S-expressions (ie a Scheme program), calls eval()
 * on each S-expression in the top-level (global) environment, and
 * prints each result in turn.
 */
void interpret(Value *tree) {
	if (top == NULL) {
		// Initialize bindings the first time
		top = setUpBindings();
	}
	Value *remaining = tree;
	while (remaining->type != NULL_TYPE) {
		printValue(eval(car(remaining), top));
		remaining = cdr(remaining);
        sweep(remaining, top); //collect garbage
	}
}

/*
 * Takes a parse tree of a single S-expression and an environment frame in
 * which to evaluate the expression, then returns a pointer to a Value 
 * representing the evaluated value. 
 */
Value *eval(Value *expr, Frame *frame){
	switch (expr->type) {
		case INT_TYPE:
			return expr;
			break;
		case STR_TYPE:
			return expr;
			break;
		case DOUBLE_TYPE:
			return expr;
			break;
		case NULL_TYPE:
			return expr;
			break;
		case BOOL_TYPE:
			return expr;
			break;
        case VOID_TYPE:
            return expr;
            break;
		case SYMBOL_TYPE:
			return lookUpSymbol(expr, frame);
			break;
        case CLOSURE_TYPE:
            return expr;
            break;
		case PRIMITIVE_TYPE:
			return expr;
			break;
		case CONS_TYPE: {// THIS IS WHERE THE FUN BEGINS
			Value *first = car(expr);
			Value *args = cdr(expr);
			raiseEvalError("Null Pointer", first->type == NULL_TYPE);
			Value *firstEval = eval(first, frame);
			raiseEvalError("Attempting to call non-function", firstEval->type != SYMBOL_TYPE && firstEval->type != CLOSURE_TYPE && firstEval->type != PRIMITIVE_TYPE);
            if (firstEval->type == SYMBOL_TYPE) {
                if (!strcmp(firstEval->s, "if")) {
                    return evalIf(args, frame);
                } else if (!strcmp(firstEval->s, "quote")) {
                    return evalQuote(args, frame);
                } else if (!strcmp(firstEval->s, "let")) {
                    return evalLet(args, frame);
                } else if (!strcmp(firstEval->s, "define")) {
                    return evalDefine(args);  
                } else if (!strcmp(firstEval->s, "lambda")) {
                    return evalLambda(args, frame);
                } else if (!strcmp(firstEval->s, "let*")) {
                    return evalLetStar(args, frame);
                } else if (!strcmp(firstEval->s, "letrec")) {
                    return evalLetrec(args, frame);
                } else if (!strcmp(firstEval->s, "set!")) {
                    return evalSet(args, frame);
                } else if (!strcmp(firstEval->s, "and")) {
                    return evalAnd(args, frame);
                } else if (!strcmp(firstEval->s, "or")) {
                    return evalOr(args, frame);
                } else if (!strcmp(firstEval->s, "begin")) {
                    return evalBegin(args, frame);
                } else if (!strcmp(firstEval->s, "cond")) {
                    return evalCond(args, frame);
                } else if (!strcmp(firstEval->s, "else")) {
                	if (inCond) {
                		return expr;
                	}
                	raiseEvalError("else: not allowed outside of cond.", true);
                } else {
                    raiseEvalError("Unrecognized special form.", true);
                }
            } else {
                Value *remaining = args;
                Value *results = recursiveEval(remaining, frame);
                return apply(firstEval, results);
            }
            raiseEvalError("This shouldn't occur.", true);
            return NULL;
		}
		case PTR_TYPE:
			raiseEvalError("Poorly formed tree!", true);
			return NULL;
		case OPEN_TYPE:
			raiseEvalError("Unparsed open parentheses!", true);
			return NULL;
		case CLOSE_TYPE:
			raiseEvalError("Unparsed close parentheses!", true);
			return NULL;
        case QUOTE_TYPE:
            raiseEvalError("Unparsed quote!", true);
            return NULL;
	}
	raiseEvalError("This shouldn't have even happened!", true);
	return NULL;
}