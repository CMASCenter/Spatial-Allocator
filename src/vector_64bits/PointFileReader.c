/****************************************************************************
 *  PointFileReader.c
 * 
 *  Created: 4/14/2005  BDB
 * 
 *  Purpose: Needed for reading PointFiles which are an alternative file 
 *           to the binary shapefile.  A pointfile contains a comma-
 *           delimited set of points, separated by rows, which can be
 *           used to define a polygon for use in spatial allocation
 *           including overlays
 *
 * Modified 5/3/2005 to add file "chunking"
 * Modified 5/24/2005 added ability to read csv files with quoted strings 
 ****************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#if defined(__MACH__)
#include <stdlib.h>
#else
#include <malloc.h>
#endif
#include "shapefil.h"
#include "mims_spatl.h"
#include "mims_evs.h"
#include "parms3.h"
#include "io.h"

fpos_t filePosition; /* global added to allow for file "chunking" */
int window;

PolyObject *PointFileReader(char *ename, MapProjInfo *file_mapproj, 
                       MapProjInfo *output_mapproj)
{

    FILE *ifp;
    
    PolyObject *poly;
    PolyShape *ps;
    AttributeHeader *aheader;
    AttributeValue *tmpAV;
    Shape *shp;
    extern char *prog_name;
    extern int fileCompleted;
    extern int maxShapes;
    char pointFileName[256], mesg[256], lineBuffer[1024];
    char trimBuffer[1024], *tBuf;
    char overlayAttrs[256], mimsMode[100];
    char xc[25], yc[25];
    char **data, *tmpString, *ptr, *dbCheck;
    char **header;
    char **attribList;
    int i, j,  count, numPoints;
    bool insideQuote, getAllAttribs;
    int xcol = -1;
    int ycol = -1;
    int ncnt, nmax, hc, matched;
    int nObjects, numFields, numRecords, nShapeType;

    char de[24], delimiter[10], d;
    char *running, tempString[256];
 
    double projx, projy;

    PointFileIndex *pfIndex;
    int  StringItems;  // = 1 set String items and = 0 set Double items

    /* read in name of column not number */

    if(!getEnvtValue(ENVT_INPUT_FILE_XCOL, xc))
    {
        sprintf(mesg, "%s must be set in order to use a PointFile", 
                         ENVT_INPUT_FILE_XCOL);
        ERROR(prog_name, mesg, 2); 
    }


    if(!getEnvtValue(ENVT_INPUT_FILE_YCOL, yc))
    {
        sprintf(mesg, "%s must be set in order to use a PointFile", 
                         ENVT_INPUT_FILE_YCOL);
        ERROR(prog_name, mesg, 2); 
    }

    /*printf("xcol name=%s ycol name =%s\n", xc, yc);*/


    if(!getEnvtValue(ENVT_INPUT_FILE_DELIM, de))
    {
        sprintf(mesg, "%s must be set in order to use a PointFile", 
                         ENVT_INPUT_FILE_DELIM);
        ERROR(prog_name, mesg, 2); 
    }

    if(!strcmp(de, "COMMA"))
    {
          strcpy(delimiter, ",");
          d = ',';
    }
    else if(!strcmp(de, "SEMICOLON"))
    {
          strcpy(delimiter, ";");
          d = ';';
    }
    else if(!strcmp(de, "PIPE"))
    {
          strcpy(delimiter, "|");
          d = '|';
    }
    /* add other delimiters here */
    else
    {
          /* no delimiter specified so use a space */
          strcpy(delimiter, " ");
          d = ' ';
    }
    /*printf("Delimiter is:%s or [%s]\n", de, delimiter);*/


    if(!getEnvtValue(ENVT_INPUT_FILE_NAME, pointFileName))
    {
        sprintf(mesg, "%s must be set in order to use a PointFile", 
                         ENVT_INPUT_FILE_NAME);
        ERROR(prog_name, mesg, 2); 
    }
 
    getEnvtValue(ENVT_MIMS_PROCESSING, mimsMode);
    
    if(!strcmp(mimsMode, "OVERLAY") )
    {
         if(!strlistc(ENVT_OVERLAY_ATTRS,
                  " Comma separated list of attribute names",
                  &nmax, &ncnt, &attribList))
         {
              sprintf(mesg, "error reading list of attribute names from %s",
                           ENVT_OVERLAY_ATTRS);
              ERROR(prog_name, mesg, 2);
         }
         StringItems = 1; // set string items for overlay
    }
    else
    {
         if(!strlistc(ENVT_ALLOCATE_ATTRS,
                  " Comma separated list of attribute names",
                  &nmax, &ncnt, &attribList))
         {
              sprintf(mesg, "error reading list of attribute names from %s",
                           ENVT_ALLOCATE_ATTRS);
              ERROR(prog_name, mesg, 2);
         }
         StringItems = 0;  // set double items for allocation
    }


    if((ifp = fopen(pointFileName, "r")) == NULL)
    {
        sprintf(mesg,"Unable to read %s\n", pointFileName);
        ERROR(prog_name, mesg, 2);
    }

    /* do a quick reconnoiter of the file */
    /* ignore comments in files */
    numFields = 0;

    do
    {
        fgets(lineBuffer, sizeof(lineBuffer), ifp);
    }
    while(lineBuffer[0] == '#');

       
    trim(lineBuffer, trimBuffer);
    ptr = trimBuffer;
    insideQuote = FALSE;
 
    while(ptr[0] != '\0')
    {
        if(ptr[0] == d && insideQuote == FALSE)
        {
            numFields++;
        }
        else if(ptr[0] == '"' && insideQuote)
        {
            insideQuote = FALSE;
        }
        else if(ptr[0] == '"' && !insideQuote)
        {
            insideQuote = TRUE;
        }
        ptr++;
    } 
    numFields++;
    /*printf("numFields=%d\n", numFields);*/
   
    if(numFields == 0)
    {
        sprintf(mesg, "Formatting error encountered in %s", 
                             pointFileName);
        ERROR(prog_name, mesg, 2);
    }


    /* this if block is here and not above only because
       numFields was unknown before this point */
    if(!strcmp(attribList[0], "ALL") )
    {
        getAllAttribs = TRUE;
        /* change ncnt to total # of fields in point file */
        ncnt = numFields;
    }
    else
    {
        getAllAttribs = FALSE;
    }


    pfIndex = malloc(ncnt * sizeof(PointFileIndex));
 
    /* malloc an array for header names */
    header = (char **) malloc(numFields * sizeof(char *));
    /*printf("malloced for header\n");*/
         
    poly = getNewPoly(0);

    if(!poly->attr_hdr)
    {
        poly->attr_hdr = getNewAttrHeader();
    }
    /*printf("got new attr header\n");*/


    tBuf = trimBuffer;

    insideQuote = 0;
    i = 0;
    count = 0;
   
    while(tBuf[0] != '\0')
    {
        if(tBuf[0] == d && insideQuote == FALSE)
        {
             tempString[i] = '\0';
             i = 0;
             /*printf("PointFileReader tempString=%s count=%d\n", 
                    tempString, count);*/

             header[count] = (char *) strdup(tempString);
             
             if(!strcmp(header[count], xc))
             {
                 xcol = count;
             }
             else if(!strcmp(header[count], yc))
             {
                 ycol = count;
             }

             if(getAllAttribs)
             {
                 if (StringItems)
                 {
                      addNewAttrHeader(poly->attr_hdr, header[count], FTString);
                 }
                 else
                 {
                       addNewAttrHeader(poly->attr_hdr, header[count], FTDouble);
                 }
             }
             count++;
        }
        else if(tBuf[0] == '"' && insideQuote == FALSE)
        {
             insideQuote = TRUE;

        }
        else if(tBuf[0] == '"' && insideQuote == TRUE)
        {
             insideQuote = FALSE;

        }
        else
        {
             tempString[i] = tBuf[0];
             i++;
        }

        tBuf++;
    } /* end of while loop */

    tempString[i] = '\0';
    /*printf("PointFileReader tempString=%s count=%d\n", 
               tempString, count); */

    header[count] = (char *) strdup(tempString);
    
    if(!strcmp(header[count], xc))
    {
        xcol = count;
    }
    else if(!strcmp(header[count], yc))
    {
        ycol = count;
    }

    if(getAllAttribs)
    {
         if (StringItems)
         {
           addNewAttrHeader(poly->attr_hdr, header[count], FTString);
         }
         else
         {
            addNewAttrHeader(poly->attr_hdr, header[count], FTDouble);
         }
    }

    if(xcol == -1)
    {
        /* error condition */
        sprintf(mesg, "Could not locate %s in point file", xc);
        ERROR(prog_name, mesg, 2);
    }

    if(ycol == -1)
    {
        /* error condition */
        sprintf(mesg, "Could not locate %s in point file", yc);
        ERROR(prog_name, mesg, 2);
    }
    /*sprintf(mesg, "Columns with x,y coords in PointFile are %d and %d",
                  xcol, ycol);
    MESG(mesg);
    */

    /* the ALL condition was not selected */
    if(!getAllAttribs)
    {

        for(hc = 0; hc < ncnt; hc++)
        {
            matched = 0;
            for(count = 0; count < numFields; count++)
            {
                if(!strcmp(header[count], attribList[hc]) )
                {
                     /* saves the name and where to find it in
                        the header list so that when we go to get
                        the data, we know which columns we need
                        to save--pfIndex is internal only, it has
                        nothing to do with the PolyObject or its 
                        header structure
                      */
                     pfIndex[hc].name = (char *) strdup(header[count]);
                     pfIndex[hc].index = count;     

                     if (StringItems)
                     {
                        addNewAttrHeader(poly->attr_hdr, header[count], FTString); 
                     }
                     else
                     {
                        addNewAttrHeader(poly->attr_hdr, header[count], FTDouble);
                     }
                     matched = 1;
                }

            }
            if(matched == 0)
            {
                sprintf(mesg, "%s does not contain attribute %s",
                        pointFileName, attribList[hc]);
                WARN(mesg);
            }

        }
    }

    /* testing only */
    /*for(count = 0; count < ncnt; count++)
    {
        printf("pfIndex[%d] name=%s index=%d\n", count, pfIndex[count].name,
                       pfIndex[count].index);

    }*/

    /* this is probably wrong */
    poly->map = file_mapproj;

    poly->nSHPType = SHPT_POINT;
    poly->name = (char *) strdup(pointFileName);

    poly->bb = newBBox(0, 0, 0, 0);

    storeProjection(file_mapproj, output_mapproj); 

     nObjects = 0;

     data = (char **) malloc(numFields * sizeof(char *));

     /* if using file chunking, move the file pointer to the correct line */
     if(window > 0)
     {
        fsetpos(ifp, &filePosition); 
     }
          
     /* read in each line of the PointFile and get the coordinates */
     while((fgets(lineBuffer, sizeof(lineBuffer), ifp) != NULL) &&
            (nObjects < maxShapes || maxShapes == 0) )   
     {
          /* ignore embedded comments, if any */
          if(lineBuffer[0] == '#')
          {
               continue;
          }

          nObjects++;

          /* go through each line and break out fields */
          trim(lineBuffer, trimBuffer);

          tBuf = trimBuffer;

          insideQuote = 0;
          i = 0;
          count = 0;

          while(tBuf[0] != '\0')
          {
               if(tBuf[0] == d && insideQuote == FALSE)
               {
                   /* new field */
                   tempString[i] = '\0';
                   i = 0;
                   /*printf("PointFileReader2 tempString=%s\n", 
                            tempString);*/

                   data[count] = (char *) strdup(tempString);

                   if(count == xcol)
                   {
                        strcpy(xc, tempString);
                   }
                   else if(count == ycol)
                   {
                        strcpy(yc, tempString);

                   }
                   count++;

               }
               else if(tBuf[0] == '"' && insideQuote == FALSE)
               {
                    insideQuote = TRUE;

               }
               else if(tBuf[0] == '"' && insideQuote == TRUE)
               {
                    insideQuote = FALSE;

               }
               else
               {
                    tempString[i] = tBuf[0];
                    i++;
               }

               tBuf++;
          } /* end of while tBuf */
      
          tempString[i] = '\0';
          /*printf("PointFileReader2 tempString=%s\n", tempString);*/

          data[count] = (char *) strdup(tempString);

          if(count == xcol)
          {
               strcpy(xc, tempString);
          }
          else if(count == ycol)
          {
               strcpy(yc, tempString);
          }
          count++;

          ps = getNewPolyShape(1);
          ps->num_contours = 0;
          shp = getNewShape(1);

          if(digiCheck(xc))
          {
              sprintf(mesg, 
                 "%s is non-numeric and cannot be used to define a coordinate",
                  xc);
              ERROR(prog_name, mesg, 2);

          }

          if(digiCheck(yc))
          {
              sprintf(mesg, 
                 "%s is non-numeric and cannot be used to define a coordinate",
                  yc);
              ERROR(prog_name, mesg, 2);
          }

          /* get x and y for this shape from file */
          projectPoint(atof(xc), atof(yc), &projx, &projy);
          /*printf("xval=%f yval=%f projx=%f projy=%f\n", atof(xc), atof(yc),
                 projx, projy);*/

          /* output proj */
          shp->vertex[0].x = projx;
          shp->vertex[0].y = projy;
         /* printf("x=%10s y=%10s projx=%12.0f projy=%12.0f\n", xc, yc, projx, projy);*/
          gpc_add_contour(ps, shp, NOT_A_HOLE);
          polyShapeIncl(&(poly->plist), ps, NULL);

         
     } /* end of while fgets loop */
     
     
     
     poly->nObjects = nObjects;

     fclose(ifp);


     poly->attr_val = (AttributeValue **) 
                      malloc(nObjects * sizeof(AttributeValue *));

     if(!poly->attr_val)
     {
          sprintf (mesg, "%s", 
             "Unable to allocate memory for point file attributes");
          ERROR (prog_name, mesg, 2);
     }

     for(count = 0; count < nObjects; count++)
     {
         poly->attr_val[count] =
                   (AttributeValue *) malloc (ncnt *
                                sizeof (AttributeValue));

         
         for(j = 0; j < ncnt; j++)
         {
              poly->attr_val[count][j].str = NULL;
         }
     }


   
     /* re-open file to read the attributes */
     if((ifp = fopen(pointFileName, "r")) == NULL)
     {
          sprintf(mesg,"Unable to read %s\n", pointFileName);
          ERROR(prog_name, mesg, 2);
     }

    do
    {
        fgets(lineBuffer, sizeof(lineBuffer), ifp);
    }
    while(lineBuffer[0] == '#');
 
    
    numRecords = 0;

    /* if using file chunking, move the file pointer to the correct line */
    if(window > 0)
    {
        fsetpos(ifp, &filePosition); 
    }
    
    while((fgets(lineBuffer, sizeof(lineBuffer), ifp) != NULL) &&
            (numRecords < maxShapes || maxShapes == 0) )   
    {
          
          /* ignore embedded comments, if any */
          if(lineBuffer[0] == '#')
          {
               continue;
          }
          
          trim(lineBuffer, trimBuffer);

          tBuf = trimBuffer;

          insideQuote = 0;
          i = 0;
          count = 0;

          while(tBuf[0] != '\0')
          {
               if(tBuf[0] == d && insideQuote == FALSE)
               {
                   /* new field */
                   tempString[i] = '\0';
                   i = 0;
                   /*printf("PointFileReader3 tempString=%s\n", tempString);*/

                   if(getAllAttribs)
                    {
                       if (StringItems)
                       {
                          poly->attr_val[numRecords][count].str = (char *) strdup(tempString);
                       }
                       else
                       {
                          // set double items for allocation
                          if(digiCheck(tempString))
                          {
                            sprintf(mesg, "%s is non-numeric and cannot be used for ALLOCATE mode", tempString);
                            ERROR(prog_name, mesg, 2);
                          }
                          poly->attr_val[numRecords][count].val = atof(tempString);
                       }
                    }
                    else
                    {
                        for(hc = 0; hc < ncnt; hc++)
                        {
                            if(pfIndex[hc].index == count)
                            {
                               if (StringItems)
                               {
                                  poly->attr_val[numRecords][hc].str = (char *) strdup(tempString);
                               }
                               else
                               {
                                 // set double items for allocation
                                 if(digiCheck(tempString))
                                 {
                                     sprintf(mesg, "%s is non-numeric and cannot be used for ALLOCATE mode", tempString);
                                     ERROR(prog_name, mesg, 2);
                                 }
                                 poly->attr_val[numRecords][hc].val = atof(tempString);
                               } // StringItems
                            } //count
                       } //hc
                    } //getAllAtribs
                    count++;
               }
               else if(tBuf[0] == '"' && insideQuote == FALSE)
               {
                    insideQuote = TRUE;
               }
               else if(tBuf[0] == '"' && insideQuote == TRUE)
               {
                    insideQuote = FALSE;
               }
               else
               {
                    tempString[i] = tBuf[0];
                    i++;
               }


               tBuf++;
          } /* end of while tBuf */
          tempString[i] = '\0';
          /*printf("PointFileReader3 tempString=%s\n", tempString);*/

          if(getAllAttribs)
           {
              if (StringItems)
              {
                  poly->attr_val[numRecords][count].str = (char *) strdup(tempString);
              }
              else
              {
                 // set double items for allocation
                 if(digiCheck(tempString))
                 {
                    sprintf(mesg, "%s is non-numeric and cannot be used for ALLOCATE mode", tempString);
                    ERROR(prog_name, mesg, 2);
                 }
                 poly->attr_val[numRecords][count].val = atof(tempString);
              }
           }
           else
           {
               for(hc = 0; hc < ncnt; hc++)
               {
                   if(pfIndex[hc].index == count)
                   {
                      if (StringItems)
                      {
                          poly->attr_val[numRecords][hc].str = (char *) strdup(tempString);
                      }
                      else
                      {
                          // set double items for allocation
                          if(digiCheck(tempString))
                          {
                             sprintf(mesg, "%s is non-numeric and cannot be used for ALLOCATE mode", tempString);
                             ERROR(prog_name, mesg, 2);
                          }
                          poly->attr_val[numRecords][hc].val = atof(tempString);
                      } // StringItems
                   } //hc
              } //getAllAttribs
           }
           count++;

          numRecords++;
     }

     if(!feof(ifp))
     {
         fgetpos(ifp, &filePosition);     
     }
     else
     {
         fileCompleted = 1; /* the whole file is done */
              
     }      

     window++;

     fclose(ifp);

     recomputeBoundingBox(poly);


     return poly;

}



#ifdef TEST_POINTR
char *prog_name = "Point File Tester";
int maxShapes;
int fileCompleted = 1;
int main(int argc, char *argv[])
{
     int count, x;
     PolyObject *p;
     MapProjInfo *inputMapProj, *outputMapProj;

     maxShapes = 200;
     inputMapProj = getFullMapProjection(ENVT_INPUT_FILE_ELLIPSOID, 
                      ENVT_INPUT_FILE_MAP_PRJN);
     /* use OVERLAY_ELLIPSOID */
     p = PointFileReader(getenv(ENVT_INPUT_FILE_NAME), 
                         inputMapProj, inputMapProj);
                          
     printf("returned from PointFileReader\n");
     printf("number of header attribs=%d\n", p->attr_hdr->num_attr);
     printBoundingBox(p->bb);

     /*printf("header attributes:\n");*/
     for(x = 0; x < p->nObjects; x++)
     {
     
          for(count = 0; count < p->attr_hdr->num_attr; count++)
          {
               /*printf("%s\n", p->attr_hdr->attr_desc[count]->name);*/
               printf("%s ", p->attr_val[x][count].str);
               
          }
          printf("\n");
     }
     return 0;
}
#endif
