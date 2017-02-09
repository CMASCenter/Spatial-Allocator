/****************************************************************************
 *  parseAllocModes.c
 *
 *  Functions for the allocation mode file parse
 *  This module uses most of the functionality of parse_weight_attributes.c
 *
 *  Date:  5/13/2005  BDB
 *  Updated: 5/25/2005 BDB -- added support for ALL keyword in lieu of a file
 *  Updated: 6/30/2005 BDB -- added additional ALL support for discrete 
 *                            modes
 ***************************************************************************/
 
#include "parseAllocModes.h"
#include "io.h"

AllocateMode *allocModeArray = NULL;
int allocModeAttribCount = 0; 
 

/* parser for the allocation mode input file (formerly the attribute info file)
   setting allocModeAttribCount = -1 is code for an ALL condition, which must
   be handled by the calling function
*/
int parseAllocModes(char *fileName)
{
  FILE *ifp;
  char str[256];
  
  extern char *prog_name;
  int atCount = 0;
  int hasMode;
  
  int duplicate = 0;
  
  char tmpString[256], mesg[256];
  char name[100], mode[50];

  /* if the fileName is ALL_AGGREGATE or ALL_AVERAGE, do not process the
     allocation mode file, but include all attribs 
   */

  allocModeArray = malloc(1 * sizeof(AllocateMode));   

  if(!strcmp(fileName, "ALL_AGGREGATE"))
  {
       allocModeAttribCount = -1;
       allocModeArray[0].name = (char *) strdup("ALL");
       allocModeArray[0].mode = Aggregate;
       return 0;
   
  
  }
  
  if(!strcmp(fileName, "ALL_AVERAGE"))
  {
       allocModeAttribCount = -1;
       allocModeArray[0].name = (char *) strdup("ALL");
       allocModeArray[0].mode = Average;
       return 0;

  }

  /* added 6/30/2005 */
  if(!strcmp(fileName, "ALL_DISCRETEOVERLAP"))
  {
       allocModeAttribCount = -1;
       allocModeArray[0].name = (char *) strdup("ALL");
       allocModeArray[0].mode = DiscreteOverlap;
       return 0;

  }

  /* added 6/30/2005 */
  if(!strcmp(fileName, "ALL_DISCRETECENTROID"))
  {
       allocModeAttribCount = -1;
       allocModeArray[0].name = (char *) strdup("ALL");
       allocModeArray[0].mode = DiscreteCentroid;
       return 0;
  }

  /* added 10/05/2006 for sea surf zone area percent*/
  if(!strcmp(fileName, "ALL_AREAPERCENT"))
  {
       allocModeAttribCount = -1;
       allocModeArray[0].name = (char *) strdup("ALL");
       allocModeArray[0].mode = AreaPercent;
       return 0;
  }


  if((ifp = fopen(fileName, "r")) == NULL)
  {
     sprintf(mesg, "Unable to open allocation mode file %s", fileName);
     ERROR(prog_name, mesg, 1); 

  }    
      
  MESG2("Allocation mode file name: ", fileName); 

  while(fgets(tmpString, sizeof(tmpString), ifp) != NULL)
  {
      trim(tmpString, str);
         
      if ((strlen(str) == 0) || str[0] == '#' || str[0] == '\n' || str[0] == '\r') 
      {
         continue;   /* ignore comment or empty  lines */
      }   
    
      if(strstr(str, "ATTRIBUTE"))
      {
         allocModeAttribCount++;
      }
  }
  fclose(ifp);   
  
  allocModeArray = malloc(allocModeAttribCount * sizeof(AllocateMode));   
   
   if((ifp = fopen(fileName, "r")) == NULL)
   {
       sprintf(mesg, "Unable to allocation mode file %s", fileName);
       ERROR(prog_name, mesg, 1); 
   }     
   
   while(fgets(tmpString, sizeof(tmpString), ifp) != NULL)
   {
         /* trim the string */
         trim(tmpString, str);
         
         /* ignore comments and blank lines */
         if ((strlen(str) == 0) || str[0] == '#' || str[0] == '\n' || str[0] == '\r') 
         {
            continue;   
         }
          
         convertToUpper(str);  
         
         if(strstr(str, "ATTRIBUTE"))
         {
                    
            if(getValues(str, name, mode))
            {
               /* fix the error message */
               sprintf(mesg, "Syntax error in %s, line %s", fileName, str);
               ERROR(prog_name, mesg, 1); 
            }

            duplicate = checkMultipleModes(name, atCount);
            
 
            if(duplicate == 1)
            {
              sprintf(mesg, "Duplicate ATTRIBUTE detected in %s", fileName);
              ERROR(prog_name, mesg, 1); 
            }

            allocModeArray[atCount].name = (char *) strdup(name);

            if(!strcmp(mode, "Aggregate") || !strcmp(mode, "AGGREGATE"))
            {
                allocModeArray[atCount].mode = Aggregate;
            }
            else if(!strcmp(mode, "Average") || !strcmp(mode, "AVERAGE"))
            {
                allocModeArray[atCount].mode = Average;
            }
            else if(!strcmp(mode, "DiscreteOverlap") || 
                       !strcmp(mode, "DISCRETE_OVERLAP") ||
                       !strcmp(mode, "DISCRETEOVERLAP") )
            {
                allocModeArray[atCount].mode = DiscreteOverlap;
            }
            else if(!strcmp(mode, "DiscreteCentroid") || 
                       !strcmp(mode, "DISCRETECENTROID") ||
                       !strcmp(mode, "DISCRETE_CENTROID") )
            {
                allocModeArray[atCount].mode = DiscreteCentroid;
            }
            else
            {
                sprintf(mesg,"Unknown processing mode (%s) detected", mode);
                ERROR(prog_name, mesg, 1);
            }

            sprintf(mesg,"line=%d name=%s  mode=%s", atCount,name,mode);
            MESG2("line: ",mesg);

            atCount++; 
          
         }
     
    }
    return 0;
}


/* borrowed from parseWeightAttributes.c, but much simplified for this purpose */
/* format is be ATTRIBUTE=<name>:<mode> */
int getValues(char *input, char *name, char *mode)
{    
    int i = 0;
    int len = strlen(input);
    
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

    #ifdef DEBUG_ALLOC_MODES
    fprintf(stderr,"RHS='%s'\n",input);
    #endif

    /* at this point, line is everything after the '=' */
    len = strlen(input); /* have to get new length */
    i = 0;
    while((input[0] != ':') && i < len)
    {
        name[i] = input[0];
        input++;
        i++; 

    }
    name[i] = '\0';
 
    if(i == len)
    {
         return 1; /* error condition */
    }

    input++; /* move past the ':' character */

    i = 0;
    while((input[0] != '\0') && i < len)
    {
        mode[i] = input[0]; 
        input++;
        i++;
    }
    mode[i] = '\0';

    #ifdef DEBUG_ALLOC_MODES
    fprintf(stderr, "name=%s mode=%s\n", name, mode);
    #endif

    return 0;
  
}


/* retrieves the mode for a given attribute name */
/* or returns mode when "ALL" keyword is in play */
/* returns "NotFound" if name does not exist */
amode getMode(char *name)
{
    int i;
    char mesg[256];


    /* this is the ALL_AVERAGE or ALL_AGGREGATE case */
    if(allocModeAttribCount == -1)
    {
         //sprintf(mesg,"ALLOCTE: name=%s  mode=%d", name, allocModeArray[0].mode);
         //MESG2("Variable: ", mesg);
         return allocModeArray[0].mode;
    }
    else
    {
   
         for(i = 0; i < allocModeAttribCount; i++)
         {
            
             //sprintf(mesg,"ALLOC: name=%s  mode=%d", allocModeArray[i].name, allocModeArray[i].mode);
             //MESG2("line: ",mesg);
 
             if(!strcmp(allocModeArray[i].name, name))
             {
                 //sprintf(mesg,"ALLOCTE: name=%s  mode=%d", name, allocModeArray[i].mode);
                 //MESG2("Variable: ",mesg);
                 return allocModeArray[i].mode;
             }
     
         }
   
    }
    return NotFound;


}

/* Checks for duplicate attribute name and mode
   returns 1 "fail" if duplicate exists
   returns 0 "success" if no duplicates were encountered
*/
int checkMultipleModes(char *newItem, int currentCount)
{
    int count;
    char mesg[100];
    
    for(count = 0; count < currentCount; count++)
    {
       /* checks for duplicate attribute names */
       if(!strcmp(allocModeArray[count].name, newItem))
       {
            sprintf(mesg,"ATTRIBUTE=%s appears more than once in the allocation mode file", newItem);
            WARN(mesg);   
             return 1;
       }
     }  

    return 0;
}

/* simple memory freeing routine invoked when we are done with the
   allocModeArray
*/
void cleanUpAllocModes()
{
     int i;
     
     for(i = 0; i < allocModeAttribCount; i++)
     {
          free(allocModeArray[i].name);
     }
    
     free(allocModeArray);
	 
}



#ifdef ALLOC_MODE_TEST
/* main function for testing purposes*/
char *prog_name = "Alloc Mode Tester";

int main(int argc, char *argv[])
{
    int retVal, i;
    extern int debug_output;
    
    debug_output = 1;
    
    retVal = parseAllocModes(argv[1]);

    for(i = 0; i < allocModeAttribCount; i++)
    {
        printf("name=%s mode=%d\n", 
             allocModeArray[i].name, allocModeArray[i].mode);     
         
    }
    return EXIT_SUCCESS;
} 

#endif
