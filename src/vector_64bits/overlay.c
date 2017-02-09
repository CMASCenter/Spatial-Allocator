/* overlay.c
 * 
 * Written by Ben Brunk Carolina Environmental Program in support of
 * the EPA Multimedia Integrated Modeling System, 2005.
 *
 * Prints the results of an overlay operation to stdout or to a file
 * with a specified delimiter
 */

#include <stdio.h>
#include <stdlib.h> 
#include <string.h>

#include "mims_spatl.h"
#include "mims_evs.h"
#include "shapefil.h"
#include "io.h"

/* Write results of OVERLAY operation to stdout or to a file */
                                                                                              
int reportOverlays(PolyObject *poly)
{
     int count, retVal, id, dataValType,outIDYes;
     int index, numAttribs;
     char overlayOutType[30];
     char overlayType[15];
     char overlayOutName[256];
     char overlayOutDelim[30];
     char overlayOutCellID[5];
     char headerYesNo[10];
     char mesg[256];
     char *attrVal, delimiter;
     bool writeHeader;
     PolyShape *ps;
     PolyShapeList *plist;
     PolyParent *pp;
     PolyObject *i_poly,*o_poly;

     double dataValDouble;
     int dataValInteger;
     char *dataValString;
     extern char *prog_name;
     extern int window;

     FILE *ofp;
     int outID,polyID,ncols,row,col; /*for output overlay ID (col and row)*/

     retVal = getEnvtValue(ENVT_WRITE_HEADER, headerYesNo);

     if(!retVal)
     {
          ERROR(prog_name, 
                   "Must specify WRITE_HEADER either Y or N", 2);
     }

     if(!strcmp(headerYesNo, "Y"))
     {
          writeHeader = TRUE;
     }
     else if(!strcmp(headerYesNo, "N"))
     {
          writeHeader = FALSE;
     }
     else
     {
          ERROR(prog_name, 
             "You must specify either Y or N for WRITE_HEADER", 2);
     }
     
     retVal = getEnvtValue(ENVT_OVERLAY_TYPE, overlayType);

     if(!retVal)
     {
          ERROR(prog_name,
                   "Must specify an output type in OVERLAY_TYPE", 2);
     } 

     retVal = getEnvtValue(ENVT_OVERLAY_OUT_TYPE, overlayOutType);

     if(!retVal)
     {
          ERROR(prog_name, 
                   "Must specify an output type in OVERLAY_OUT_TYPE", 2);
     }

     retVal = getEnvtValue(ENVT_OVERLAY_OUT_DELIM, overlayOutDelim);

     if(!retVal)
     {
          ERROR(prog_name, 
               "Must specify a delimiter in OVERLAY_OUT_DELIM", 2);
     }
     sprintf(mesg, "Overlay output delimiter: %s\n", overlayOutDelim);
     MESG(mesg);

     if(!strcmp(overlayOutDelim, "COMMA"))
     {
          delimiter = ',';
     }
     else if(!strcmp(overlayOutDelim, "SEMICOLON"))
     {
          delimiter = ';';
     }
     else if(!strcmp(overlayOutDelim, "PIPE"))
     {
          delimiter = '|';
     }
     /* add other delimiters here */
     else
     {
          /* no delimiter specified so use a space */
          delimiter = ' ';
     }

     /* overlay output type can be stdout or DelimitedFile */
     sprintf(mesg, "Overlay output type: %s\n", overlayOutType);
     MESG(mesg);

     if(!strcmp(overlayOutType, "Stdout") || 
                !strcmp(overlayOutType,"STDOUT") ||
                !strcmp(overlayOutType, "stdout") ) 
     {
         ofp = stdout;
      
     }
     else if(!strcmp(overlayOutType, "DelimitedFile"))
     {

         retVal = getEnvtValue(ENVT_OVERLAY_OUT_NAME, overlayOutName);

         if(!retVal || overlayOutName[0] == '\0')
         {
             ERROR(prog_name, "Must specify a file name in OVERLAY_OUT_NAME", 2);
         }

         if((ofp = fopen(overlayOutName, "w")) == NULL)
         {
               sprintf(mesg,"Unable to open %s for writing\n", 
                        overlayOutName);
               ERROR(prog_name, mesg, 2);
         }

         sprintf(mesg,"Writing overlay output to %s", overlayOutName);
         MESG(mesg);

     }
     else
     {
          sprintf(mesg, "Unrecognized value for %s", ENVT_OVERLAY_OUT_TYPE);
          ERROR(prog_name, mesg, 2);
     }

     plist = poly->plist;                                                                                         
     i_poly = poly->parent_poly1;
     outIDYes = getEnvtValue(ENVT_OVERLAY_OUT_CELLID, overlayOutCellID);
     if(outIDYes && !strcmp(overlayOutCellID,"YES"))
     {
       sprintf(mesg, "Output overlay file ID (column and row) for RegularGrid or EGrid overlay file\n");
       MESG(mesg);
       o_poly = poly->parent_poly2;
       ncols = o_poly->map->ncols;
       sprintf(mesg,"oObject=%d  iObjectio = %d  ncols=%d", o_poly->nObjects,i_poly->nObjects,ncols);
       MESG(mesg);
     }
  
     numAttribs = i_poly->attr_hdr->num_attr; 
     sprintf(mesg,"numAttribe = %d", numAttribs);
     MESG(mesg);

     if(writeHeader && window - 1 == 0)
     {
         for(index = 0; index < numAttribs; index++)
         {
              fprintf(ofp, "%s", 
                  i_poly->attr_hdr->attr_desc[index]->name);
              if(index != numAttribs - 1)
              {
                   fprintf(ofp,"%c", delimiter);
              }
         }
        
         if(outIDYes && !strcmp(overlayOutCellID,"YES"))
         {
            fprintf(ofp,"%c", delimiter);
            fprintf(ofp, "%s","COL");
            fprintf(ofp,"%c", delimiter);
            fprintf(ofp, "%s","ROW");
         }     

         fprintf(ofp, "\n");
     }

     sprintf(mesg,"nObjects = %d", poly->nObjects);
     MESG(mesg);

     for (count = 0; count < poly->nObjects; count++)
     {
         ps = plist->ps;  /* the intersected weight-data polygon */
         id = -1;
         pp = plist->pp;  /* the parent polygon */
         /* only care about the input parent */
                                                                                              
         /* this id corresponds to the input shape, and we need
          * the number of attribs
          */
         id = pp->p1->index;   /*input shape index*/
         
         if(outIDYes && !strcmp(overlayOutCellID,"YES"))
         {
           outID = pp->p2->index;  /*overlay shape index*/
         }

         for(index = 0; index < numAttribs; index++)
         {
              dataValType = i_poly->attr_hdr->attr_desc[index]->type;

               if (dataValType == FTInteger)
               {
                    /*MESG("overlay: data value is INTEGER");*/
                    dataValInteger = i_poly->attr_val[id][index].ival;

                    fprintf(ofp, "%d", dataValInteger);
                    if(index != numAttribs - 1)
                    {
                        fprintf(ofp, "%c", delimiter); 
                    }
               }
               else if (dataValType == FTDouble)
               {
                    /*MESG("overlay: Data value is DOUBLE");*/
                    dataValDouble = i_poly->attr_val[id][index].val;

                    fprintf(ofp, "%f", dataValDouble);
                     
                    if(index != numAttribs - 1)
                    {
                       fprintf(ofp, "%c", delimiter); 
                    }
               }
               else
               {
                    /*MESG("overlay: data value is STRING");*/
                    dataValString = i_poly->attr_val[id][index].str;
                    /*printf("dataValString=%s\n", dataValString);*/

                    fprintf(ofp, "%s", dataValString);
                    if(index != numAttribs - 1)
                    {
                        fprintf(ofp, "%c", delimiter); 
                    }
                    
               }
                                                                                              
                                                                                              
         } /* end of for(index) */


         if(outIDYes && !strcmp(overlayOutCellID,"YES"))

         {
            if (!strcmp(overlayType,"RegularGrid"))
            {
                col = o_poly->attr_val[outID][0].ival;
                row = o_poly->attr_val[outID][1].ival;
            }
            else if (!strcmp(overlayType,"EGrid"))
            {
                polyID = o_poly->attr_val[outID][0].ival;
                col = (polyID-1)%ncols + 1;
                row = (polyID-1)/ncols + 1;
            }
            else
            {
               sprintf(mesg, "OVERLAY_TYPE value has to be RegularGrid or EGrid when outputing overlay col and row.");
               ERROR(prog_name, mesg, 2);
            }
            fprintf(ofp,"%c", delimiter);
            fprintf(ofp, "%d",col);
            fprintf(ofp,"%c", delimiter);
            fprintf(ofp, "%d",row);
         } 

         fprintf(ofp, "\n");

         plist = plist->next;
     } /* end of for(count) */

    if(!strcmp(overlayOutType, "DelimitedFile"))
    {
         fclose(ofp);
    }

} /* end of function */
