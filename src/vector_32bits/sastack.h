/***************************************************************************
                          sastack.h  -  description
                             -------------------
    begin                : Fri Dec 3 2004
    email                : brunk@unc.edu
 ***************************************************************************/
                                                                                                                                              
/***************************************************************************
 *                                                                         
 *  This software is part of the enhancements to the MIMS Spatial Allocator
 *
 *  Header file for module that implements a stack data structure
 *  that operates on strings
 *                                                                         
 ***************************************************************************/

/* sastack.h */
#ifndef SASTACK_H
#define SASTACK_H
#include <string.h>
#include <stdlib.h>
#include <ctype.h>

#define STACKMAX 200

typedef struct
	{
	 char elts[STACKMAX];
	 int top;
	}

STACK;

int push(char item, STACK *s_ptr);
char pop(STACK *s_ptr);
void init_stack(STACK *s_ptr);
int is_empty(STACK s);
int is_full(STACK s);

#endif
