/****************************************************************************
 *  parseAllocModes.h
 *
 *  Header file for the allocation mode file parse
 *  This module uses most of the functionality of parse_weight_attributes.c
 *
 *  Date:  5/13/2005  BDB
 *  Updated: June 2005, added DiscreteOverlap and DiscreteCentroid modes to
 *           enumeration
 ***************************************************************************/
 
 #ifndef PARSE_ALLOC_MODES
 #define PARSE_ALLOC_MODES
 
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <sys/types.h>
 
typedef enum {
  NotFound,
  Aggregate,
  Average,
  DiscreteOverlap,
  DiscreteCentroid,
  AreaPercent
} amode;

typedef struct _AllocateMode
{
         char *name;
         amode mode;  /* supported modes are aggregate and average */
} AllocateMode;  
 

int parseAllocModes(char *fileName); 
int getValues(char *input, char *name, char *mode);
int checkMultipleModes(char *newItem, int  currentCount);
void cleanUpAllocModes();
amode getMode(char *name);
 


#endif 
