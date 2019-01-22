#include <stdbool.h>
#include "value.h"
#include "linkedlist.h"
#include "tokenizer.h"
#include "talloc.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
 
/*
 * Prints a simple error trace and exits the program upon discovering untokenizable input
 */
void raiseError(char* message, bool condition, int location, char charRead) {
    if (!condition) {
        printf("Untokenizable input on character '%c' at %i: %s\n", charRead, location, message);
        texit(1);
    }
}

/*
 * Takes a list and the length of said list as parameters.
 * Returns a string of all the characters in the list concatenated together.
 */ 
char *join(Value *list, int length) {
    char *string = talloc(sizeof(char) * (length + 1));
    string[length] = '\0';
    for (int i = length - 1; i >= 0; i--) {
        string[i] = car(list)->s[0];
        list = cdr(list);
    }
    return string;
}

/*
 * Returns true if a char c is in the set of initial symbols
 */
bool isInInitialSymbol(char c) {
    char *valid = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ!$%&*/:<=>?~_^";
    for (int i = 0; i < 66; i++) {
        if (valid[i] == c) {
            return true;
        }
    }
    return false;
}

/*
 * Returns true if a char c is in the set of subsequent symbols
 */
bool isInSubsequentSymbol(char c) {
    char *valid = "0123456789.+-";
    if (isInInitialSymbol(c)) {
        return true;
    }
    for (int i = 0; i < 13; i++) {
        if (valid[i] == c) {
            return true;
        }
    }
    return false;
}

/* 
 * Returns true if a char c is in the set of digits
 */ 
bool isDigit(char c) {
    char *valid = "0123456789";
    for (int i = 0; i < 10; i++) {
        if (valid[i] == c) {
            return true;
        }
    }
    return false;
}

/* 
 * Returns a value object that stores as single character as a string
 */
Value *buildString(char c, Value *currentString, int *currentStringLength) {
    Value *newChar = makeNull();
    newChar->type = STR_TYPE; 
    newChar->s = talloc(sizeof(char) * 2);
    newChar->s[0] = c;
    newChar->s[1] = '\0';
    currentString = cons(newChar, currentString);
    *currentStringLength = *currentStringLength + 1;
    return currentString;
}

Value *endString(Value *list, Value *currentString, int *currentStringLength, valueType type) {
    Value *newValue = makeNull();
    newValue->type = type;
    if (type == STR_TYPE || type == SYMBOL_TYPE || type == BOOL_TYPE) {
        newValue->s = join(currentString, *currentStringLength);
    } else if (type == DOUBLE_TYPE) {
        newValue->d = atof(join(currentString, *currentStringLength));
    } else if (type == INT_TYPE) {
        newValue->i = atoi(join(currentString, *currentStringLength));
    }
    *currentStringLength = 0;
    list = cons(newValue, list);
    return list;
}

/* 
 * Prints each found token to the terminal
 */
void displayTokens(Value *list) {
    while (list->type != NULL_TYPE) {
        if (car(list)->type == STR_TYPE) {
            printf("\"%s\":string", car(list)->s);
        } else if (car(list)->type == DOUBLE_TYPE) {
            printf("%f:double", car(list)->d);
        } else if (car(list)->type == INT_TYPE) {
            printf("%i:integer", car(list)->i);
        } else if (car(list)->type == OPEN_TYPE) {
            printf("%s:open", car(list)->s);
        } else if (car(list)->type == CLOSE_TYPE) {
            printf("%s:close", car(list)->s);
        } else if (car(list)->type == SYMBOL_TYPE) {
            printf("%s:symbol", car(list)->s);
        } else if (car(list)->type == BOOL_TYPE) {
            printf("%s:boolean", car(list)->s);
        } 
        list = cdr(list);
        printf("\n");
    }
}

/* 
 * Takes in a character stream, returns a list of every tokenizable symbol
 * in the stream or gracefully exits if a non-tokenizable symbol is encountered.
 */
Value *tokenize() {
    int inputStatus = isatty(fileno(stdin));
    char charRead;
    Value *list = makeNull();
    Value *currentString = makeNull();
    int currentStringLength = 0;
    charRead = 'a';
    int totalRead  = 0;
    bool inString  = false;
    bool inSymbol  = false;
    bool inComment = false;
    bool inNumber  = false;
    bool inDecimal = false;
    bool isEscaped = false;
    while ((inputStatus == 1 && charRead != 10 && charRead != EOF) || (inputStatus == 0 && charRead != EOF)) {
        totalRead++;
        charRead = fgetc(stdin);
        if (inComment) {
            if (charRead == '\n') {
                inComment = false;
            } 
        } else {
            // Strings
            if (inString) {
                if (isEscaped) {
                    if (charRead == 'n') {
                        currentString = buildString('\n', currentString, &currentStringLength);
                    } else if (charRead == 't') {
                        currentString = buildString('\t', currentString, &currentStringLength);
                    } else if (charRead == '\\') {
                        currentString = buildString('\\', currentString, &currentStringLength);
                    } else if (charRead == '"') {
                        currentString = buildString('"', currentString, &currentStringLength);
                    } else if (charRead == '\'') {
                        currentString = buildString('\'', currentString, &currentStringLength);
                    } else {
                        // Invalid escaped character
                        raiseError("Invalid escaped character", false, totalRead, charRead);
                    }
                    isEscaped = false;
                } else if (charRead == '"') {
                    list = endString(list, currentString, &currentStringLength, STR_TYPE);
                    currentString = makeNull();
                    inString = false;
                } else if (charRead == '\\') {
                    isEscaped = true;
                } else {
                    currentString = buildString(charRead, currentString, &currentStringLength);
                }

            // Symbols
            } else if (inSymbol) {
                if (charRead == ' ' || charRead == '(' || charRead == ')' || charRead == EOF || charRead == '\n' || charRead == '\r') {
                    list = endString(list, currentString, &currentStringLength, SYMBOL_TYPE);
                    currentString = makeNull();
                    inSymbol = false;
                    if (!(inputStatus == 1 && charRead == 10)) {
                        ungetc(charRead, stdin);
                    }
                    totalRead--;
                } else {
                    raiseError("Invalid symbol", isInSubsequentSymbol(charRead), totalRead, charRead);
                    currentString = buildString(charRead, currentString, &currentStringLength);
                }

            // Numbers
            } else if (inNumber) {
                if (charRead == ' ' || charRead == '(' || charRead == ')' || charRead == EOF || charRead == '\n' || charRead == '\r') {
                    raiseError("Invalid number", !(currentStringLength >= 1 && car(currentString)->s[0] == '.'), totalRead, charRead);
                    if (inDecimal) {
                        list = endString(list, currentString, &currentStringLength, DOUBLE_TYPE);
                        currentString = makeNull();
                    } else {
                        list = endString(list, currentString, &currentStringLength, INT_TYPE);
                        currentString = makeNull();
                    }
                    if (!(inputStatus == 1 && charRead == 10)) {
                        ungetc(charRead, stdin);
                    }
                    totalRead--;
                    inNumber = false;
                    inDecimal = false;
                } else {
                    if (!inDecimal && charRead == '.') {
                        inDecimal = true;
                        currentString = buildString(charRead, currentString, &currentStringLength);
                    } else {
                        raiseError("Invalid number", isDigit(charRead), totalRead, charRead);
                        currentString = buildString(charRead, currentString, &currentStringLength);
                    }
                }

            // Handle Parentheses
            } else if (charRead == '(') {
                Value *newValue = makeNull();
                newValue->type = OPEN_TYPE;
                newValue->s = talloc(sizeof(char) * 2);
                strcpy(newValue->s, "(");
                list = cons(newValue, list);
            } else if (charRead == ')') {
                Value *newValue = makeNull();
                newValue->type = CLOSE_TYPE;
                newValue->s = talloc(sizeof(char) * 2);
                strcpy(newValue->s, ")");
                list = cons(newValue, list);   
            } else if (charRead == '\'') {
                Value *newValue = makeNull();
                newValue->type = QUOTE_TYPE;
                newValue->s = talloc(sizeof(char) * 2);
                strcpy(newValue->s, "(");
                list = cons(newValue, list);
            } 
            // Else detect if we should flip on any new flags
            else if (isDigit(charRead)) {
                inNumber = true;
                currentString = buildString(charRead, currentString, &currentStringLength);

            } else if (charRead == '.') {
                inNumber = true;
                inDecimal = true;
                currentString = buildString(charRead, currentString, &currentStringLength);

            } else if (charRead == '+' || charRead == '-') {
                currentString = buildString(charRead, currentString, &currentStringLength);
                char nextChar = fgetc(stdin);
                if (nextChar == ' ' || nextChar == '(' || nextChar == ')' || nextChar == EOF || nextChar == '\n' || nextChar == '\r') {
                    list = endString(list, currentString, &currentStringLength, SYMBOL_TYPE);
                    currentString = makeNull();
                } else {
                    inNumber = true;
                }
                ungetc(nextChar, stdin);

            } else if (charRead == '#') {
                char nextChar = fgetc(stdin);
                if (nextChar == 't' || nextChar == 'f') {
                    currentString = buildString(charRead, currentString, &currentStringLength);
                    currentString = buildString(nextChar, currentString, &currentStringLength);
                    list = endString(list, currentString, &currentStringLength, BOOL_TYPE);
                } else {
                    raiseError("Incorrect Boolean", false, totalRead, charRead);
                }
            } else if (charRead == ';') {
                inComment = true;

            } else if (charRead == '"' && !isEscaped) {
                inString = true;
            } else if (isInInitialSymbol(charRead)) {
                inSymbol = true;
                currentString = buildString(charRead, currentString, &currentStringLength);

            // Continue on our merry way
            } else if (charRead == '\n' || charRead == '\t' || charRead == ' ' || charRead == '\r' || charRead == EOF) {
                continue;
            } else {
                raiseError("Input not recognized", false, totalRead, charRead);
            }
        }
    }
    return reverse(list);
}
