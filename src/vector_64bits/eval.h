/***************************************************************************
                          eval.h  -  description
                             -------------------
    begin                : Mon 6 December 2004
    copyright            : (C) 2004 by Benjamin Brunk
    email                : brunk@unc.edu
 ***************************************************************************/
                                                                                                          
/***************************************************************************
 *                                                                         *
 *  This software is part of the enhancements to the MIMS Spatial Allocator
 *
 *  Header function for module to mathematically evaluates postfix expressions
 *                                                                         *
 ***************************************************************************/
#ifndef EVAL_H
#define EVAL_H

#define TRUE 1
#define FALSE 0

typedef struct _EXPRESSION_ELEMENT
{
	char *item;
        int isVariable;

}  EXPRESSION_ELEMENT;


int compileExpression(char infixExpression[]);
double calculateExpression(double values[]);
double evaluatePostfix(char postfix[]);
void convertPostfix(char infix[], char postfix[]);
char *convertToUpper(char *input);

#endif
