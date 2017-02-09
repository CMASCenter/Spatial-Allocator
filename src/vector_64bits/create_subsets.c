/***************************************************************************
                          create_subsets.c  -  description
                             -------------------
    begin                : Thu Nov 11 2004
    copyright            : (C) 2004 by Benjamin Brunk
    email                : brunk@unc.edu
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *  This software is part of the enhancements to the MIMS Spatial Allocator
 *
 *  In this module, we open the shapefiles and compare input attributes                       
 *                                                                         *
 ***************************************************************************/


#include "parseWeightAttributes.h"
#include "shapefil.h"
#include "io.h"


#ifdef SHAPE_TEST_MAIN
int main(int argc, char *argv[])
{
    int success = 0;
    int index, i;
    int iCount, eCount;
    char *inputFileName, *outputFileName;
    
       
    int retVal;
    
    if (argc != 4)
    {
       ERROR("Improper usage",
           "Usage: filter.exe shape_file_in filter_file output_file\n",1);       
    }

    success = filterDBF(argv[1], argv[2], argv[3]);

 
   

    return EXIT_SUCCESS;
    
}
#endif

int filterDBF(char *szInputFileName, char *szFilterFileName, char *szOutputFileName)
{
   char fileName[256];
   DBFHandle hDBF;
   int i, nf, ac;
   int nWidth, nDecimals;
   char szTitle[20];
   DBFFieldType eType;
   int found;
   int nr, recno;
   int ival;
   double val;
   char mesg[256], strBuffer[256];
   char *line;
   
   FieldTitleList *fieldTitleList;
   int parseReturnValue, includeLine = 1, excludeLine = 0, totalMatches = 0;

   SHPHandle hSHP, newShape;
   int nObjects, nShapeType;
   double minBound[4], maxBound[4];   

    FILE *pOfp;
    DBFHandle outDBF;
#ifdef DEBUG
    sprintf(mesg,"FilterDBF: input = %s, filter = %s, output = %s\n",
       szInputFileName, szFilterFileName, szOutputFileName);
    MESG(mesg);
#endif    

    /* open the shape file that will be read from */
    sprintf(fileName,"%s.shp",szInputFileName);

    hSHP = SHPOpen(fileName, "rb");

    if( hSHP == NULL )
    {
         ERROR(fileName, "Unable to open input shapefile", 1);
    }


    SHPGetInfo( hSHP, &nObjects, &nShapeType, minBound, maxBound );

    #ifdef DEBUG
    printf("Shapefile %s opened.  Number of shape objects: %d Number of Shape Types: %d\n",
                 fileName, nObjects, nShapeType);
    #endif
    
        
    /* open up the dbf file to read from */    
    sprintf(fileName,"%s.dbf",szInputFileName);
    hDBF = DBFOpen( fileName, "rb" );
    if( hDBF == NULL )
    {
        sprintf( mesg,"Could not open DBF file %s.dbf", szInputFileName );
        ERROR("filterDBF", mesg, 1);
    }

    #ifdef DEBUG
    fprintf(stdout, "DBF file %s opened for read\n",szInputFileName);
    #endif

    /* If there is no data in this file let the user know. */
    if( (nf = DBFGetFieldCount(hDBF)) == 0 )
    {
        sprintf( mesg,"%s","There are no fields in the DBF file." );
        ERROR("filterDBF",mesg, 1);
    }

    #ifdef DEBUG
    fprintf(stdout, "num DBF fields = %d\n",nf);
    #endif

    /* open up a dbf file for writing */
    sprintf(fileName,"%s.dbf",szOutputFileName);
    
    outDBF = DBFCreate(fileName);
    if (outDBF == NULL)
    {
       ERROR("DBF creation failed", "Could not create output DBF", 1);
    }
   
    MESG2("DBF output filename: ", fileName);

    /* open an output file to write the new shapefile */
    sprintf(fileName,"%s.shp",szOutputFileName);
    newShape = SHPCreate(fileName, nShapeType);

    if( newShape == NULL )
    {
         sprintf(mesg,"Unable to open:%s.shp", szOutputFileName );
         ERROR("filterDBF", mesg, 1);
    }
   
    MESG2("Shapefile output filename: ", fileName);

    /* open a file for writing the csv data */
    sprintf(fileName,"%s.csv",szOutputFileName);

     if((pOfp = fopen(fileName, "w")) == NULL)
    {
        ERROR("Output file ", "Unable to open for writing", 1);
        return 0;  /* failure */
    }
    MESG2("CSV output filename: ", fileName);

    #ifdef DEBUG
    fprintf(stdout,"opened csv output file %s.csv\n",szOutputFileName);
    #endif

    /* create the AttributeInfo data structure in memory and
     fill it up using the filter (subsets) file */
    parseReturnValue = parseWeightSubsets(szFilterFileName);

    #ifdef DEBUG
    fprintf(stdout, "completed parse of %s\n", szFilterFileName);
    #endif

    if(parseReturnValue != 0)
    {
        DBFClose(hDBF);
        DBFClose(outDBF); 
        SHPClose(hSHP);
        SHPClose(newShape);
        fclose(pOfp);
        cleanUp();
        ERROR(szFilterFileName,
            "Something is wrong with one of the filter attributes", 1);
    }

    parseReturnValue = parseAndSaveFilters();

    #ifdef DEBUG
    fprintf(stdout, "completed parsing of rhs attributes\n");
    #endif
    

    fieldTitleList = malloc(nf * sizeof(FieldTitleList));

    
    /* Read the names of the fields into FieldTitleList */
    for (i=0; i < nf; i++)
    {
        eType = DBFGetFieldInfo( hDBF, i, szTitle, &nWidth, &nDecimals );

        fieldTitleList[i].pszFieldName = (char *) strdup(szTitle);
        fieldTitleList[i].eType = eType;


        #ifdef DEBUG
        /* fprintf(stdout,"title = %s\n",fieldTitleList[i].pszFieldName); */
        #endif

        /* add this field to the output DBF file */
        if (DBFAddField(outDBF, szTitle, eType, nWidth, nDecimals) == -1)
        {
           DBFClose(hDBF);
           DBFClose(outDBF); 
           SHPClose(hSHP);
           SHPClose(newShape);
           fclose(pOfp);
           cleanUp();
           sprintf(mesg,"error adding field %s (%d)\n", szTitle, i);
           ERROR("filterDBF",mesg,1);
        }


    }

    /* added 1/5/2005 BB */
    /* loop over all the attributes to make sure there is a match */
    for(ac = 0; ac <= totalAttribs; ac++)
    {
         found = FALSE;
         for(i = 0; i < nf; i++)
         {
              if(strcmp(attribArray[ac].name, fieldTitleList[i].pszFieldName) == 0)
              {
                   found = TRUE;
              }
         }
 
         if(!found)
         {
              DBFClose(hDBF);
              DBFClose(outDBF); 
              SHPClose(hSHP);
              SHPClose(newShape);
              fclose(pOfp);
              cleanUp();
              sprintf(mesg, "Filter attribute %s not found in shapefile", attribArray[ac].name);
              ERROR(szFilterFileName, mesg, 1);
         }

    }

    /* Looping over attributes and noting which are included in the file */
    /* and writing the header to the csv file */
    for(i = 0; i < nf; i++)
    {
         for(ac = 0; ac <= totalAttribs; ac++)
         {
               if(strcmp(attribArray[ac].name, fieldTitleList[i].pszFieldName) == 0)
               {
                     found = ac; 
                     /* we have a matching field name */
                     #ifdef DEBUG
                     /*fprintf(stdout,"found criterion for field %s\n")*/
                     #endif

                     fieldTitleList[i].pszFieldName;
                     fieldTitleList[i].flagged = TRUE;
                     fieldTitleList[i].attribArrayIndex = ac;
                     ac = totalAttribs + 1;
               }
               else
               {
                     #ifdef DEBUG
                     /*fprintf(stdout,"no criterion for field %s\n",) */
                     #endif

                     fieldTitleList[i].pszFieldName;
                     fieldTitleList[i].flagged = FALSE;
                     fieldTitleList[i].attribArrayIndex = -1;
               }

          }

          fprintf(pOfp, "\"%s\"", fieldTitleList[i].pszFieldName);
          if(i != nf - 1) fprintf(pOfp, ",");

    }
    fprintf(pOfp, "\n");
    nr = DBFGetRecordCount(hDBF);
    #ifdef DEBUG
    fprintf(stdout,"record count = %d\n",nr);
    #endif

    /* loop over all records to see which ones match the filter criteria */
    for(recno = 0; recno < nr; recno++)
    {
      line = NULL;           /* re-initialize line buffer */
      includeLine = TRUE;    /* re-initialize boolean filter tests */
      excludeLine = FALSE;


      /* check each field to see if the criteria for this field is satisfied */
      for (i=0; i < nf; i++)
      {

          switch(fieldTitleList[i].eType)
          {
              case FTInteger:
                ival = DBFReadIntegerAttribute( hDBF, recno, i );
                sprintf(strBuffer, "%d", ival);
                break;

              case FTDouble:
                val = DBFReadDoubleAttribute( hDBF, recno, i );
                sprintf(strBuffer, "%f", val);
                break;

              case FTString:

                  strcpy(strBuffer, DBFReadStringAttribute( hDBF, recno, i ));
                  break;

              default:

              	sprintf(mesg,"%s",
                    "Only INTEGER, DOUBLE or STRING attributes are supported");
  	 	break;

          }
          /*fprintf(stdout,"value for %d = %s ",i,strBuffer); */

          /* See if this field is flagged for an include/exclude check */
          if(fieldTitleList[i].flagged)
          {
               /*fprintf(stdout," field = %d, eType = %d, index = %d ",i, eType,
                 fieldTitleList[i].attribArrayIndex); */
               if(includeLine)
               {

                    includeLine = checkIncludeExclude(CHECK_INCLUDE_LIST,
                    fieldTitleList[i].attribArrayIndex,
                    strBuffer, fieldTitleList[i].eType);
               }
               /*fprintf(stderr, "include = %d,", includeLine);*/

               if(!excludeLine)
               {
                       excludeLine = checkIncludeExclude(CHECK_EXCLUDE_LIST,
                       fieldTitleList[i].attribArrayIndex,
                       strBuffer, fieldTitleList[i].eType);
               }
               /*fprintf(stderr, "exclude = %d ", excludeLine); */

           }
           /*fprintf(stderr,"\n"); */

           if(includeLine && !excludeLine)
           {
                if(line == NULL)
                {
                     line = malloc(strlen(strBuffer)+6);
                     strcpy(line, "\"");

                }
                else
                {
                     line = realloc(line, strlen(line)+strlen(strBuffer)+6);
                     strcat(line, "\"");

                }

                strcat(line, strBuffer);
                strcat(line, "\"");
                if(i != nf - 1) strcat(line, ",");



           }  /* end of if */
           else
           {
                /* go on to the next record, this one is filtered out */
                i = nf;
           }



      }  /* end for i (loop over fields) */

      if(includeLine && !excludeLine)
      {
         /* write out the matching line from the
            existing dbf file to the new dbf file */
         for(i = 0; i < nf; i++)
         {
                switch( fieldTitleList[i].eType )
                {
                   case FTInteger:
                      #ifdef DEBUG
                      fprintf(stderr,"writing int %d ", i);
                      #endif
                      if (DBFWriteIntegerAttribute( outDBF, totalMatches, i,
                              DBFReadIntegerAttribute( hDBF, recno, i ) ) != 1)
                      {
                         DBFClose(hDBF);
                         DBFClose(outDBF); 
                         SHPClose(hSHP);
                         SHPClose(newShape);
                         fclose(pOfp);
                         cleanUp();
                         ERROR("writing integer", "Error writing integer to DBF", 1);
                      }
                      break;

                   case FTDouble:
                      #ifdef DEBUG
                      fprintf(stderr,"writing dbl %d ", i);
                      #endif
                      if (DBFWriteDoubleAttribute( outDBF, totalMatches,i,
                              DBFReadDoubleAttribute( hDBF, recno, i ) ) != 1)
                      {
                         DBFClose(hDBF);
                         DBFClose(outDBF); 
                         SHPClose(hSHP);
                         SHPClose(newShape);
                         fclose(pOfp);
                         cleanUp();
                         ERROR("writing double", "Error writing double to DBF", 1);
                      }
                      break;

                   case FTString:
                      #ifdef DEBUG
                      fprintf(stderr,"writing str %d ", i);
                      #endif
                      if (DBFWriteStringAttribute( outDBF, totalMatches, i,
                              DBFReadStringAttribute( hDBF, recno, i ) ) != 1)
                      {
                         DBFClose(hDBF);
                         DBFClose(outDBF); 
                         SHPClose(hSHP);
                         SHPClose(newShape);
                         fclose(pOfp);
                         cleanUp();
                         ERROR("writing string", "Error writing string to DBF", 1);
                      }
                      break;

                    default:

                     sprintf(mesg,"%s",
                       "Only INTEGER, DOUBLE or STRING attributes are supported");
                 }

             }  /* end for i */
        


         /* now read the corresponding line from the existing shapefile and write
            it to the new shapefile */
          SHPWriteObject( newShape, -1, SHPReadObject( hSHP, recno) );

         totalMatches++;
         
         if(line != NULL)
         {
              fprintf(pOfp, "%s\n", line);
              #ifdef DEBUG
              printf("%s\n",line);
              #endif
         }
      }
      #ifdef DEBUG
       else
      {
         fprintf(stderr,"record %d excluded\n",recno);
      }
      #endif

    } /* end for loop over records */
    if (totalMatches == 0)
    {
       sprintf(mesg, 
         "No records in shapefile %s matched the filter in %s\n",
          szOutputFileName, szFilterFileName);
       WARN(mesg);
    }

    /* added check for pass through filter 1/7/2005  BB */
    if(totalMatches == nr)
    {
         WARN("Output shapefile is identical to input shapefile--no records were filtered\n");
    }

    DBFClose(hDBF);
    DBFClose(outDBF); 
    SHPClose(hSHP);
    SHPClose(newShape);
    fclose(pOfp);
    cleanUp();

    return 1;
}

