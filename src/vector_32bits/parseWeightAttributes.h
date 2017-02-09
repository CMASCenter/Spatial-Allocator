/***************************************************************************
                          parseWeightAttributes.h  -  description
                             -------------------
    begin                : Wed Nov 10 2004
    copyright            : (C) 2004 by Benjamin Brunk
    email                : brunk@unc.edu
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *  Function and type declarations for the attribute parser
 *                                                                         *
 ***************************************************************************/
#ifndef PARSE_WEIGHT_ATTR_H
#define PARSE_WEIGHT_ATTR_H
 
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <sys/types.h>

#include "shapefil.h"


/*#define SIMPLEMAIN*/
/*#define DEBUG */

#define CHECK_INCLUDE_LIST 1
#define CHECK_EXCLUDE_LIST 0

#define TRUE 1
#define FALSE 0


#define NO_ERRORS "No errors detected while parsing weight attributes"
#define PROCESSING_CANNOT_CONTINUE "Processing cannot continue due to errors"
#define UNRECOGNIZED_KEYWORD "Unrecognized keyword"
#define MISSING_ATTRIB_NAME "Missing ATTRIBUTE_NAME value"
#define DUPLICATE_ATTRIB_NAME "Duplicate ATTRIBUTE_NAME detected"
#define MULTIPLE_ATTRIB_TYPE_ERROR "Multiple ATTRIBUTE_TYPE statements under one attribute"
#define MISSING_ATTRIB_TYPE "Missing attribute ATTRIBUTE_TYPE value"

#define MULTIPLE_INCLUDES "Multiple INCLUDE_VALUES statements under one attribute"
#define MULTIPLE_EXCLUDES "Multiple EXCLUDE_VALUES statements under one attribute"
#define EXCLUDE_VALS_NO_ARGS "EXCLUDE_VALUES attribute specified with no arguments"
#define INCLUDE_VALS_NO_ARGS "INCLUDE_VALUES attribute specified with no arguments"
#define MISSING_INCEX_LINES "must have a corresponding INCLUDE_VALUES line or EXCLUDE_VALUES line"
#define MISSING_ATTRIB_TYPE_LINE "must have a correspoding ATTRIBUTE_TYPE line"

#define  MINRANGEVAL -1E20;
#define  MAXRANGEVAL  1E20;


#ifndef MIMS_SPATL_INCLUDED
#define MIN(x,y) ((x)<(y) ? (x):(y))
#define MAX(x,y) ((x)>(y) ? (x):(y))


#define OVERLAP0(begin1, end1, begin2, end2) \
        (MIN(end1, end2) >= MAX(begin1, begin2))
#endif        


typedef struct attribListItem
{
     char op[3];
     double leftOperand;
     double rightOperand;
} AttribListItem;



typedef struct attributeInfo
{
         char *name;
         char *type;
         char *includeValues;
         char *excludeValues;
         AttribListItem *includes;
         AttribListItem *excludes;
} AttributeInfo; 


typedef struct ftl
{
         char *pszFieldName;
         DBFFieldType eType;
         int flagged;
         int attribArrayIndex;
} FieldTitleList; 


/* global variables*/
extern AttributeInfo *attribArray;
extern int totalAttribs;

/* the core of this module, the parser function */
int parseWeightSubsets(char *filename);

/* the rest are helper functions */
char *getAttribs(char *input);   

int errorCheck(AttributeInfo *attribArray, int numElements);

int checkDuplicates(char *newItem);

int checkMultipleIncludesExcludes(AttributeInfo *attributeArray, int currentAttrib, int newItemType);

void cleanUp();

int parseAndSaveFilters();

int filterDBF(char *szInputFileName, char *szFilterFileName, char *szOutputFileName);

void parseItem(char *item, AttribListItem *ali);


/* Error & Warning Functions 
   Note:  These are the same as the ones in the io.c file and were included
          here for debugging purposes only (when code is compiled separately from
          the rest of the spatial allocator)
*/
#ifdef DEBUG_PWA
int WARN(char *msg);

int WARN2(char *msg1, char *msg2);

int MESG(char *msg);

int MESG2(char *msg1, char *msg2);

void ERROR(char *pgm, char *msg, int errcode);
#endif

#endif
