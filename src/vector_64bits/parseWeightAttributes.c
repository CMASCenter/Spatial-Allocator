/***************************************************************************
                          parseWeightAttributes.c  -  description
                             -------------------
    begin                : Wed Nov  3 13:47:44 EST 2004
    copyright            : (C) 2004 by Ben Brunk
    email                : brunk@unc.edu
 ***************************************************************************/

/***************************************************************************
 *                                                                         
 *   This code is for the MIMS Spatial Allocator Enhancement
 *                                                                         
 *   Updated 1/4/2005
 *   Updated 1/5/2005  -- bug fixes BDB 
 *   Updated 5/13/2005 -- removed error and mesg debugging functions because
 *                        those were moved to io.h  BDB
 ***************************************************************************/



#include "parseWeightAttributes.h"
#include "io.h"
                                 
/* global variables */
AttributeInfo *attribArray = NULL;

/* NOTE:  totalAttribs is kind of a misnomer the way I wrote it
      error condition.  --BB

*/
int totalAttribs = -1;  


/* captures only the right half of the string after the first '='
 AME: changed to ignore white space after =
*/
char *getAttribs(char *input)
{    
    int i = 0;
    int len = strlen(input);
    
    #ifdef DEBUG_PWA
    fprintf(stderr,"getAttribs(%s)",input);
    #endif
    
    while((input[0] != '=') && (i < len))
    {
      input++;
      i++;
    }
    input++; /* move past the '=' char */
    i++;
    
    /* skip any white space past the = char */
    while(((input[0] == ' ') || (input[0] == '\t')) && (i < len))
    {
      input++;
      i++;
    }
    #ifdef DEBUG_PWA
    fprintf(stderr,"RHS='%s'\n",input);
    #endif
    return input;
  
}


/* Checks for duplicate attribute name, type, include and exclude lines
   returns 1 "fail" if duplicate exists
   returns 0 "success" if no duplicates were encountered
*/
int checkDuplicates(char *newItem)
{
    int count;

    for(count = 0; count <= totalAttribs; count++)
    {
        
        /* checks for duplicate name */
        if(strcmp(attribArray[count].name, newItem) == 0)
        {
          return 1;
        }
     }  

    return 0;
}

/* checks for things like multiple includes and excludes for the same attribute */
int checkMultiple(AttributeInfo *attributeArray, int currentAttrib, int newItemType)
{
    switch(newItemType)
    {
        /* checks for multiple includes */
        case 0: if(attributeArray[currentAttrib].includeValues != NULL)
                {
                    return 1;
                }
                break;
        /* checks for multiple excludes */
        case 1: if(attributeArray[currentAttrib].excludeValues != NULL)
                {
                    return 1;
                }
                break;
        case 2: if(attributeArray[currentAttrib].type != NULL)
                {
                    return 1;
                }
                break;
       
    }

    return 0;
}


void cleanUp()
{
     int i;
     for(i = 0; i <= totalAttribs; i++)
     {
          free(attribArray[i].name);
          free(attribArray[i].type);
          free(attribArray[i].includeValues);
          free(attribArray[i].excludeValues);      
          free(attribArray[i].includes);
          free(attribArray[i].excludes);
     }

     /* added these so that cppunit can call this module multiple times */
     totalAttribs = -1;   
     attribArray = NULL;

     #ifdef DEBUG_PWA
     fprintf(stderr,"CleanUp function completed\n");
     #endif
	 
}

int parseWeightSubsets(char *filename)
{

  FILE *ifp;
  char str[256];
  
  int attribCount = 0, hasIncludes = FALSE, hasExcludes = FALSE, hasType = FALSE;
  
  int duplicate = 0;
  
  char tmpString[256], mesg[256];
  char *returnString;

  totalAttribs = -1;
 
  if((ifp = fopen(filename, "r")) != NULL)
  {
      MESG2("Filter filename: ", filename);

      while (fgets(tmpString, sizeof(tmpString), ifp) != NULL)
      {
         trim(tmpString, str);
         
         if ((strlen(str) == 0) || str[0] == '#' || str[0] == '\n' || str[0] == '\r') 
         {
            continue;   /* ignore comment or empty  lines */
         }   
         
         if(strstr(str, "ATTRIBUTE_NAME"))
         {
            attribCount++;
         }

      }
      fclose(ifp);

      /*printf("Number of attributes: %d\n", attribCount);*/
      attribArray = malloc(attribCount * sizeof(AttributeInfo));
      
  }
  /* added check 1/5/2005  BB */
  else
  {
       ERROR(filename,"Unable to read filter file", 1);

  }

  /* now we know how many attributes there are, so we can build a structure in memory for them
     and then re-open the file */

  
  if((ifp = fopen(filename, "r")) != NULL)
  {
      while (fgets(tmpString, sizeof(tmpString), ifp) != NULL)
      {
         /* trim the string */
         trim(tmpString, str);
         
         /* ignore comments and blank lines */
         if ((strlen(str) == 0) || str[0] == '#' || str[0] == '\n' || str[0] == '\r') 
         {
            continue;   
         }
          
         convertToUpper(str);  

         if(strstr(str, "ATTRIBUTE_NAME"))
         {
            returnString = getAttribs(str);

            #ifdef DEBUG_PWA
            fprintf(stderr,"Returned from getAttribs\n");
            #endif

            if(returnString[0] == '\0')
            {
               ERROR(filename, MISSING_ATTRIB_NAME, 1);
               return 1;
            }
            duplicate = checkDuplicates(returnString);

            #ifdef DEBUG_PWA
            fprintf(stderr,"Checked for duplicates\n");
            #endif

            if(duplicate == 1)
            {
              ERROR(filename, DUPLICATE_ATTRIB_NAME, 1);
              return 2;
             
            }

            /* added error checking 1/10/2005 BB */
            if(totalAttribs < 0 || hasIncludes || hasExcludes)
            {
                 hasIncludes = FALSE;
                 hasExcludes = FALSE;
            }
            else
            {
                 sprintf(mesg, "ATTRIBUTE_NAME=%s %s", 
                      attribArray[totalAttribs].name, MISSING_INCEX_LINES);
                 ERROR("Filter File", mesg, 1);
                 return 8; /* will only return when testing */
            }
           
            if(totalAttribs < 0 || hasType)
            {
                 hasType = FALSE;
            } 
            else
            {
                 sprintf(mesg, "ATTRIBUTE_NAME=%s %s", 
                     attribArray[totalAttribs].name, MISSING_ATTRIB_TYPE_LINE);
                 ERROR("Filter File", mesg, 1);
                 return 9;
            }

            totalAttribs++;

            attribArray[totalAttribs].name = (char *) strdup(returnString);
            /* attribArray[totalAttribs].name = malloc(strlen(returnString)+1);
               strcpy(attribArray[totalAttribs].name, returnString); */

            /* initialize empty space*/
            attribArray[totalAttribs].type = NULL;
            attribArray[totalAttribs].includeValues = NULL;
            attribArray[totalAttribs].excludeValues = NULL;
            attribArray[totalAttribs].includes = NULL;
            attribArray[totalAttribs].excludes = NULL;

         }
         else if(strstr(str, "ATTRIBUTE_TYPE"))
         {
            hasType = TRUE;
            returnString = getAttribs(str);

            if(returnString[0] == '\0')
            {
                ERROR(filename, MISSING_ATTRIB_TYPE, 1);
                return 3;
                
            }

            duplicate = checkMultiple(attribArray, totalAttribs, 2);

            if(duplicate == 1)
            {
                ERROR(filename, MULTIPLE_ATTRIB_TYPE_ERROR, 1);
                return 4;
            }

            /* AME: changed to use strdup instead */
            attribArray[totalAttribs].type = (char *) strdup(returnString);
            /*attribArray[totalAttribs].type = malloc(strlen(returnString)+1);
            strcpy(attribArray[totalAttribs].type, returnString);*/
             
         }  
         else if(strstr(str, "INCLUDE_VALUES"))
         {
            hasIncludes = TRUE;
            returnString = getAttribs(str);

            if(returnString[0] == '\0')
            {
                WARN(INCLUDE_VALS_NO_ARGS);
            }
                     
            duplicate = checkMultiple(attribArray, totalAttribs, 0);
            
            if(duplicate == 1)
            {
                ERROR(filename, MULTIPLE_INCLUDES, 1);
                return 5;
              
            }
            /* AME: changed to use strdup instead*/
            attribArray[totalAttribs].includeValues = (char *) strdup(returnString);
            
            /*attribArray[totalAttribs].includeValues = malloc(strlen(returnString)+1);
            strcpy(attribArray[totalAttribs].includeValues, returnString); */
         }
         else if(strstr(str, "EXCLUDE_VALUES"))
         {
            hasExcludes = TRUE;
            returnString = getAttribs(str);

            if(returnString[0] == '\0')
            {
                WARN(EXCLUDE_VALS_NO_ARGS);
            }

            duplicate = checkMultiple(attribArray, totalAttribs, 1);

            if(duplicate == 1)
            {
                ERROR(filename, MULTIPLE_EXCLUDES, 1);
                return 6;
              
            }
            attribArray[totalAttribs].excludeValues = malloc(strlen(returnString)+1);
            strcpy(attribArray[totalAttribs].excludeValues, returnString);
         }
         else /*  error condition */
         {
             ERROR(filename, UNRECOGNIZED_KEYWORD, 1);
             return 7;
         }    
     }

     /* added error checking 1/10/2005 BB */
     if(!hasIncludes && !hasExcludes)
     {
            sprintf(mesg, "ATTRIBUTE_NAME=%s %s", 
            attribArray[totalAttribs].name, MISSING_INCEX_LINES);
            ERROR("Filter File", mesg, 1);
            return 8; /* will only return when testing */
     }
           
     if(!hasType)
     {
          sprintf(mesg, "ATTRIBUTE_NAME=%s %s", 
              attribArray[totalAttribs].name, MISSING_ATTRIB_TYPE_LINE);
          ERROR("Filter File", mesg, 1);
          return 9;
     }
     fclose(ifp);
  }    
         
  return 0;
}

#ifdef SIMPLEMAIN
/* main function for testing purposes*/
int main(int argc, char *argv[])
{
    int retVal;
    
    retVal = parseWeightSubsets(argv[1]);

    return EXIT_SUCCESS;
} 

#endif
