/***************************************************************************
                          sastack.c  -  description
                             -------------------
    begin                : Fri Dec 3 2004
    email                : brunk@unc.edu
 ***************************************************************************/
                                                                                
/***************************************************************************
 *                                                                         *
 *  This software is part of the enhancements to the MIMS Spatial Allocator
 *
 *  Implements a stack data structure that operates on strings
 *                                                                         *
 ***************************************************************************/

#include "sastack.h"
#include <stdio.h>


/* init_stack()           Initializes a stack to an empty state  */
void init_stack(STACK *s_ptr)
{
    s_ptr->top = STACKMAX;
    /*s_ptr->elts = (char *) malloc(STACKMAX * sizeof(char *));*/

}

/* is_empty()              Returns 1 if the STACK is empty and 0 if not. */
int is_empty( STACK s)
{
     #ifdef STACK_DEBUG
     printf("checking if stack is empty\n");
     #endif

     if (s.top >= STACKMAX)
     {
        #ifdef STACK_DEBUG
        printf("s.top=%d ge STACKMAX\n", s.top);
        #endif
	return(1);
     }
     else
     {
        #ifdef STACK_DEBUG
        printf("s.top=%d not ge STACKMAX\n", s.top);
        #endif
	return(0);
     }
}

/*  is_full()     Returns 1 if the STACK is full, and 0 if not.  */
int is_full(STACK s)
{
     #ifdef STACK_DEBUG
     printf("checking if stack is full\n");
     #endif


     if(s.top <= 0)
	return(1);
     else
	return(0);

}

/* push a new item onto the stack*/
/* adjust top of stack */
int push(char item, STACK *s_ptr)  
{                              

     if(is_full(*s_ptr))
	return(-1);

     else
     {
	  s_ptr->top--;
	  s_ptr->elts[ s_ptr->top ] = item;
          /*#ifdef STACK_DEBUG
          printf("%c pushed onto stack\n", item);
          #endif*/
	  return(0);
     }

}

/* pop an item off of the stack */
char pop(STACK *s_ptr)
{
      char item;

      if(is_empty(*s_ptr) )
	 return 0;

      else
      {
         item = s_ptr->elts[s_ptr->top];
         s_ptr->top++;

         #ifdef STACK_DEBUG
         printf("%c popped from stack\n", item);
         #endif
         return item;
      }

}



