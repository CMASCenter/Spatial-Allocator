/********************************************************************************
 * AttachDBFAttribute.c
 *
 * File contains:
 * AttachDBFAttribute
 *
 * Updated: Jan 2005 for weight function capabilities BB
 * Updated: April 2005 for "ALL" keyword support  BB
 * Updated: May 2005 to support file chunking   BB
 * Updated: June 2005, split shape_ifc.c into three separate files BB
 *
 * Comment from shape_ifc.c:
 * Note:  Many (most) of the functions in this module return "1" indicating
 *        success whereas elsewhere in the codeset a "0" indicates success.
 *        There is no consistency amongst return codes at all, so pay attention to
 *        how they are used by the calling function.
 *
 ********************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
                                                                                    
#include "shapefil.h"
#include "mims_spatl.h"
#include "mims_evs.h"           /* added 4/4/2005 BB */
#include "projects.h"
#include "parms3.h"
#include "eval.h"               /* added 12/17/2004 BB */
#include "io.h"                 /* added 4/4/2005 BB */

/*
 * =============================================================
 */
/*
 * connect attribute values to a polygon
 */
/*
 * Updated 12/17/2004 to accept weight function capability --BB
 */
int attachDBFAttribute(PolyObject * poly, char *attr_env_name, char *ctgr_name)
{
    DBFHandle hDBF;
    int i, nf;
    int nWidth, nDecimals;
    char szTitle[12];
    DBFFieldType eType;
    int found;
    int nr, recno;
    int ival, jc;
    int attr_id;
    double val;
    char *attr_name;
    int n, oldn;
    int ncnt, nmax;
    char **list, **listc;
    char mesg[256];
    char *str;
    extern int maxShapes;
    extern int numElements; /* number of things to sub into the expression */
    extern EXPRESSION_ELEMENT *postfixE; /* gets set in call to compileExpression */
#ifdef DEBUG
    const char *pszTypeName;
#endif
                                                                                    
    /*
     * Added vars on 12/17/2004 BB
     */
    char *attr_weight, expression[256];
    int valIndex,
        fieldCount, varCount, totalRecords, recordCount, numFields,
        *fieldIndices;
    int fieldSize, numberDecimals, count;
    char fieldName[50], envVar[100];
    double *fieldValues;
    DBFFieldType *fieldTypes;
    extern char *prog_name;
    extern int lastObjectRead;
    extern int startOfChunk;
    extern char *polyName;

                                                                                    
    /* no attributes, so just create an empty structure and return */
    /* this is used for Regular Grids in ALLOCATE mode */
    /* added 5/25/2005 BDB */
    if(attr_env_name == NULL)
{
        MESG("No attributes to attach, creating dummy header and NULL variables list");
                                                                                    
        if(!poly->attr_hdr)
        {
            poly->attr_hdr = getNewAttrHeader();
        }
        poly->attr_hdr->num_attr = 0;
                                                                                    
        poly->attr_val = NULL;
                                                                                    
        MESG("No attributes attached\n");
        return 1;
    }
                                                                                    
                                                                                    
    if(!poly)
    {
        sprintf(mesg, "%s", "PolyObject struct not set");
        ERROR(prog_name, mesg, 1);
    }
                                                                                    
    if(!poly->name)
    {
        sprintf(mesg, "%s", "PolyObject name not yet specified");
        ERROR(prog_name, mesg, 1);
    }
                                                                                    
                                                                                    
    if(!strlistc(attr_env_name,
                 "Comma separated list of attribute names",
                 &nmax, &ncnt, &list))
    {
                                                                                    
        sprintf(mesg, "%s:%s", "error reading list of attribute names",
                attr_env_name);
        ERROR(prog_name, mesg, 1);
    }
                                                                                    
    n = ncnt;
                                                                                    
    if(ncnt <= 0)
    {
        sprintf(mesg, "%s:%s",
                "Attribute list empty or negative count for list of attributes",
                attr_env_name);
        ERROR(prog_name, mesg, 2);
    }
    /* surrogate category list */
    if(ctgr_name)
    {
        if(!strlistc
           (ctgr_name, "Comma separated list of srgt category values",
            &nmax, &ncnt, &listc))
        {
            sprintf(mesg, "%s",
                    "Surrogate category list empty or negative count for list of attributes");
            ERROR(prog_name, mesg, 2);
        }
        if(n != ncnt)
        {
            sprintf(mesg, "%s",
                    "Inconsistent number of attributes and categories");
            ERROR(prog_name, mesg, 2);
        }
    }
                                                                                    
    hDBF = DBFOpen(poly->name, "rb");
    if(hDBF == NULL)
    {
        sprintf(mesg, "Unable to open DBF file %s for reading", poly->name);
        ERROR(prog_name, mesg, 2);
    }
                                                                                    
    /* added 4/13/2005 to support "ALL" keyword for attribute selection  BDB */
    if(!strcmp(list[0], "ALL"))
    {
        MESG("All attributes selected for outupt");
                                                                                    
                                                                                    
        /* make sure the file isn't empty */
        if((numFields = DBFGetFieldCount(hDBF)) == 0)
        {
            sprintf(mesg, "%s", "The DBF file contains no fields.");
            ERROR(prog_name, mesg, 2);
        }
                                                                                    
        list = malloc(numFields * sizeof(char *));
        ncnt = numFields;
                                                                                    
        for(fieldCount = 0; fieldCount < numFields; fieldCount++)
        {
            eType = DBFGetFieldInfo(hDBF, fieldCount,
                                    fieldName, &fieldSize, &numberDecimals);
                                                                                    
            list[fieldCount] = (char *) strdup(fieldName);
        }
                                                                                    
        /* stuff */
        /*for(fieldCount = 0; fieldCount < numFields; fieldCount++)
        {
            printf("list[%d]=%s\n", fieldCount, list[fieldCount]);
        }*/
                                                                                    
    }
                                                                                    
    /* total number of records in the input file */
    totalRecords = DBFGetRecordCount(hDBF);
                                                                                    
    if(!strcmp(polyName, ENVT_INPUT_FILE_NAME) || !strcmp(polyName, ENVT_WEIGHT_FILE_NAME))
    {
        if(maxShapes <= 0)
        {
              count = totalRecords;
        }
        else if(startOfChunk + maxShapes > totalRecords)
        {
             count = totalRecords - startOfChunk;
        }
        else
        {
             count = maxShapes;
        }
    }
    else
    {
        count = totalRecords;
    }
                                                                                    
    /*fprintf(stderr,"maxShapes=%d count=%d\n",maxShapes, count);*/
                                                                                    
    if(!strcmp(list[0], "USE_FUNCTION"))
    {
        MESG("Using function for weights\n");
                                                                                    
                                                                                    
        /* make sure the file isn't empty */
        if((numFields = DBFGetFieldCount(hDBF)) == 0)
        {
            sprintf(mesg, "%s", "The DBF file contains no fields.");
            ERROR(prog_name, mesg, 2);
        }
                                                                                    
        /* added 1/6/2005 BB */
        if(totalRecords == 0)
        {
            sprintf(mesg, "Input shapefile %s contains no records", poly->name);
            ERROR(prog_name, mesg, 2);
       }
                                                                                        
        if(!poly->attr_hdr)
        {
            poly->attr_hdr = getNewAttrHeader();
        }
        oldn = addNewAttrHeader(poly->attr_hdr, "Weight Function", FTDouble);
                                                                                        
        n = poly->attr_hdr->num_attr;
/*         printf("n=%d\n", n); */
                                                                                        
        if(!poly->attr_val)     /* if space has not yet been allocated */
        {
/*             printf("poly->attr_val unallocated\n"); */
            poly->attr_val =
                (AttributeValue **) malloc(totalRecords *
                                           sizeof(AttributeValue *));
                                                                                        
            if(!poly->attr_val)
            {
                sprintf(mesg, "%s",
                        "Unable to allocate memory for polygon attribute list");
                ERROR(prog_name, mesg, 2);
            }
                                                                                        
                                                                                        
            for(recordCount = 0; recordCount < count; recordCount++)
            {
                if((poly->attr_val[recordCount] =
                    (AttributeValue *) malloc(n *
                                              sizeof(AttributeValue))) == NULL)
                {
                    sprintf(mesg, "%s",
                            "Unable to allocate memory for polygon list items");
                    ERROR(prog_name, mesg, 2);
                }
                for(jc = 0; jc < n; jc++)
                {
                     poly->attr_val[recordCount][jc].str = NULL;
                }
            }
        }
        else
        {
                                                                                        
/*             printf("poly->attr_val already allocated\n"); */
            for(recordCount = 0; recordCount < count; recordCount++)
            {
                if((poly->attr_val[recordCount] =
                    (AttributeValue *) realloc(poly->attr_val[recordCount],
                                               n *
                                               sizeof(AttributeValue))) == NULL)
                {
                    sprintf(mesg, "%s",
                            "Unable to reallocate memory for polygon list items");
                    ERROR(prog_name, mesg, 2);
                }
                for(jc = oldn; jc < n; jc++)
                {
                     poly->attr_val[recordCount][jc].str = NULL;
                }
            }
        }
        getEnvtValue(ENVT_WEIGHT_FUNCTION, expression);
                                                                                        
#ifdef DEBUG
        sprintf(mesg, "Weight function detected=%s\n", expression);
        MESG(mesg);
#endif
                                                                                        
        MESG2("weight function: ", expression);
                                                                                        
        /* compile the expression */
        compileExpression(expression);
                                                                                        
                                                                                        
        /* create an array that will indicate which fields we are interested in */
        fieldIndices = (int *) malloc(numElements * sizeof(int));
                                                                                        
        if(!fieldIndices)
        {
            sprintf(mesg, "%s", "Unable to allocate memory for field index");
            ERROR(prog_name, mesg, 2);
        }
                                                                                        
        /* now create arrays to hold the field values and types needed */
        fieldValues = malloc(numElements * sizeof(double));
        fieldTypes =
            (DBFFieldType *) malloc(numElements * sizeof(DBFFieldType));
                                                                                        
        if(!fieldTypes)
        {
            sprintf(mesg, "%s",
                    "Unable to allocate memory for field values and types");
            ERROR(prog_name, mesg, 2);
        }
                                                                                        
        /* initialize fieldIndices array */
        for(varCount = 0; varCount < numElements; varCount++)
        {
            fieldIndices[varCount] = -1;
        }
                                                                                        
        /* set the values of that array according to the information in the db table */
        for(fieldCount = 0; fieldCount < numFields; fieldCount++)
        {
            eType = DBFGetFieldInfo(hDBF, fieldCount,
                                    fieldName, &fieldSize, &numberDecimals);
                                                                                        
            /*printf("fieldName: %s type=", fieldName);
                                                                                        
               if(eType == FTInteger)
               printf("FTInteger\n");
               else if(eType == FTDouble)
               printf("FTDouble\n");
               else if(eType == FTString)
               printf("FTString\n");
             */
                                                                                        
            for(varCount = 0; varCount < numElements; varCount++)
            {
                if(strcmp
                   (convertToUpper(postfixE[varCount].item),
                    convertToUpper(fieldName)) == 0)
                {
                    fieldIndices[varCount] = fieldCount;
                }
/*#ifdef DEBUG*/
                printf("fieldIndices[%d] = %d postfixE[%d].item=%s\n", varCount,
                       fieldIndices[varCount], varCount, postfixE[varCount].item);
                printf("fieldName: %s ", fieldName);
/* #endif*/
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
#ifdef DEBUG
        printf("number of Elements=%d\n", numElements);
        printf("number of records=%d\n", totalRecords);
#endif
                                                                                        
        /* loop over all the rows in the db file and perform the calculation */
/*         printf("\nlastObjectRead = %d  maxShapes = %d\n", lastObjectRead, maxShapes); */
        /*for(recordCount = lastObjectRead - maxShapes;
                      recordCount < lastObjectRead; recordCount++)
        */
        for(recordCount = 0;
                      recordCount < count; recordCount++)
        {
            valIndex = 0;
/*             printf ("working on record: %d\n", recordCount);  */
                                                                                        
            for(varCount = 0; varCount < numElements; varCount++)
            {
                if(fieldIndices[varCount] > -1)
                {
                    if(fieldTypes[varCount] == FTDouble)
                    {
                        fieldValues[valIndex++] = DBFReadDoubleAttribute(hDBF,
                                            recordCount, fieldIndices[varCount]);
#ifdef DEBUG
                        printf("fieldValues[%d]=%f\n", valIndex - 1,
                               fieldValues[valIndex - 1]);
#endif
                    }
                    else if(fieldTypes[varCount] == FTInteger)
                    {
                        /* convert to double */
                        fieldValues[valIndex++] =
                            (double) DBFReadIntegerAttribute(hDBF, recordCount,
                                        fieldIndices[varCount]);
                                                                                        
#ifdef DEBUG
                        printf("fieldValues[%d]=%f\n", valIndex - 1,
                               fieldValues[valIndex - 1]);
#endif
                                                                                        
                    }
                    else if(fieldTypes[varCount] == FTString)
                    {
                        /* this is an error condition, we can't compute on a string value */
                        ERROR(poly->name,
                              "Variable specified is not an Integer or Double",
                              1);
                    }
                    else
                    {
                        /* this should never occur, but who knows? */
                        ERROR(poly->name, "Unknow datatype in shapefile", 1);
                    }
               }
            }
                                                                                        
            /* n - 1 is the last attribute added and contains the weight function result */
            poly->attr_val[recordCount][n - 1].val =
                calculateExpression(fieldValues);
#ifdef DEBUG 
            fprintf(stderr, "result=%f\n", poly->attr_val[recordCount][n - 1].val);
#endif 

        }
                                                                                        
        free(fieldIndices);
        free(fieldValues);
        /* need to fill in the surrogate number */
                                                                                        
        if(!getEnvtValue(ctgr_name, envVar))
        {
            sprintf(mesg, "Unable to read %s", ctgr_name);
            ERROR(prog_name, mesg, 2);
        }
                                                                                        
        poly->attr_hdr->attr_desc[0]->category = atoi(envVar);
    }
    else                        /* not using weight function */
    {
                                                                                        
                                                                                        
        MESG("Not using function for weights\n");
                                                                                        
        for(attr_id = 0; attr_id < ncnt; attr_id++)
        {
            attr_name = list[attr_id];
                                                                                        
            ival = -1;
            if(ctgr_name)
            {
                ival = atoi(listc[attr_id]);
            }
                                                                                        
            if(!strcmp(attr_name, "NONE"))
            {
                MESG("Attribute specified as NONE: assuming AREA based surrogate\n");
                if(!poly->attr_hdr)
                {
                    poly->attr_hdr = getNewAttrHeader();
                }
                                                                                        
                i = addNewAttrHeader(poly->attr_hdr, attr_name, FTInvalid);
                                                                                        
                poly->attr_hdr->attr_desc[attr_id]->category = ival;
                                                                                        
                continue;
                /*goto next_attr; */
            }
            else
            {
                sprintf(mesg, "Using attribute %s", attr_name);
                MESG(mesg);
            }
                                                                                        
                                                                                        
            if((nf = DBFGetFieldCount(hDBF)) == 0)
            {
                sprintf(mesg, "%s", "There are no fields in the DBF file.");
                ERROR(prog_name, mesg, 2);
            }
                                                                                        
            found = 0;
            /*fprintf(stderr,"count=%d nf=%d\n", count, nf);*/
            for(i = 0; i < nf; i++)
            {
                eType = DBFGetFieldInfo(hDBF, i, szTitle, &nWidth, &nDecimals);
                if(!strcmp(szTitle, attr_name))
                {
                    found = 1;
                    break;
                }
                                                                                        
            }
                                                                                        
                                                                                        
            if(!found)
            {
                sprintf(mesg, "Attribute %s cannot be found in DBF file",
                        attr_name);
                ERROR(prog_name, mesg, 2);
            }
#ifdef DEBUG
            if(eType == FTString)
                pszTypeName = (char *) strdup("String");
            else if(eType == FTInteger)
                pszTypeName = (char *) strdup("Integer");
            else if(eType == FTDouble)
                pszTypeName = (char *) strdup("Double");
            printf("Field %d: Type=%s, Title=`%s', Width=%d, Decimals=%d\n",
                   i, pszTypeName, szTitle, nWidth, nDecimals);
#endif
                                                                                        
            nr = DBFGetRecordCount(hDBF);
                                                                                        
            if(nr != poly->nObjects && maxShapes == 0)
            {
                sprintf(mesg, "%s", "attribute/polygon count mismatch");
                ERROR(prog_name, mesg, 2);
            }
                                                                                        
            /* added 1/6/2005 BB */
            if(nr == 0)
            {
                ERROR(poly->name, "Input shapefile contains no records", 1);
            }
                                                                                        
                                                                                        
            if(!poly->attr_hdr)
            {
                poly->attr_hdr = getNewAttrHeader();
            }
                                                                                        
            oldn = addNewAttrHeader(poly->attr_hdr, attr_name, eType);
                                                                                        
            poly->attr_hdr->attr_desc[attr_id]->category = ival;
                                                                                        
            n = poly->attr_hdr->num_attr;
                                                                                        
            if(!poly->attr_val)
            {
                poly->attr_val =
                    (AttributeValue **) malloc(nr * sizeof(AttributeValue *));
                                                                                        
                if(!poly->attr_val)
                {
                    sprintf(mesg, "%s", "Attribute allocation error 1");
                    ERROR(prog_name, mesg, 2);
                }
                                                                                        
                for(recno = 0; recno < count; recno++)
                {
                    if((poly->attr_val[recno] =
                        (AttributeValue *) malloc(n *
                                           sizeof(AttributeValue)))
                                           == NULL)
                    {
                        sprintf(mesg, "%s", "Attribute allocation error 2");
                        ERROR(prog_name, mesg, 2);
                    }
                                                                                        
                    for(jc = 0; jc < n; jc++)
                    {
                         poly->attr_val[recno][jc].str = NULL;
                    }
                }
            }
            else
           {
                for(recno = 0; recno < count; recno++)
                {
                    if((poly->attr_val[recno] =
                        (AttributeValue *) realloc(poly->attr_val[recno],
                                                   n *
                                                   sizeof
                                                   (AttributeValue))) == NULL)
                    {
                        sprintf(mesg, "%s", "Attribute allocation error 3");
                        ERROR(prog_name, mesg, 2);
                    }
                                                                                        
                    for(jc = oldn; jc < n; jc++)
                    {
                         poly->attr_val[recno][jc].str = NULL;
                    }
                }
            }
                                                                                        
            for(recno = 0; recno < count; recno++)
            {
                switch (eType)
                {
                case FTInteger:
                    if(!strcmp(polyName, ENVT_INPUT_FILE_NAME))
                    {
                        ival = DBFReadIntegerAttribute(hDBF,
                                startOfChunk + recno, i);
                        poly->attr_val[recno][attr_id].ival = ival;
                    }
                    else
                    {
                        ival = DBFReadIntegerAttribute(hDBF, recno, i);
                        poly->attr_val[recno][attr_id].ival = ival;
                                                                                        
                    }
#ifdef DEBUG
                    fprintf(stderr, "recno = %d, attr = %d\n", recno, ival);
#endif
                    break;
                                                                                        
                case FTDouble:
                    if(!strcmp(polyName, ENVT_INPUT_FILE_NAME))
                    {
                        val = DBFReadDoubleAttribute(hDBF,
                                startOfChunk + recno, i);
                        poly->attr_val[recno][attr_id].val = val;
                    }
                    else
                    {
                        val = DBFReadDoubleAttribute(hDBF, recno, i);
                        poly->attr_val[recno][attr_id].val = val;
                    }
#ifdef DEBUG
                    fprintf(stderr,"attachDBFAttribute: recno = %d, attr = %f i=%d\n",
                              recno, val, i);
#endif
                    break;
                case FTString:
                    if(!strcmp(polyName, ENVT_INPUT_FILE_NAME))
                    {
                         poly->attr_val[recno][attr_id].str =
                              (char *) strdup(DBFReadStringAttribute(hDBF,
                                  startOfChunk + recno, i));
                    }
                    else
                    {
                         poly->attr_val[recno][attr_id].str =
                              (char *) strdup(DBFReadStringAttribute(hDBF, recno, i));
                                                                                        
                    }
                                                                                        
#ifdef DEBUG
                    fprintf(stderr, "recno = %d, attr = %s\n", recno,
                            poly->attr_val[recno][attr_id].str);
#endif
                    break;
                default:
                    sprintf(mesg, "%s",
                            "Only INTEGER, DOUBLE or STRING attributes are supported");
                    ERROR(prog_name, mesg, 2);
                }               /* end switch (eType) */
                                                                                        
            }                   /* end for recno=0; recno < nr */
                                                                                        
          /*next_attr:
            continue;*/
        }                       /* end of loop over attribs */
    }                           /* end of else */
    DBFClose(hDBF);
    free(list);
    return 1;
}

