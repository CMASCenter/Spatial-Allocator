/***************************************************************************
                          parse_include_exclude.c  -  description
                             -------------------
    begin                : Thu Nov 11 2004
    copyright            : (C) 2004 by Benjamin Brunk
    email                : brunk@unc.edu
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *  This software is part of the enhancements to the MIMS Spatial Allocator                       
 *                                                                         *
 ***************************************************************************/
#include <ctype.h> /* added 1/6/2005 BB */
#include <regex.h>
#include "parseWeightAttributes.h"
#include "shapefil.h"
#include "io.h"


/* returns the number of items on either on an INCLUDE_VALUES or an
 * EXCLUDE_VALUES line */
int getTokenCount(char *list)
{
     int count = 0;

     if(list == NULL) return 0;
     
     if(list[0] == '\0') return 0;
     
     while(list[0] != '\0')
     {
          if(list[0] == ',') count++;

          list++;
     }
     count++;

     return count;
 
}


int parseList(char *list, AttribListItem *attribListItems)
{
    int bufCount = 0, listIndex = 0;
    char *ptr, buffer[100];

    AttribListItem ali;
    
    if(list[0] == '\0' || list == NULL) return 0;

    ptr = list;

    while(ptr[0] != '\0')
    {
        if(ptr[0] == ',')
        {
            /* buffer[bufCount] = '\0'; */
            parseItem(buffer, &ali);

            
            strcpy(attribListItems[listIndex].op, ali.op);
            attribListItems[listIndex].leftOperand  = ali.leftOperand;
            attribListItems[listIndex].rightOperand = ali.rightOperand;
 

            listIndex++;

            /* reset for next item */
            bufCount = 0;
        }
        else
        {
            buffer[bufCount] = ptr[0];
            bufCount++;
            buffer[bufCount] = '\0';
            

            if(bufCount >= 99) ERROR("buffer overflow",
                                 " include/exclude line too long", 1);
        }
        ptr++;

    }
    /*buffer[bufCount] = '\0';*/
    parseItem(buffer, &ali);
    
    /*attribListItems[listIndex] = malloc(sizeof(AttribListItem));*/
    strcpy(attribListItems[listIndex].op, ali.op);
    attribListItems[listIndex].leftOperand  = ali.leftOperand;
    attribListItems[listIndex].rightOperand = ali.rightOperand;

    return 1;

}


int parseAndSaveFilters()
{
     int index, tCountInc, tCountExc, continuous = 0, overlaps = 0;

     for(index = 0; index <= totalAttribs; index++)
     {
          if(strcmp(attribArray[index].type, "CONTINUOUS") == 0)
          {
               continuous = 1;
               tCountInc = getTokenCount(attribArray[index].includeValues);
               tCountExc = getTokenCount(attribArray[index].excludeValues);

               if(tCountInc > 0)
               {
                    attribArray[index].includes = malloc(tCountInc * sizeof(AttribListItem));

                    parseList(attribArray[index].includeValues, attribArray[index].includes);
                     
               }

               if(tCountExc > 0)
               {
                    attribArray[index].excludes = malloc(tCountExc * sizeof(AttribListItem));
                    
                 
                    parseList(attribArray[index].excludeValues, attribArray[index].excludes);
                    
               }
          }
     }  /* end for */

     if(continuous)
     {
          overlaps = checkForOverlaps();

          if(overlaps == 1)
          {
               ERROR("INCLUDE_VALUES", "Overlapping ranges detected", 1);
               return 0;
          }

          if(overlaps == 2)
          {
               ERROR("EXCLUDE_VALUES", "Overlapping ranges detected", 1);
               return 0;
          }
     }
     return 1;
}


void parseItem(char *item, AttribListItem *ali)
{
     char *ptr, leftOperand[25], rightOperand[25];
     int range, lCount = 0, rCount = 0, switchOver = 0, lt = 0;
     int found = FALSE; /* added 1/6/2005 BB */


     ali->leftOperand = MINRANGEVAL;
     ali->rightOperand = MAXRANGEVAL;

     ptr = item;

     /* trim leading blanks, if any */
     while(ptr[0] == ' ')
     {
          ptr++;
     } 
     
     /* check for errors added 1/6/2005 BB */
     while(ptr[0] != '\0')
     {
     
         if(!isdigit(ptr[0]) && ptr[0] != '<' && ptr[0] != '>' && ptr[0] != '=' && ptr[0] != '-' )
         {
              ERROR("filter file", 
                 "alphabetic character or unknown operator found on include or exclude line under ATTRIBUTE_TYPE=CONTINUOUS", 1);
         }
         
         ptr++;
     }

     ptr = item;
     
     if(ptr[0] == '<')      
     {
          ali->op[0] = ptr[0];
          ali->op[1] = '\0'; 
          
          if(ptr[1] == '=')
          {
               ali->op[1] = ptr[1];
               ali->op[2] = '\0';     
          }
          range = 0;
          lt = 1;
          
     }
     else if(ptr[0] == '>')
     {
          ali->op[0] = ptr[0];
          ali->op[1] = '\0';

          if(ptr[1] == '=')
          {
               ali->op[1] = ptr[1];
               ali->op[2] = '\0';
          }
          range = 0;
          lt = 0;     


     }
     /* changed 1/6/2005 to include error checks BB */
     else
     {
          found = FALSE;
          while(ptr[0] != '\0')
          {
               if(ptr[0] == '-')
               {
                    found = TRUE;
               }
               ptr++;
          }
          if(found == TRUE)
          {
               ali->op[0] = '-'; /* original 3 lines */
               ali->op[1] = '\0';
               range = 1;
          }
          else
          {
               ERROR("filter file", 
                 "Missing operator in include or exclude line under ATTRIBUTE_TYPE=CONTINUOUS", 1);
          }
     }
     /* end of changes */

     ptr = item;
     
     if(range == 1)
     {
          while(ptr[0] != '\0')
          {
               switch(ptr[0])
               {
                    case '0':
                    case '1':
                    case '2':
                    case '3':
                    case '4':
                    case '5':
                    case '6':
                    case '7':
                    case '8':
                    case '9':
                    case '.': if(switchOver == 0)
                              {
                                   leftOperand[lCount] = ptr[0];
                                   lCount++;
                                   leftOperand[lCount] = '\0';
                              }
                              else
                              {
                                   rightOperand[rCount] = ptr[0];
                                   rCount++;
                                   rightOperand[rCount] = '\0';
                              }
                              break;

                    case '-': switchOver = 1;
                              break;
                              
                    default:  break;

               }

               ptr++;
          }

          ali->leftOperand = (double) atof(leftOperand);
          ali->rightOperand = (double) atof(rightOperand);
     }
     else
     {
          while(ptr[0] != '\0')
          {
               switch(ptr[0])
               {
                    case '0':
                    case '1':
                    case '2':
                    case '3':
                    case '4':
                    case '5':
                    case '6':
                    case '7':
                    case '8':
                    case '9':
                    case '.': if(lt == 1)

                              {
                                   rightOperand[rCount] = ptr[0];
                                   rCount++;
                                   rightOperand[rCount] = '\0';
                              }
                              else
                              {
                                   leftOperand[lCount] = ptr[0];
                                   lCount++;
                                   leftOperand[lCount] = '\0';
                              }

                              break;
                    default:  break;

               }

               ptr++;
          }

          if(lt == 1)
          {
               ali->rightOperand = atof(rightOperand);
          }
          else
          {
               ali->leftOperand = atof(leftOperand);
          }
     }
}


/* checks to see if the name exists in the attribArray structure
 * if it does, returns the index to that item, otherwise it returns -1 */
int checkForNameAttrib(char * pszName)
{
     int index;

     for(index = 0; index < totalAttribs; index++)
     {
          if(!strcmp((char *) attribArray[index].name, (char *) convertToUpper(pszName)))
               return index;  /* found it! */

     }
     return -1;  /* not found */
}

/* returns 1 if item is on include list or within include params such as a range
   first argument determines whether we are checking the includeValues
   or the excludeValues of attribArray */

int checkIncludeExclude(int includesOrExcludes,
         int index, char *val, DBFFieldType varType)
{
    int bufCount = 0, regexMatch;
    char *ptr, buffer[100], mesg[256];
    int returnVal = 0, cindex, ctotal, insideQuote = FALSE;
    char *regexCompileDone;

    struct re_pattern_buffer pbuf;

    if(includesOrExcludes == 1)   /* do the includes*/
    {
         if(attribArray[index].includeValues == NULL) 
         {
            return 1; 
         }
         if(attribArray[index].includeValues[0] == '\0') 
         {
            return 1;  /* this way, a blank INCLUDE_VALUES line or no line*/
                       /* will have the same "include all" effect*/
         }              
         ptr = attribArray[index].includeValues;
    }
    else                           /* do the exludes*/
    {
         if (attribArray[index].excludeValues == NULL)
         {
            return 0;
         }
         if(attribArray[index].excludeValues[0] == '\0') 
         {
            return 0;  /* this way, a blank EXCLUDE_VALUES line or no line*/
                       /* will have the same "exclude none" effect*/
         }              
         
         ptr = attribArray[index].excludeValues;                    
 
    }
    /*printf("name: %s ", attribArray[index].name);*/
    /*printf("type: %s ", attribArray[index].type);*/

    /*printf("vals: %s\n", ptr);*/

    if(strcmp(attribArray[index].type, "DISCRETE") == 0)
    {
         while(ptr[0] != '\0')
         {

              /* check if there are quotes around the string*/
              if(ptr[0] == '"')
              {
                   insideQuote = (insideQuote == TRUE) ? FALSE : TRUE;
                   ptr++;
              }

              if(ptr[0] == ',' && insideQuote == FALSE)
              {
                  pbuf.syntax = RE_SYNTAX_GREP;  /* use USE_DW_FILE-style r.e. syntax*/
                  pbuf.allocated = 8;
                  pbuf.buffer = malloc(pbuf.allocated);
                  pbuf.fastmap = 0;
                  pbuf.translate = 0;

                  #ifdef DEBUG
                  printf("regex buffer=%s\n", buffer);
                  #endif

                  regexCompileDone = (char *) re_compile_pattern(buffer,
                       strlen(buffer), &pbuf);
                  #ifdef DEBUG
                  printf("regexCompileDone=%d\n", regexCompileDone);
                  #endif

                  regexMatch =  re_match(&pbuf, convertToUpper(val),
                       strlen(val), 0, 0);

                  if(regexMatch > 0)
                  {
                      #ifdef DEBUG
                      fprintf(stderr,"Regular Expression matched\n");
                      #endif
                      return TRUE;
                  }
                  /* reset for next item */
                  bufCount = 0;
                }
                /* 1/6/2005 BB --can't do this or regex is busted */
                /*else if(ptr[0] == '<' || ptr[0] == '>')
                {
                    if(includesOrExcludes == 1)
                    {
                         sprintf(mesg, 
                             "ATTRIBUTE_NAME=%s contains non-discrete operators, mismatched ATTRIBUTE_TYPE and INCLUDE_VALUES", 
                             attribArray[index].name);
                         ERROR("in filter file", mesg, 1);
                    }
                    else
                    {

                         sprintf(mesg, 
                             "ATTRIBUTE_NAME=%s contains non-discrete operators, mismatched ATTRIBUTE_TYPE and EXCLUDE_VALUES", 
                             attribArray[index].name);
                         ERROR("in filter file", mesg, 1);
                    }

                }*/
                else
                {
                     buffer[bufCount] = ptr[0];
                     bufCount++;
                     buffer[bufCount] = '\0';
                }
                ptr++;
           } /* end while */

           /* process the last one (no commas or no more commas) */

           pbuf.syntax = RE_SYNTAX_GREP;   /* use USE_DW_FILE-style r.e. syntax */
           pbuf.allocated = 8;
           pbuf.buffer = malloc(pbuf.allocated);
           pbuf.fastmap = 0;
           pbuf.translate = 0;

           #ifdef DEBUG
           printf("regex buffer=%s\n", buffer);
           #endif

           regexCompileDone = (char *) re_compile_pattern(buffer, strlen(buffer), &pbuf);
           #ifdef DEBUG
           printf("regexCompileDone=%d\n", regexCompileDone);
           #endif

           regexMatch = re_match(&pbuf, (char *) convertToUpper(val), strlen(val), 0, 0);
           #ifdef DEBUG
           printf("regexMatch=%d\n", regexMatch);
           #endif

           if(regexMatch > 0)
           {
               #ifdef DEBUG
               fprintf(stderr,"Regular Expression matched\n");
               #endif
               return TRUE;
           }
    }
    else if(strcmp(attribArray[index].type, "CONTINUOUS") == 0)
    {
         if(includesOrExcludes == 1)
         {
              ctotal = getTokenCount(attribArray[index].includeValues);

              for(cindex = 0; cindex < ctotal; cindex++)
              {
                   /*printf("op=%s left=%f val=%f right=%f\n",
                                attribArray[index].includes[cindex].op,
                                attribArray[index].includes[cindex].leftOperand,
                                atof(val),
                                attribArray[index].includes[cindex].rightOperand);
                   */
                   if(strcmp(attribArray[index].includes[cindex].op, "<") == 0)
                   {
                        if(atof(val) < attribArray[index].includes[cindex].rightOperand)
                        {
                             return 1;
                        }
                   }
                   else if(strcmp(attribArray[index].includes[cindex].op, "<=") == 0)
                   {
                        if(atof(val) <= attribArray[index].includes[cindex].rightOperand)
                        {
                             return 1;
                        }
                   }
                   else if(strcmp(attribArray[index].includes[cindex].op, ">") == 0)
                   {
                        if(atof(val) > attribArray[index].includes[cindex].leftOperand)
                        {
                            return 1;
                        }
                   }
                   else if(strcmp(attribArray[index].includes[cindex].op, ">=") == 0)
                   {

                        if(atof(val) >= attribArray[index].includes[cindex].leftOperand)
                        {
                            return 1;
                        }
                   }
                   else if(strcmp(attribArray[index].includes[cindex].op, "-") == 0)
                   {
                       #ifdef DEBUG
                       /*fprintf(stderr, "left operand=%f, right operand=%f\n", 
                              attribArray[index].includes[cindex].leftOperand,
                              attribArray[index].includes[cindex].rightOperand);
                       */
                       #endif
                        if((atof(val) >= attribArray[index].includes[cindex].leftOperand) &&
                           (atof(val) <= attribArray[index].includes[cindex].rightOperand) )
                        {
                            return 1;
                        }
                   }
              }
         }
         else
         {
              ctotal = getTokenCount(attribArray[index].excludeValues);
              for(cindex = 0; cindex < ctotal; cindex++)
              {
                   /*printf("op=%s left=%f val=%f right=%f\n",
                                     attribArray[index].excludes[cindex].op,
                                     attribArray[index].excludes[cindex].leftOperand,
                                     atof(val),
                                     attribArray[index].excludes[cindex].rightOperand);
                   */
                   if(strcmp(attribArray[index].excludes[cindex].op, "<") == 0)
                   {
                        if(atof(val) < attribArray[index].excludes[cindex].rightOperand)
                        {
                             return 1;
                        }
                   }
                   else if(strcmp(attribArray[index].excludes[cindex].op, "<=") == 0)
                   {
                        if(atof(val) <= attribArray[index].excludes[cindex].rightOperand)
                        {
                             return 1;
                        }
                   }
                   else if(strcmp(attribArray[index].excludes[cindex].op, ">") == 0)
                   {
                        if(atof(val) > attribArray[index].excludes[cindex].leftOperand)
                        {
                            return 1;
                        }
                   }
                   else if(strcmp(attribArray[index].excludes[cindex].op, ">=") == 0)
                   {
                        if(atof(val) >= attribArray[index].excludes[cindex].leftOperand)
                        {
                            return 1;
                        }
                   }
                   else if(strcmp(attribArray[index].excludes[cindex].op, "-") == 0)
                   {
                        if((atof(val) >= attribArray[index].excludes[cindex].leftOperand) &&
                           (atof(val) <= attribArray[index].excludes[cindex].rightOperand))
                        {
                            return 1;
                        }
                   }
              }
         }
    }
    else
    {
         sprintf(mesg,"ATTRIBUTE_TYPE=%s not recongized. The only valid types are DISCRETE and CONTINUOUS", 
                attribArray[index].type);
         ERROR("filter file", mesg, 1);
    }
    return 0;  /* not included or not excluded */
}




/* returns 0 if no overlaps are found in the filter ranges*/
/* returns 1 if any of the include  values contain overlaps in continuous filter ranges */
/* returns 2 if any of the exclude  values contain overlaps in continuous filter ranges */
int checkForOverlaps()
{
    /*float begin1, end1, begin2, end2; */
    int attribIndex, filterIndex, lookAheadIndex, tc;

    for(attribIndex = 0; attribIndex <= totalAttribs; attribIndex++)
    {
          if(attribArray[attribIndex].includes != NULL)
          {
               tc = getTokenCount(attribArray[attribIndex].includeValues);

               for(filterIndex = 0; filterIndex < tc; filterIndex++)
               {
                    for(lookAheadIndex = filterIndex + 1; lookAheadIndex < tc; lookAheadIndex++)
                    {
 
                         if(OVERLAP0(attribArray[attribIndex].includes[filterIndex].leftOperand,
                                     attribArray[attribIndex].includes[filterIndex].rightOperand,
                                     attribArray[attribIndex].includes[lookAheadIndex].leftOperand,
                                     attribArray[attribIndex].includes[lookAheadIndex].rightOperand))
                         {
                              return 1;  /* an overlap was found */
                         }            
                    }                     

               }

          }

          if(attribArray[attribIndex].excludes != NULL)
          {
               tc = getTokenCount(attribArray[attribIndex].excludeValues);

               for(filterIndex = 0; filterIndex < tc; filterIndex++)
               {
                    for(lookAheadIndex = filterIndex + 1; lookAheadIndex < tc; lookAheadIndex++)
                    {
                         if(OVERLAP0(attribArray[attribIndex].excludes[filterIndex].leftOperand,
                                     attribArray[attribIndex].excludes[filterIndex].rightOperand,
                                     attribArray[attribIndex].excludes[lookAheadIndex].leftOperand,
                                     attribArray[attribIndex].excludes[lookAheadIndex].rightOperand))
                         {
                              return 2;  /* an overlap was found */
                         }
                    }

               }

          }
    }
    
    return 0; /* no overlaps */
}


/* Do some checking to make sure that an item doesn't appear on both the include and the
   exclude list, if so, generates a terminal error
   Also checks for mutually exclusive conditions that are errors
   this function is internal to this module (would be private member in a C++ class)
*/
int attribArrayCheckup()
{


}  



