/***************************************************************************
                          sdstack.h  -  description
                             -------------------
    begin                : Fri Dec 10 2004
    email                : brunk@unc.edu
 ***************************************************************************/
                                                                                                                                              
/***************************************************************************
 *                                                                         
 *  This software is part of the enhancements to the MIMS Spatial Allocator
 *
 *  Header file for module that implements a stack data structure
 *  that operates on doubles
 *  This module is identical to sastack except that it stores doubles 
 *  and was added solely to increase the overall throughput of the postfix
 *  expression evaluator by not having to convert from strings to doubles
 *  and back again while calculating the final answer
 *                                                                         
 ***************************************************************************/

/* sdstack.h */
#ifndef SDSTACK_H
#define SDSTACK_H

#include <stdlib.h>

#ifndef MAX
#define STACKMAX 200
#endif 


typedef struct
	{
	 double elts[STACKMAX];
	 int top;
	}

DSTACK;

int pushd(double item, DSTACK *s_ptr);
double popd(DSTACK *s_ptr);
void init_dstack(DSTACK *s_ptr);
int is_emptyd(DSTACK s);
int is_fulld(DSTACK s);

#endif
