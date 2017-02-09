/*****************************************************************************************
 * strlistc.c 
 * A support routine for MIMS Spatial tool, strlistc tokenizes a comma separated list
 * of attributes and places the list entries into an array
 *
 *
 * Updated: April 2005, changed how environment vars. are read -- BDB
 *          Also got rid of all goto lines and added appropriate error messages
 *
 *****************************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "io.h"

#include "mims_spatl.h"

int strlistc(char *ename, char *edesc, int *nmax, int *ncnt, char ***list )
{

   int n;
   int max_size = 4;
   char *tmp;
   char **list_array;
   char mesg[256], str[100];
   extern char *prog_name;


   list_array = (char **)malloc(max_size*sizeof(char *));
   if(!list_array) 
   {
     sprintf(mesg,"Memory allocation error in STRLISTc");
     *ncnt = 0;
     *nmax = 0;
     *list = NULL;
     sprintf(mesg, "Unable to allocate memory for unique attribute value string array");
     ERROR(prog_name, mesg, 2);
   }

   n = 0;


   if(!getEnvtValue(ename, str))
   {
       sprintf(mesg, "Unable to continue because %s has not been set", ename);
       ERROR(prog_name, mesg, 2);
   }

   tmp = strtok(str, ",");
   while(tmp != NULL) 
   {

         if(n >= max_size) 
         {
	     max_size *= 2;
	     list_array = (char **)realloc(list_array,max_size*sizeof(char *));
	     if (!list_array) 
             {
	         sprintf(mesg,"Memory re-allocation error in STRLISTc");
                 *ncnt = 0;
                 *nmax = 0;
                 *list = NULL;
                 ERROR_NO_EXIT(prog_name, mesg);
                 return 0;
	     }
           }
           list_array[n] = (char *) strdup(tmp); 
           tmp = strtok(NULL,",");
           n++;
   }
     

   *ncnt = n;
   *nmax = max_size;
   *list = list_array;
   return 1;

}

