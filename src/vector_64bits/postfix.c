/***************************************************************************
                          postfix.c  -  description
                             -------------------
    begin                : Fri Dec 3 2004
    email                : brunk@unc.edu
 ***************************************************************************/

/***************************************************************************
 *                                                                         
 *  This software is part of the enhancements to the MIMS Spatial Allocator
 *
 * postfix.c  This module will convert the infix into the postfix
 * and trap user syntax errors                                                                          
 ***************************************************************************/

#include "sastack.h"
#include "io.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>


void convertPostfix(char infix[], char postfix[])
{


     /*  parenCount checks for paren errors */
     int inCount, postCount, check, check2, parenCount;
     int tmpCount, errorLevel = 0;
     char *tmpString, mesg[256];
     char oper;
     STACK st;
     char *prog_name = "postfix module";


     inCount = 0;
     postCount = 0;
     check2 = 0;
     parenCount = 0;
     check = 20;
     postfix[0] = '\0';


     init_stack(&st);

     for(inCount = 0; infix[inCount] != '\0'; inCount++)
     {
              /* if it's a number or a char, just pass it on */
              /* need to check for '.' also!!! */
              if(isalnum(infix[inCount]) || infix[inCount] == '.' || infix[inCount] == '_')
              {
	           postfix[postCount++] = infix[inCount];
                   postfix[postCount] = '\0';
                   #ifdef DEBUG_POST
                   printf("postfix=%s\n", postfix);
                   #endif
              }   
              else if(infix[inCount] == '(')
              {

                   parenCount++;
                   push('(', &st);
                   /*postfix[postCount++] = ' ';*/
                   /*postfix[postCount] = '\0';*/
                   #ifdef DEBUG_POST
                   printf("postfix=%s\n", postfix);
                   #endif

              }
              else if(infix[inCount] == ')')
              {

                  parenCount--;

                  while((oper = pop(&st)) != '('  && is_empty(st) != 1)
                  {
                      if(oper != '(')
                      {
                           postfix[postCount++] = ' ';
                           postfix[postCount] = '\0';
                           #ifdef DEBUG_POST
                           printf(") detected postfix=%s\n", postfix);
                           #endif


                           postfix[postCount++] = oper;
		           postfix[postCount++] = ' ';
                           postfix[postCount] = '\0';
                           #ifdef DEBUG_POST
                           printf(") detected postfix=%s\n", postfix);
                           #endif
		      }
                      else
                      {
                           postfix[postCount++] = ' ';
                           postfix[postCount] = '\0';
                           #ifdef DEBUG_POST
                           printf(") detected postfix=%s\n", postfix);
                           #endif
		      }
                  }
              }
              else if(infix[inCount] == '-')
              {
                   check2 = 4;
                   if(check2 >= check && is_empty(st) != 1)
                   {
                       postfix[postCount++] = ' ';
                       postfix[postCount] = '\0';
                       #ifdef DEBUG_POST
                       printf("postfix=%s\n", postfix);
                       #endif
                       oper =  pop(&st);
                       if(oper != '(')
                       { 
                           postfix[postCount++] = oper;

		           postfix[postCount++] = ' ';
                           postfix[postCount] = '\0';
                           #ifdef DEBUG_POST
                           printf("postfix=%s\n", postfix);
                           #endif
                       }
                   }
                   else
                   {
                        postfix[postCount++] = ' ';
                        postfix[postCount] = '\0';
                        #ifdef DEBUG_POST
                        printf("postfix=%s\n", postfix);
                        #endif
                   }
                  push('-', &st);
                   check = check2;
                }
                else if(infix[inCount] == '+')
                {
                     check2 = 3;

                     if(check2 >= check && is_empty(st) != 1)
                     {
                           postfix[postCount++] = ' ';
                           postfix[postCount] = '\0';
                           #ifdef DEBUG_POST
                           printf("Before+pop\n");   
                           #endif
                           oper = pop(&st);
                           #ifdef DEBUG_POST
                           printf("oper=%c\n",oper);
                           #endif
			   if(oper != '(')
                           {
                                postfix[postCount++] = oper;
		                postfix[postCount++] = ' ';
                                postfix[postCount] = '\0';
                           }


		     }
		     else
		     {
		           postfix[postCount++] = ' ';
                           postfix[postCount] = '\0';
		     }
	             push('+', &st);
                     #ifdef DEBUG_POST
                     printf("postfix=%s\n", postfix);
                     #endif
	             check = check2;
               }
               else if(infix[inCount] == '*')
               {
	             check2 = 1;

                     if(check2 >= check && is_empty(st) != 1)
		     {
                          postfix[postCount++] = ' ';
                          postfix[postCount] = '\0';

		          oper = pop(&st);
		          if(oper != '(')
                          {
                              postfix[postCount++] = oper;
                              postfix[postCount++] = ' ';
                              postfix[postCount] = '\0';
                          }

		     }
                     else
                     {
                          postfix[postCount++] = ' ';
                          postfix[postCount] = '\0';
		     }
		     push('*', &st);
                     #ifdef DEBUG_POST
                     printf("postfix=%s\n", postfix);
                     #endif
                     check = check2;
               }
               else if(infix[inCount] == '/')
               {
                    check2 = 2;
                    if(check2 >= check && is_empty(st) != 1)
		    {
                           postfix[postCount++] = ' ';
                           postfix[postCount] = '\0';
			   oper = pop(&st);
		           if(oper != '(')
                           {
                                postfix[postCount++] = oper;
		                postfix[postCount++] = ' ';
                                postfix[postCount] = '\0';
                           }
                    }
                    else
                    {
                            postfix[postCount++] = ' ';
                            postfix[postCount] = '\0';
                            #ifdef DEBUG_POST
                            printf("postfix=%s\n", postfix);
                            #endif
                    }
		    push('/', &st);
                    #ifdef DEBUG_POST
                    printf("postfix=%s\n", postfix);
                    #endif
                    check =  check2;
               }
               else if(infix[inCount] == ' ')
               {
                    continue; /* ingore spaces in infix for now */ 
               }
               else
               {
                       /* error condition */
	            errorLevel = 1;
                    sprintf(mesg,"%s is not a valid character in the expression", infix[inCount]);
                    ERROR(prog_name, mesg, 1);
               }


        }     /*  end of for loop  */

        /* now pop the rest of the operators off of the stack */
        while(is_empty(st) != 1)
        {
             oper = pop(&st);

             if(oper != '(')
             {
                  postfix[postCount++] = ' ';
                  postfix[postCount] = '\0';
                  #ifdef DEBUG_POST
                  printf("popping operator %c\n", oper);
                  #endif

                  postfix[postCount++] = oper;
                  postfix[postCount] = '\0';
             }
             #ifdef DEBUG_POST
             printf("postfix=%s\n", postfix);
             #endif
        }

        /* adds the last operator to the postfix string */
        /*if(oper != '(')
        {
             postfix[postCount++] = ' ';
             postfix[postCount] = '\0';
             #ifdef DEBUG_POST
             printf("popping operator %c\n", oper);
             #endif

             postfix[postCount++] = oper;

             postfix[postCount] = '\0';
             #ifdef DEBUG_POST
             printf("postfix=%s\n", postfix);
             #endif
        }*/

        /* checks to see that there was an equal number of left and
           right parentheses */

        if(parenCount != 0)
        {
            errorLevel = 2;
            ERROR(prog_name, "Mismatched parentheses in mathematical equation", 1);
        }

        #ifdef DEBUG_POST
        printf("convertPostfix completed\n");
        #endif
}  /*  end of convertPostfix()  */

