/*****************************************************************************
 *
 *  dbf2asc.c  
 *  Created:  July 8, 2005 by Ben Brunk, Carolina Environmental Program
 *  Program to convert .dbf files into comma-delimited ASCII files
 *
 *****************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "io.h"
#include "shapefil.h"

extern int debug_output;

int main(int argc, char *argv[])
{

    char *inDBFfileName, *outFileName;
    char *usage = "DBF to delimited ASCII converter usage:  \ndbf2asc <name of DBF file> <name of output file>\n";
    char *version = "dbf2asc Version 1.0 7/8/2005";
    char *prog_name, *dbString, *de;
    char mesg[256], fieldName[50];
    char delim = ',';
    int count, field, recordCount, numFields, ival;
    int fieldSize, numberDecimals, totalRecords;
    double val;


    DBFFieldType eType;

    FILE *ofp;

    DBFHandle hDBF;

    prog_name = argv[0];

    debug_output = 1; /* a global from io.c */

    MESG(version);

    if(argc <  2)
    {
         MESG(usage);
         exit(0);
    }

    inDBFfileName = argv[1];
    outFileName = argv[2];

    hDBF = DBFOpen(inDBFfileName, "rb");

    if( hDBF == NULL )
    {
        sprintf(mesg, "Unable to open %s for reading", inDBFfileName);
        ERROR(prog_name, mesg, 2);
    }

    if((ofp = fopen(outFileName, "w")) == NULL)
    {
        sprintf(mesg,"Unable to open %s for writing\n", outFileName);
        ERROR(prog_name, mesg, 2);
    }

    if( (numFields = DBFGetFieldCount(hDBF)) <= 0 )
    {
        sprintf(mesg, "%s contains no records", inDBFfileName);
        ERROR("weightDBF", mesg, 2);
    }

    for(field = 0; field < numFields; field++)
    {
         eType = DBFGetFieldInfo( hDBF, field, fieldName, &fieldSize, &numberDecimals);
         fprintf(ofp,"%s", fieldName); 

         if(field < totalRecords - 1);
         {
              fprintf(ofp,"%c", delim);
         }

    }

    fprintf(ofp, "\n");
    totalRecords = DBFGetRecordCount(hDBF);

    for(recordCount = 0; recordCount < totalRecords; recordCount++)
    {
         for(field = 0; field < numFields; field++)
         {
              eType = DBFGetFieldInfo( hDBF, field, fieldName, &fieldSize, &numberDecimals);

              if(eType == FTInteger)
              {
                   ival = DBFReadIntegerAttribute( hDBF, recordCount, field);
                   fprintf(ofp, "%d", ival);

              }
              else if(eType == FTDouble)
              {
                   val = DBFReadDoubleAttribute( hDBF, recordCount, field);
                   fprintf(ofp, "%f", val);

              }
              else if(eType == FTString)
              {
                   dbString = (char *) strdup(DBFReadStringAttribute(hDBF, recordCount, field));

                   if(strlen(dbString) > 0)
                   {
                        fprintf(ofp, "\"%s\"", dbString);
                   }

              }
              if(field < numFields - 1)
              {
                   fprintf(ofp,"%c", delim);
              }

         }

         if(recordCount < totalRecords - 1)
         {
              fprintf(ofp, "\n");
         }
    }
    DBFClose(hDBF);
}

