/***************************************************************************
                          eval.c  -  description
                             -------------------
    begin                : Mon 6 December 2004
    email                : brunk@unc.edu
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *  This software is part of the enhancements to the MIMS Spatial Allocator
 *
 *  Mathematically evaluates postfix expressions
 *                                                                         *
 ***************************************************************************/
/* Modified 4/4/2005 to replace getenv with getEnvtValue wrapper function BB */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "sastack.h"
#include "sdstack.h"
#include "shapefil.h"

#include "eval.h"
#include "mims_evs.h"
#include "io.h"

int numElements;
EXPRESSION_ELEMENT *postfixE;

#ifdef EVAL_C_MAIN
char *prog_name;
int main(int argc, char *argv[])
{
    *prog_name = argv[0];

  int i;
    double values[4];
    double result;


    values[0] = 2.0;
    values[1] = 5.0;
    values[2] = 2.0;
    values[3] = 1.0;


    /*compileExpression("(AB_+CD_+DE_)");*/
    compileExpression("CLASS1_03_ + CLASS2_03_ + CLASS3_03_ + CLASS4_03_");
    /*compileExpression("CLASS1_03_+CLASS2_03_");*/
    /* error example compileExpression("2 AB  +");*/

/*     printf("returned from compilePostfix\n"); */
/*     printf("numElements=%d\n", numElements); */

    result = calculateExpression(values);

/*     printf("result=%f\n", result); */


    /*weightDBF(argv[1], argv[2]); */

    /* printf("Returned from weightDBF\n"); */



}
#endif


/* wrapper function for calling this module from 
   MIMS or another surrogate tool */
int weightDBF(char *inFile, char *outFile)
{
    FILE *pOfp;
    DBFHandle hDBF;
    DBFFieldType eType;
    char *expression;
    int valIndex, fieldCount, varCount, totalRecords, recordCount, numFields, *fieldIndices;
    int fieldSize, numberDecimals, n;
    char fieldName[50];
    double result, *fieldValues;
    DBFFieldType *fieldTypes;

	char *prog_name = "postfix evaluation module";
    char mesg[256];



    getEnvtValue(ENVT_WEIGHT_FUNCTION, expression);
    /* expression = getenv("WEIGHT_FUNCTION"); */
    /*expression = strdup("HOUSEHOLDS + MARHH_CHD + MHH_CHILD + AGE_22_29 + AGE_3__39 + AGE_4__49 + AGE_5__64 + AGE_65_UP + MALES + FEMALES");*/

    #ifdef DEBUG
    fprintf(stdout,"Weight function=%s\n", expression);
    #endif


    if((pOfp = fopen(outFile, "w")) == NULL)
    {
        sprintf(mesg,"Unable to open %s for writing\n", outFile);
        ERROR(prog_name, mesg, 2);
    }

    hDBF = DBFOpen( inFile, "rb" );

    if( hDBF == NULL )
    {
        sprintf(mesg, "Unable to open %s for reading", inFile);
        ERROR(prog_name, mesg, 2);
    }

    if( (numFields = DBFGetFieldCount(hDBF)) <= 0 )
    {
        sprintf(mesg, "%s contains no records", inFile);
        ERROR(prog_name, mesg, 2);
    }

    #ifdef DEBUG
    fprintf(stdout, "number of fields=%d\n", numFields);
    #endif

    /* compile the expression */
    compileExpression(expression);


    /* create an array that will indicate which fields we are interested in */
    fieldIndices = (int *) malloc(numElements * sizeof(int));

    /* now create arrays to hold the field values and types needed */
    fieldValues = malloc(numElements * sizeof(double));
    fieldTypes = (DBFFieldType *) malloc(numElements * sizeof(DBFFieldType));

    /* initialize fieldIndices array */
    for(varCount = 0; varCount < numElements; varCount++)
    {
         fieldIndices[varCount] = -1;
    }

    /* set the values of that array according to the information in the db table */
    for(fieldCount = 0; fieldCount < numFields; fieldCount++)
    {
         eType = DBFGetFieldInfo( hDBF, fieldCount, fieldName, &fieldSize, &numberDecimals );
         /*printf("fieldname=%s\n", fieldName);*/

         /*printf("fieldName: %s type=", fieldName);

         if(eType == FTInteger)
         {
              printf("FTInteger\n"); 
         }
         else if(eType == FTDouble)
         {
              printf("FTDouble\n"); 

         }
         else if(eType == FTString)
         {
              printf("FTString\n"); 

         } */

         for(varCount = 0; varCount < numElements; varCount++)
         {
              /*printf("%s =  %s = %s\n", convertToUpper(postfixE[varCount].item), convertToUpper(fieldName), fieldName); */
              if(strcmp(convertToUpper(postfixE[varCount].item), convertToUpper(fieldName)) == 0)
              {
                   fieldIndices[varCount] = fieldCount;              
              }

              /*printf("fieldIndices[%d] = %d\n", varCount, fieldIndices[varCount]);*/

              /*printf("fieldName: %s type=", fieldName, eType);*/

              if(eType == FTInteger)
              {
                   fieldTypes[varCount] = FTInteger;
              }
              else if(eType == FTDouble)
              {
                  fieldTypes[varCount] = FTDouble;
              }
              else if(eType == FTString)
              {
                  fieldTypes[varCount] = FTString;
              } 

         }


    }

    /* total number of records in the input file */
    totalRecords = DBFGetRecordCount(hDBF);

    #ifdef DEBUG
    fprintf(stdout,"number of Elements=%d\n", numElements);
    fprintf(stdout,"number of records=%d\n", totalRecords);
    #endif

    /* print the expression to the top of the output file for reference */
    fprintf(pOfp,"%s\n", expression);

    /* loop over all the rows in the db file and perform the calculation */
    for(recordCount = 0; recordCount < totalRecords; recordCount++)
    {
         valIndex = 0;

         for(varCount = 0; varCount < numElements; varCount++)
         {
              if(fieldIndices[varCount] > -1)
              {
                   if(fieldTypes[varCount] == FTDouble)
                   {
                        fieldValues[valIndex++] = DBFReadDoubleAttribute( hDBF, recordCount, fieldIndices[varCount] );
/*                         printf("fieldValues[%d]=%f\n", valIndex-1, fieldValues[valIndex-1]); */
                   }
                   else if(fieldTypes[varCount] == FTInteger)
                   {
                        /* convert to double */
                        fieldValues[valIndex++] = (double) DBFReadIntegerAttribute( hDBF, recordCount, fieldIndices[varCount] );
/*                         printf("fieldValues[%d]=%f\n", valIndex-1, fieldValues[valIndex-1]); */

                   }
                   else if(fieldTypes[varCount] == FTString)
                   {
                        /* this is an error condition, we can't compute on a string value */
                        sprintf(mesg, "Variable in %s is not of Integer or Double type", inFile);
                        ERROR(prog_name, mesg, 2);
                   }
                   else
                   {
                        /* this should never occur, but who knows? */
                        sprintf(mesg, "Unknown datatype encountered in %s", inFile);
                        ERROR(prog_name, mesg, 2);

                   }
                   /*fprintf(pOfp, "\"%f\",", fieldValues[varCount]);*/
              }
         }

         /* process the data and save into csv format */
         result = calculateExpression(fieldValues);

         /* write result to the output file */
         fprintf(pOfp, "%10.3f\n", result); 
    }

    free(fieldIndices);
    free(fieldValues);

    for(n = 0; n < numElements; n++)
    {
        free(postfixE[n].item);
    }
    free(postfixE);

    DBFClose(hDBF);
    fclose(pOfp);

    return 1;  /* success */

}

/* creates a postfix expression based on an infix 
   input and returns the list of variable names in
   the order the appear in the expression
*/
int compileExpression(char infixExpression[])
{
     char *tmpVar, *postfixExpression;

     int i, isal, validEq = FALSE;


     postfixE = NULL;

     /* added 1/10/2005 to fix a crash problem when an invalid
        equation is provided 
     */
     for(i = 0; infixExpression[i] != '\0'; i++)
     {
          if(infixExpression[i] == '+' ||
             infixExpression[i] == '-' ||
             infixExpression[i] == '*' ||
             infixExpression[i] == '/')
               validEq = TRUE;

     }

     if(validEq == FALSE)
     {
          ERROR(ENVT_WEIGHT_FUNCTION, "Function specified is not a valid equation", 1);
     }

     /* TBD: consider size of array for SrgMerge */
     postfixExpression = (char *) malloc(1024 * sizeof(char));

     convertPostfix(infixExpression, postfixExpression);

     /*#ifdef DEBUG
     printf("postfix:%s\n", postfixExpression);
     #endif
     */


     /* turns out that strtok modifies the original string */
     /*tmpExpression = (char *) strdup(postfixExpression);*/

     /*printf("tmpExpression=%s\n", tmpExpression);*/

     tmpVar = strtok(postfixExpression, " ");

     numElements = 0;

     while(tmpVar != NULL)
     { 
          /* see if it is a variable or something else */
          isal = FALSE;


          for(i = 0; i < strlen(tmpVar); i++)
          {
               if(isalpha(tmpVar[i]))
               {
                    isal = TRUE;
               }

          }

          if((postfixE = (EXPRESSION_ELEMENT *) 
                    realloc(postfixE, (numElements + 1) * 
                    sizeof(EXPRESSION_ELEMENT)) ) == NULL)
          {
               fprintf(stderr,"Memory allocation failed\n");
               return 0;
          }

          /* was using strdup */
          postfixE[numElements].item = (char *) malloc(strlen(tmpVar)+1);
          sprintf(postfixE[numElements].item, "%s", tmpVar);


          if(isal == TRUE) 
          {
               postfixE[numElements].isVariable = TRUE;
          }
          else
          {
               postfixE[numElements].isVariable = FALSE;
          }

          tmpVar = strtok(NULL, " ");
          numElements++;
     }

     /*#ifdef DEBUG
     for(i = 0; i < numElements; i++)
     {
          printf("postfixE[%d]->item=%s  postfixE[%d]->isVariable=%d\n", i, postfixE[i].item, i, postfixE[i].isVariable);
     }
     #endif*/

     free(postfixExpression);

     return TRUE;
}



/* Takes an array of doubles and add them into the postfix
   expression in the appropriate place, then it evaluates
   the postfix expression
*/
double calculateExpression(double values[])
{
        char tmpStr[100], calcExpression[2048];
        int index, exCount = 0, valCount = 0, tmpCount, errorLevel;

        #ifdef DEBUG
        printf("in calculateExpression\n");
        #endif


        for(index = 0; index < numElements; index++)
        {
            if(postfixE[index].isVariable == TRUE)
            {
                 sprintf(tmpStr,"%f",values[valCount]);
                 #ifdef DEBUG
                 /*printf("values[%d]=%f\n", valCount, values[valCount]);*/
                 printf("postfixE[%d] is a variable ", index);
                 printf("tmpStr=%s\n", tmpStr);
                 #endif

                 for(tmpCount = 0; tmpCount < strlen(tmpStr); tmpCount++)
                 {
                      calcExpression[exCount++] = tmpStr[tmpCount];
                 }
                 /*printf("valCount=%d\n", valCount);*/
                 valCount++;

            }
            else
            {
                 #ifdef DEBUG
                 printf("postfixE[%d] is not a variable\n", index);
                 #endif 

                 for(tmpCount = 0; tmpCount < strlen(postfixE[index].item); tmpCount++)
                 {
                      calcExpression[exCount++] = postfixE[index].item[tmpCount];                     

                 }
            } 
            calcExpression[exCount++] = ' ';
        } 
        calcExpression[exCount - 1] = '\0'; /* removes the trailing space */

        #ifdef DEBUG
        printf("calcExpression=%s\n", calcExpression);
        #endif

        return  evaluatePostfix(calcExpression);
}

/* Evaluates a postfix expression of doubles */
double evaluatePostfix(char postfix[])
{
     int x, y, spaceCheck, errorLevel = 0;
     char temp[256];
     double op1, op2;
     double FPAnswer;
     DSTACK st;
	 char *prog_name = "Postfix evaluator";

     init_dstack(&st);

/*  keeps track of which operand it is on */
     spaceCheck = 1;  

     FPAnswer = 0.0;
     y = 0;

     for(x = 0; postfix[x] != '\0'; x++)
     {
         if(isalnum(postfix[x]) || postfix[x] == '.')
         {
              spaceCheck = 0;
              temp[y++] = postfix[x];
              temp[y] = '\0';
         }
         else if(postfix[x] == ' ')
         {

              if(spaceCheck == 0)
	      {
                   pushd(atof(temp), &st);
                   #ifdef DEBUG_EVALP
                   printf("%f pushed onto stack\n", atof(temp));
                   #endif
                   spaceCheck = 1;
                   y = 0;
                   temp[0] = '\0';
              }
         }
         else if(postfix[x] == '+')
         {

             op1 = popd(&st);
             #ifdef DEBUG_EVALP 
             printf("%f popped from stack\n", op1);
             #endif

             if(is_emptyd(st) == 0)
             {
                  op2 = popd(&st);
                  #ifdef DEBUG_EVALP
                  printf("%f popped from stack\n", op2);
                  #endif

                  FPAnswer = op2 + op1;

                  pushd(FPAnswer, &st);
                  #ifdef DEBUG_EVALP
                  printf("%f pushed onto stack\n", FPAnswer);
                  #endif
             }
             else 
             {
                  errorLevel = 1;
                  ERROR(prog_name, "Something is wrong near the '+' in your equation", 1);
             }
         }
         else if(postfix[x] == '-')
         {
              if(postfix[x+1] == ' ' || postfix[x+1] == '\0')
              {
                   op1 = popd(&st);
                   #ifdef DEBUG_EVALP
                   printf("%f popped from stack\n", op1);
                   #endif

                   if(is_emptyd(st) == 0)
                   {
                        op2 = popd(&st);
                        #ifdef DEBUG_EVALP
                        printf("%f popped from stack\n", op2);
                        #endif 

                        FPAnswer = op2 - op1;


                        pushd(FPAnswer, &st);

                        #ifdef DEBUG_EVALP
                        printf("%f pushed onto stack\n", FPAnswer);
                        #endif 
                   }
                   else
                   {
                        errorLevel = 2;
                        ERROR(prog_name,
                              "Something is wrong near the '-' in your equation", 1);
                   }
              }
              else /* unary minus */
              {
                   spaceCheck = 0;
                   temp[y++] = postfix[x];
                   temp[y] = '\0';
              }
         }
         else if(postfix[x] == '*')
         {
              op1 = popd(&st);
              #ifdef DEBUG_EVALP
              printf("%f popped from stack\n", op1);
              #endif

              if(is_emptyd(st) == 0)
              {
                   op2 = popd(&st); 
                   #ifdef DEBUG_EVALP
                   printf("%f popped from stack\n", op2);
                   #endif

                   FPAnswer = op2 * op1;


                   pushd(FPAnswer, &st);

                   #ifdef DEBUG_EVALP
                   printf("%f pushed onto stack\n", FPAnswer);
                   #endif
              }
              else
              {
                   errorLevel = 3;
                   ERROR(prog_name,
                       "Something is wrong near the '*' in your equation", 1);
              }
          }
          else if(postfix[x] == '/')
          {
               op1 = popd(&st);
               #ifdef DEBUG_EVALP
               printf("%f popped from stack\n", op1);
               #endif

               if(is_emptyd(st) == 0)
               {
                    op2 = popd(&st);
                    #ifdef DEBUG_EVALP
                    printf("%f popped from stack\n", op2);
                    #endif 

                    FPAnswer = op2 / op1;


                    pushd(FPAnswer, &st);
                    #ifdef DEBUG_EVALP
                    printf("%f pushed onto stack\n", FPAnswer);
                    #endif
               }
               else
               {
                    errorLevel = 4;
                    ERROR(prog_name,
                       "Something is wrong near the '/' in your equation", 1);
               }

           }
           else
           {
                errorLevel = 5;
                ERROR(prog_name,
                    "incorrect character used in mathematical expresion", 1);
           }
     }  /* end of for loop */

     FPAnswer = popd(&st);

     #ifdef DEBUG_EVALP
     printf("Final answer popped from stack=%f\n", FPAnswer); 
     #endif 


     return(FPAnswer);

}   /*  end of evaluate()  */


/* converts all input string text [a-z] to CAPITAL letters
   duplicated
   Note:  This function is already included in parse_weight_attributes.c
          it was duplicated here for testing purposes only --BB
*/
#ifdef EVAL_UNIT
char *convertToUpper(char *input)
{
    int i = 0;

    while(input[i] != '\0')
    {
        if(isalpha(input[i]) )
        {
            input[i] = toupper(input[i]);
        }

        i++;
    }
    return input;
}
#endif

