/***************************************************************************
                          sdstack.c  -  description
                             -------------------
    begin                : Fri Dec 10 2004
    email                : brunk@unc.edu
 ***************************************************************************/
                                                                                
/***************************************************************************
 *                                                                         *
 *  This software is part of the enhancements to the MIMS Spatial Allocator
 *
 *  Implements a stack data structure that operates on doubles
 *  Refer to the header block in sdstack.h for furhter details
 *                                                                         *
 ***************************************************************************/

#include "sdstack.h"
#include <stdio.h>


/* init_stack()           Initializes a stack to an empty state  */
void init_dstack(DSTACK *s_ptr)
{
    s_ptr->top = STACKMAX;
}


/* is_empty()              Returns 1 if the DSTACK is empty and 0 if not. */
int is_emptyd( DSTACK s)
{
     #ifdef DSTACK_DEBUG
     printf("checking if dstack is empty\n");
     #endif

     if (s.top >= STACKMAX)
	return(1);
     else
	return(0);
}

/*  is_full()     Returns 1 if the DSTACK is full, and 0 if not.  */
int is_fulld(DSTACK s)
{
     #ifdef DSTACK_DEBUG
     printf("checking if dstack is full\n");
     #endif


     if(s.top <= 0)
	return(1);
     else
	return(0);

}

/* push a new item onto the stack*/
/* adjust top of stack */
int pushd(double item, DSTACK *s_ptr)  
{                              
     if(is_fulld(*s_ptr))
	return(-1);

     else
     {
	  s_ptr->top--;
	  s_ptr->elts[ s_ptr->top ] = item;
          #ifdef DSTACK_DEBUG
          printf("%s pushed onto stack\n", item);
          #endif
	  return(0);
     }

}

/* pop an item off of the stack */
double popd(DSTACK *s_ptr)
{
      double returnVal;

      if(is_emptyd(*s_ptr) )
	 return 0;

      else
      {
         returnVal = s_ptr->elts[s_ptr->top];

         s_ptr->top++;

         #ifdef DSTACK_DEBUG
         printf("%s popped from stack\n", returnString);
         #endif
         return returnVal;
      }

}



