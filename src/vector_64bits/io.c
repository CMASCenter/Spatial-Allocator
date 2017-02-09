/* Error and warning reporting functions for use by the MIMS Spatial Tool.
 * Developed by Atanas Trayanov of MCNC Environmental Modeling Center in
 * support of the EPA Multimedia Integrated Modeling System, 2002 */
 
/* updates:
 * 3/31/2005 -- added new getEnvtValue function BDB
 * 4/1/2005 -- moved trim function into this module  BDB 
 * 4/22/2005 -- added the digiCheck utility function BDB
 * 5/13/2005 -- moved convertToUpper in from parseWeightAttributes.c BDB
 * 7/05/2005 -- added strNullTerminate and strBlankCopy CAS
 */

#include <stdio.h>
#include <stdlib.h>
#include "io.h"

int debug_output = 0;

/* ================================================== */
int WARN(char *msg)
{
  if(debug_output)
  {
      fprintf(stderr,"WARNING: %s\n", msg);
  }
  return 0;
}

/* ================================================== */
int WARN2(char *msg1, char *msg2)
{
  if(debug_output)
  {
       fprintf(stderr,"WARNING: %s %s\n", msg1, msg2);
  }
  return 0;
}

/* ================================================== */
int MESG(char *msg)
{
  if(debug_output)
  {
       fprintf(stdout,"%s\n", msg);
  }
  return 0;
}

/* ================================================== */
int MESG2(char *msg1, char *msg2)
{
  if(debug_output)
  {
       fprintf(stdout,"%s %s\n", msg1, msg2);
  }

  return 0;
}

/* ================================================== */
void ERROR(char *pgm, char *msg, int errcode)
{
  
  fprintf(stderr,"ERROR in %s: %s. Exiting...\n", pgm, msg);
  exit(errcode);
}

/* ================================================== */
void ERROR_NO_EXIT(char *pgm, char *msg)
{
  
  fprintf(stderr,"ERROR in %s: %s. ", pgm, msg);
}
/* ================================================== */

/* retrieves environment variable of evName and stores it in evValue */
/* returns success 1 or failure 0 */
int getEnvtValue(char *evName, char *evValue)
{
  char mesg[256];

  if(evName == NULL)
  {
     evValue[0] = '\0';
     /*sprintf(mesg, "Environment variable: %s, not set",evName);
     WARN(mesg);*/
     return 1;
  }

  if(getenv(evName) != NULL)
  {
     strcpy(evValue, getenv(evName));
     sprintf(mesg, "EV: %s=%s",evName, evValue);
     MESG(mesg);
     return 1;
  }
  
  /* evValue = NULL; or should it be evValue[0] = '\0';??? */
  evValue[0] = '\0';
  sprintf(mesg, "Environment variable: %s, not set",evName);
  WARN(mesg);
  return 0;
} 

/*
 * trims the spaces from before and after the string
 */
void trim(char *inString, char *outString)
{
   int start; /* the index of the first non-space character */
   int end;   /* the index of the last non-space character */
   int length; /* then length of the trimmed string */
   
   for (start = 0; start < strlen(inString); start++)
   {
      if ( (inString[start] != ' ') && (inString[start] != '\t'))
           /*&& (inString[start] != '(') )*/
      {
         break;
      }
   }
   for (end = strlen(inString)-1; end >=0; end--)
   {
      if (((inString[end] != ' ') && (inString[end] != '\t'))
           && (inString[end] != '\n') && (inString[end] != '\r'))
      {
         break;
      }
   } 
   length = end - start + 1;
   if (end >= start)
   {
      strncpy(outString, &inString[start], length);
      outString[end+1] = '\0';
   }
   else
   {
      outString[0] = '\0';
   }
   /*fprintf(stderr,"in string = '%s', start = %d end = %d length = %d\n",
     inString, start, end, length);
     fprintf(stderr,"trimmed string = '%s'\n",outString);
   */
}

/* checks to see if a string that is read in can be converted to an int or
   a double or if there are alphabetic characters encountered
  
   returns 0 if okay to convert, returns 1 if non-numeric chars are found
*/
int digiCheck(char *val)
{
     int len;
     

     len = strlen(val);

     while(val[0] != '\0')
     {
         if(!isdigit(val[0]) && val[0] != '.' && val[0] != '-')
         {
             /* check for well-formedness of exponential notation */
             if(val[0] != 'E')
             {
                 return 1;
             }
             else if((val[0] == 'E' || val[0] == 'e' ) && 
                   isdigit(val[1]) && 
                   (isdigit(val[2]) || val[2] == '\0') )
                  
             {
                return 0;
             }
             else if((val[0] == 'E' || val[0] == 'e') && 
                   (val[1] == '-' || val[1] == '+')   && 
                   isdigit(val[2]) &&  
                   (isdigit(val[3]) || val[3] == '\0') ) 
             {
                 return 0;
             }

             
         }
         val++;

     }

     return 0;

}


/* converts all input string text [a-z] to CAPITAL letters
   duplicated
*/
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

/* creates a null terminated string from a blank padded string */

int strNullTerminate(char *dest, const char *src, int n)
{
    int i;
    
    for(i = n-1; i >= 0; i--)
    {
        if(src[i] != ' ') break;
    }
    strncpy(dest, src, n);
    dest[i+1] = '\0';
	return 0;
}

/* creates a blank padded string from a null terminated string */

int strBlankCopy(char *dest, const char *src, int n)
{
    int src_len;

    src_len = strlen(src);
    
    strncpy(dest, src, n);
    if(src_len < n)
    {
        memset(&dest[src_len], ' ', n - src_len);
    }
    
    return 0;
}
