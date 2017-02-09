/* allocate.c 
 * Written by Atanas Trayanov, MCNC Environmental Modeling Center in support of
 * the EPA Multimedia Integrated Modeling System, 2002.
 *
 * formerly a part of surrogates.c 4/5/2005 BB
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "shapefil.h"
#include "mims_spatl.h"
#include "mims_evs.h"
#include "parms3.h"
#include "io.h"
#include "parseAllocModes.h"

int writeOutputGridShapeFile(double *num_gridsum, PolyObject * g_poly,
                             char *outputGridFileName)
{
    int j, n, ncols;
    DBFHandle hDBF;
    FILE *numer_sum_file;
    char numer_sum_file_name[200];
    char mesg[256];
    extern char *prog_name;
    char outputType[100];   /*output type */
    char polyID[50];        /*output polygon ID name*/
    char out_poly_id[80];   /*output polygon ID data*/
    int outIDType;          /*output ID data type*/
    int gridID,col,row;


    MESG("writeOutputGridShapeFile\n");
    
    if(!getEnvtValue(ENVT_OUTPUT_FILE_TYPE, outputType))
    {
     sprintf(mesg,"%s","Can not get OUTPUT_FILE_TYPE environmental variable in allocate.c");
     ERROR(prog_name, mesg, 2);
    } 
    
    sprintf(numer_sum_file_name, "%s.csv", outputGridFileName);
    numer_sum_file = fopen(numer_sum_file_name, "w");
    if(numer_sum_file == NULL)
    {
        sprintf(mesg, "Cannot open file \"%s\" for writing",
                numer_sum_file_name);
        ERROR(prog_name, mesg, 1);
    }
    
    if(num_gridsum != NULL)
    {
        if(polyShapeWrite(g_poly, outputGridFileName) != 0)
        {
            ERROR(prog_name,
                  "Could not write output grid/polygon shape polygon file", 2);
        }
    }
    hDBF = DBFCreate(outputGridFileName);
      if(hDBF == NULL)
      {
          ERROR(prog_name,
                "Could not write output grid/polygon shape DBF file", 1);
      }        
    
    if(strcmp(outputType, "RegularGrid")==0 || strcmp(outputType, "EGrid")==0)
    {
      ncols = g_poly->map->ncols;
      if(DBFAddField(hDBF, "COL", FTInteger, 20, 0) == -1)
      {
          ERROR(prog_name,
                "Could not add attribute to output grid shape DBF file", 1);
      }
      if(DBFAddField(hDBF, "ROW", FTInteger, 20, 0) == -1)
      {
          ERROR(prog_name,
                "Could not add attribute to output grid shape DBF file", 1);
      }
      if(DBFAddField(hDBF, "NUMERSUM", FTDouble, 20, 0) == -1)
      {
          ERROR(prog_name,
                "Could not add attribute to output grid shape DBF file", 1);
      }
      fprintf(numer_sum_file, "COL,ROW,NUMERSUM\n");
    }
    else if (strcmp(outputType, "Polygon")==0)	    
    {      
      if(!getEnvtValue(ENVT_OUTPUT_POLY_ATTR, polyID))
      {
       sprintf(mesg,"%s","Can not get OUTPUT_POLY_ATTR environmental variable in allocate.c");
       ERROR(prog_name, mesg, 2);
      }     
      
      if (g_poly->attr_hdr) {
        if (g_poly->attr_hdr->attr_desc[0]->type == FTInteger) {
           outIDType = FTInteger;
	   if(DBFAddField(hDBF, polyID, FTInteger, 20, 0) == -1)
           {
              ERROR(prog_name,
                 "Could not add attribute to output polygon shape DBF file", 1);
            }
        }
        else  {
           outIDType = FTString;
	   if(DBFAddField(hDBF, polyID, FTString, 20, 0) == -1)
           {
              ERROR(prog_name,
                 "Could not add attribute to output polygon shape DBF file", 1);
            }	     
        }
      }
      else {
        ERROR(prog_name,
            "NO ATTRIBUTE HEADER was found for output polygons\n",1);
        outIDType = FTInvalid;
      }     
      
      if(DBFAddField(hDBF, "NUMERSUM", FTDouble, 20, 0) == -1)
      {
          ERROR(prog_name,
                "Could not add attribute to output polygon shape DBF file", 1);
      }
      fprintf(numer_sum_file, "%s,NUMERSUM\n",polyID);	    
    }
    
    n = g_poly->nObjects;
    for(j = 0; j < n; j++)
    {
      if(strcmp(outputType, "RegularGrid")==0 || strcmp(outputType, "EGrid")==0)
      {    
        if(strcmp(outputType, "RegularGrid")==0)
        {
             col = j%ncols + 1; 
             row = j/ncols + 1;
        }     
        else
        { 
             gridID = g_poly->attr_val[j][0].ival;
             col = (gridID-1)%ncols + 1;
             row = (gridID-1)/ncols + 1;
        }  
	    
        if(DBFWriteDoubleAttribute(hDBF, j, 0, col) != TRUE)
        {
            ERROR(prog_name,
                  "Could not write value to output grid shape DBF file", 1);
        }
        if(DBFWriteDoubleAttribute(hDBF, j, 1, row) != TRUE)
        {
            ERROR(prog_name,
                  "Could not write value to output grid shape DBF file", 1);
        }
        if(DBFWriteDoubleAttribute(hDBF, j, 2, num_gridsum[j]) != TRUE)
        {
            ERROR(prog_name,
                  "Could not write value to output grid shape DBF file", 1);
        }
        fprintf(numer_sum_file, "%d,%d,%.8lf\n", col, row, num_gridsum[j]);
      }
      else if (strcmp(outputType, "Polygon")==0)
      {	    
        out_poly_id[0] = '\0';
        if (outIDType == FTInteger) {
          sprintf(out_poly_id,"%d",g_poly->attr_val[j][0].ival);
	  if(DBFWriteIntegerAttribute(hDBF, j, 0, g_poly->attr_val[j][0].ival) != TRUE)
          {
            ERROR(prog_name,
                "Could not write value to output polygon shape DBF file", 1);
          }	  
        }
        else if (outIDType == FTString) {
           sprintf(out_poly_id,"%s",g_poly->attr_val[j][0].str);
           if(DBFWriteStringAttribute(hDBF, j, 0, g_poly->attr_val[j][0].str) != TRUE)
           {
            ERROR(prog_name,
                "Could not write value to output polygon shape DBF file", 1);
           }   
        }
        else {
           sprintf(out_poly_id,"poly%d",j);
	   if(DBFWriteStringAttribute(hDBF, j, 0, out_poly_id) != TRUE)
           {
             ERROR(prog_name,
                "Could not write value to output polygon shape DBF file", 1);
           }	
        }

	if(DBFWriteDoubleAttribute(hDBF, j, 1, num_gridsum[j]) != TRUE)
        {
            ERROR(prog_name,
                  "Could not write value to output polygon shape DBF file", 1);
        }
        fprintf(numer_sum_file, "%s,%.8lf\n", out_poly_id,num_gridsum[j]);	      
      }
				
    }
    DBFClose(hDBF);
    fclose(numer_sum_file);
    return 0;
}


/* ============================================================= */
/* Write results of AGGREGATE or AVERAGE operation. Both .shp and .dbf 
 */

/* put in allocate .c */
/* type is hard-coded to aggregate currently */
/* formerly called reportSum */
int allocate(PolyObject * poly, /* intersected weight and data polygons */ 
              char *ename,       /* e.g. "REPORT_NAME" - environment variable for name 
                                   of shape file */  
              int use_weight_val)
{
    double *sum;
    int *max = NULL;
    int *centroid = NULL;
    int n, overlapsCalculated = 0;
    int centroidsCalculated = 0;
    int i, k;
    PolyObject *d_poly, *w_poly;
    int attrtype;
    char data_poly_id[80];
    int attr_id, dPolyNumAttr;

    char name[100], modeFileName[256];
    char mesg[256], outputString[100];
    extern char *prog_name;
    DBFHandle hDBF;
    amode mode;


    /* specify the data and weight polygons */
    /* data is ouput poly, weight is input poly */
    d_poly = poly->parent_poly2;
    w_poly = poly->parent_poly1;

    /* get the list of attributes the user is concerned with */
    if(!getEnvtValue(ename, name))
    {
        sprintf(mesg, "Unable to continue because %s has not been set", ename);
        ERROR(prog_name, mesg, 2);
    }


    if(!getEnvtValue(ENVT_ALLOC_MODE_FILE, modeFileName))
    {
        sprintf(mesg, "Unable to continue because %s has not been set",
                modeFileName);
        ERROR(prog_name, mesg, 2);
    }

    parseAllocModes(modeFileName);


    /*name=(char *) strdup(name); */
    /* write the .shp portion of the shape file for the data polygon */
    /* added error test because allocate mode was crashing if the
       wrong type of shapefile is present 6/7/2005 BDB */
    if(d_poly->nSHPType == SHPT_POLYGON)
    {
         polyShapeWrite(d_poly, name);
    }
    else
    {
         sprintf(mesg, "Unable to continue, no polygons in %s", d_poly->name);
         ERROR(prog_name, mesg, 1);
    }

    /* write out the DBF portion with the appropriate fields */

    hDBF = DBFCreate(name);
    if(hDBF == NULL)
    {
        WARN2("DBFCreate failed for ", name);
        return 1;
    }
    
    /* loop over all attribs in target poly BDB  added loop 5/18/2005 */
    dPolyNumAttr = d_poly->attr_hdr->num_attr;

    /* these are the pass through fields for the data */
    for(attr_id = 0; attr_id < dPolyNumAttr; attr_id++)
    {
        /* check to see which attributes should be sent to the .dbf file */
        if(d_poly->attr_hdr)
        {
            if(d_poly->attr_hdr->attr_desc[attr_id]->type == FTInteger)
            {
                attrtype = FTInteger;
                if(DBFAddField
                   (hDBF, d_poly->attr_hdr->attr_desc[attr_id]->name, 
                     attrtype, 20, 0) == -1)
                {
                    sprintf(mesg, "1 Could not add %s to output DBF file %s", 
                             d_poly->attr_hdr->attr_desc[attr_id]->name, name);
                    ERROR(prog_name, mesg, 1);
                    return 1;
                }
            }
            else if(d_poly->attr_hdr->attr_desc[attr_id]->type == FTString)
            {
                attrtype = FTString;
                if(DBFAddField
                   (hDBF, d_poly->attr_hdr->attr_desc[attr_id]->name, 
                    attrtype, 20,
                    0) == -1)
                {
                     sprintf(mesg, "2 Could not add %s to output DBF file %s", 
                             d_poly->attr_hdr->attr_desc[attr_id]->name, name);
                    ERROR(prog_name, mesg, 1);
                    return 1;
                }
            }
            /* added this block for FTDouble 5/18/2005 BDB */
            else if(d_poly->attr_hdr->attr_desc[attr_id]->type == FTDouble)
            {
                attrtype = FTDouble;
                if(DBFAddField
                   (hDBF, d_poly->attr_hdr->attr_desc[attr_id]->name, 
                    attrtype, 20,
                    0) == -1)
                {
                     sprintf(mesg, "3 Could not add %s to output DBF file %s", 
                             d_poly->attr_hdr->attr_desc[attr_id]->name, name);
                    ERROR(prog_name, mesg, 1);
                    return 1;
                }
            }
            else
            {
                attrtype = FTInvalid;
                if(DBFAddField
                   (hDBF, d_poly->attr_hdr->attr_desc[attr_id]->name, 
                    attrtype, 20,
                    0) == -1)
                {
                     sprintf(mesg, "4 Could not add %s to output DBF file %s", 
                             d_poly->attr_hdr->attr_desc[attr_id]->name, name);
                    ERROR(prog_name, mesg, 1);
                    return 1;
                }
            }
        }
        else /* this code is suspiciously out of place here */
        {
            WARN("No attribute header");
             /*attrtype = FTInvalid;
            if(DBFAddField
               (hDBF, d_poly->attr_hdr->attr_desc[attr_id]->name, attrtype, 20,
                0) == -1)
            {
                WARN2("DBFAddField failed for ", name);
                return 1;
            }*/
        }

    }

    /* for the allocated attributes */
    for(attr_id = 0; attr_id < w_poly->attr_hdr->num_attr; attr_id++)
    {
        mode = getMode(w_poly->attr_hdr->attr_desc[attr_id]->name);

        if(mode == Aggregate || mode == Average)
        {
             //sprintf(mesg,"ALLOCTE: name=%s  mode=%d", w_poly->attr_hdr->attr_desc[attr_id]->name, mode);
             //MESG2("Variable: ", mesg);

             /* make sure original attribute type is not FTString */
             if(w_poly->attr_hdr->attr_desc[attr_id]->type == FTString)
             {
                  sprintf(mesg, 
                   "Unable to perform AGGREGATE or AVERAGE operation on a String attribute");
                  ERROR(prog_name, mesg, 1);
             } 

             //sprintf(mesg, "got Attribute: %s", w_poly->attr_hdr->attr_desc[attr_id]->name);
             //WARN(mesg);
           
             if(DBFAddField(hDBF, w_poly->attr_hdr->attr_desc[attr_id]->name,
                            FTDouble, 20, 5) == -1)
             {
                 sprintf(mesg, "5 Could not add %s to output DBF file %s", 
                             w_poly->attr_hdr->attr_desc[attr_id]->name, name);
                 ERROR(prog_name, mesg, 1);
                 return 1;
             }
        }
        else if(mode == DiscreteOverlap || mode == DiscreteCentroid)
        {
             //sprintf(mesg,"ALLOCTE: name=%s  mode=%d", w_poly->attr_hdr->attr_desc[attr_id]->name, mode);
             //MESG2("Variable: ", mesg); 
             if(w_poly->attr_hdr->attr_desc[attr_id]->type == FTString)
             {
                  if(DBFAddField(hDBF, w_poly->attr_hdr->attr_desc[attr_id]->name,
                                 FTString, 20, 0) == -1)
                  {
                      sprintf(mesg, "6 Could not add %s to output DBF file %s", 
                                  w_poly->attr_hdr->attr_desc[attr_id]->name, name);
                      ERROR(prog_name, mesg, 1);
                      return 1;
                  }
             }
             else if(w_poly->attr_hdr->attr_desc[attr_id]->type == FTInteger)
             {
             
                  if(DBFAddField(hDBF, w_poly->attr_hdr->attr_desc[attr_id]->name,
                                 FTInteger, 20, 0) == -1)
                  {
                      sprintf(mesg, "7 Could not add %s to output DBF file %s", 
                                  w_poly->attr_hdr->attr_desc[attr_id]->name, name);
                      ERROR(prog_name, mesg, 1);
                      return 1;
                  }
             } 
             else if(w_poly->attr_hdr->attr_desc[attr_id]->type == FTDouble)
             {
             
                  if(DBFAddField(hDBF, w_poly->attr_hdr->attr_desc[attr_id]->name,
                                 FTDouble, 20, 0) == -1)
                  {
                      sprintf(mesg, "7 Could not add %s to output DBF file %s", 
                                  w_poly->attr_hdr->attr_desc[attr_id]->name, name);
                      ERROR(prog_name, mesg, 1);
                      return 1;
                  }
             } 

        }
        /* 
           Add non-double field based on mode designated for attribute
           also involves field type--integer code that cannot be apportioned
           Strings = Strings, Integers = Integer or Double, Double = Double


           get mode, based on mode decide what field type to add
         */
    }

    n = d_poly->nObjects;

    /* pass through fields */
    for(i = 0; i < n; i++)
    {
        for(attr_id = 0; attr_id < dPolyNumAttr; attr_id++)
        {
            /*data_poly_id[0] = '\0'; --> pretty sure this is junk BDB */
            attrtype = d_poly->attr_hdr->attr_desc[attr_id]->type;
            if(attrtype == FTInteger)
            {
                if(DBFWriteIntegerAttribute(hDBF, i, attr_id,
                            d_poly->attr_val[i][attr_id].ival) != TRUE)
                {
                    sprintf(mesg, "1 Unable to write attribute %s to DBF file %s", 
                           d_poly->attr_hdr->attr_desc[attr_id]->name, name);
                    ERROR_NO_EXIT(prog_name, mesg);
                    return 1;
                }
            }
            else if(attrtype == FTString)
            {
                if(DBFWriteStringAttribute(hDBF, i, attr_id,
                            d_poly->attr_val[i][attr_id].str) != TRUE)
                {
                    sprintf(mesg, "2 Unable to write attribute %s to DBF file %s",
                             d_poly->attr_hdr->attr_desc[attr_id]->name, name);
                    ERROR_NO_EXIT(prog_name, mesg);
                    return 1;
                }
            }
            else if(attrtype == FTDouble)
            {
                if(DBFWriteDoubleAttribute(hDBF, i, attr_id,
                                d_poly->attr_val[i][attr_id].val) != TRUE)
                {
                   sprintf(mesg, "3 Unable to write attribute %s to DBF file %s",
                             d_poly->attr_hdr->attr_desc[attr_id]->name, name);
                    ERROR_NO_EXIT(prog_name, mesg);
                    return 1;
                }

            }
            else
            {
                 
               /* sprintf(data_poly_id, "poly%d", i);
                if(DBFWriteStringAttribute(hDBF, i, attr_id, data_poly_id) != TRUE)
                {
                    sprintf(mesg, "Unable to write attribute to DBF file");
                    ERROR_NO_EXIT(prog_name, mesg);
                    return 1;
                }*/

            }
        }
    }


    /* for floating point attributes, sum or average the values and 
     * write them to the .dbf */

    /* for each attribute */
    for(attr_id = 0; attr_id < w_poly->attr_hdr->num_attr; attr_id++)
    {
        /* eventually store type val for each attribute and make appropriate call */
        /* will have a type for each attribute from the attribute info file */

        mode = getMode(w_poly->attr_hdr->attr_desc[attr_id]->name);

        if(mode == NotFound)
        {
            sprintf(mesg, "Retrieving allocation mode for attribute %s: not in the ALLOCATE Attribute list.",
                    w_poly->attr_hdr->attr_desc[attr_id]->name);
            WARN(mesg);

        }
        else if(mode == Aggregate)
        {
            if(sum1Poly(poly, &sum, &n, attr_id, use_weight_val))
            {
                return 1;
            }
        }
        else if(mode == Average)
        {
            if(avg1Poly(poly, &sum, &n, attr_id, use_weight_val))
            {
                return 1;
            }
        }
        else if(mode == DiscreteOverlap && !overlapsCalculated)
        {
            /* only need to do this once 
               it won't change from attribute to attribute 
            */
            if(discreteOverlap(poly, &max, &n, attr_id)) 
            {
                return 1;
            }
            overlapsCalculated = 1;
            
            /* debug code only */ 
            /*printf("printing max array\n");
            for(k = 0; k < n; k++)
            {
               printf("max[%d]=%d\n", k, max[k]);
            }
            printf("finished printing max array\n");
            */
            
        }
        else if(mode == DiscreteCentroid && !centroidsCalculated)
        {
            /* only need to do this once, also */
            if(discreteCentroid(poly, &centroid, &n, attr_id)) 
            {
                return 1;
            }
            centroidsCalculated = 1;

        }
        /* else 
        {*/
            /* mode not found */ 
        /*    sprintf(mesg, "Allocation mode not supported");
            WARN(mesg);
        }*/


        /* for each object, write the resulting value for the attribute */
        if(mode == Aggregate || mode == Average)
        {
            for(i = 0; i < n; i++)
            {
                /* type is dependent upon from the attr_hdr from the output poly */
                if(DBFWriteDoubleAttribute(hDBF, i, attr_id + dPolyNumAttr, 
                                           sum[i]) != TRUE)
                {
                    WARN("ERROR in DBFWriteDoubleAttribute");
                    return 1;
                }
            } 
            free(sum);
        }
        else if(mode == DiscreteOverlap)
        {
            /*printf("in the discrete block mode=%d attr_id=%d\n", mode, attr_id);*/
            for(i = 0; i < n; i++)
            {
                /* type is dependent upon from the attr_hdr from the output poly */
                /* put if block around this for three different types */

                if(w_poly->attr_hdr->attr_desc[attr_id]->type == FTInteger)
                {
                    if (max[i] > -1)
                    {
                       if(DBFWriteIntegerAttribute(hDBF, i, attr_id + dPolyNumAttr, 
                                      w_poly->attr_val[max[i]][attr_id].ival) != TRUE)

                       {
                          WARN("ERROR in DBFWriteIntegerAttribute");
                          return 1;
                       }
                     }
                     else
                     {
                       if(DBFWriteIntegerAttribute(hDBF, i, attr_id + dPolyNumAttr, 
                                      MISSING_N) != TRUE)

                       {
                          WARN("ERROR in DBFWriteIntegerAttribute");
                          return 1;
                       }
                     }

                }
                else if(w_poly->attr_hdr->attr_desc[attr_id]->type == FTDouble)
                {
                     if (max[i] > -1)
                     {
                       if(DBFWriteDoubleAttribute(hDBF, i, attr_id + dPolyNumAttr, 
                                     w_poly->attr_val[max[i]][attr_id].val) != TRUE)

                       {
                          WARN("ERROR in DBFWriteDoubleAttribute");
                          return 1;
                       }
                     }
                     else
                     {
                       if(DBFWriteDoubleAttribute(hDBF, i, attr_id + dPolyNumAttr, 
                                     MISSING_N) != TRUE)

                       {
                          WARN("ERROR in DBFWriteDoubleAttribute");
                          return 1;
                       }
                     }

                }
                else if(w_poly->attr_hdr->attr_desc[attr_id]->type == FTString)
                {

                     if(max[i] > -1)
                     {
                          sprintf(outputString, "%s", 
                                  w_poly->attr_val[max[i]][attr_id].str);
                     }
                     else
                     {
                          sprintf(outputString, "%s",MISSING_S);

                     }
                 
                     if(DBFWriteStringAttribute(hDBF, i, attr_id + dPolyNumAttr, 
                                     outputString) != TRUE)

                     {
                          WARN("ERROR in DBFWriteStringAttribute");
                          return 1;
                     }
                }


            } 

        }
        else if (mode == DiscreteCentroid)
        {
            for(i = 0; i < n; i++)
            {
               /* printf("writing centroid[%d][%d] centroid[%d]=%d\n", i, attr_id,
                            i, centroid[i]);
                */
                if(w_poly->attr_hdr->attr_desc[attr_id]->type == FTString)
                {
                     if(centroid[i] > -1)
                     {
                          sprintf(outputString, "%s", 
                                   w_poly->attr_val[centroid[i]][attr_id].str);
                     }
                     else
                     {
                          sprintf(outputString, "");
                     }

                     if(DBFWriteStringAttribute(hDBF, i, attr_id + dPolyNumAttr, 
                                     outputString) != TRUE)
                     {
                              WARN("ERROR in DBFWriteStringAttribute");
                              return 1;
                     }


                }
                else if(w_poly->attr_hdr->attr_desc[attr_id]->type == FTDouble)
                {
                     if(centroid[i] > -1)
                     {
                          if(DBFWriteDoubleAttribute(hDBF, i, attr_id + dPolyNumAttr, 
                                     w_poly->attr_val[centroid[i]][attr_id].val) != TRUE)

                          {
                                   WARN("ERROR in DBFWriteDoubleAttribute");
                                   return 1;
                          }
                     }
                }
                else if(w_poly->attr_hdr->attr_desc[attr_id]->type == FTInteger)
                {
                     if(DBFWriteIntegerAttribute(hDBF, i, attr_id + dPolyNumAttr, 
                                     w_poly->attr_val[centroid[i]][attr_id].ival) != TRUE)
                     {
                              WARN("ERROR in DBFWriteIntegerAttribute");
                              return 1;
                     }
                }


     
            } /* end for(i */

        } /* end of else */

    }

    if(max != NULL)
    {
         free(max);
    }
    if(centroid != NULL)
    {
        free(centroid);
    }
    DBFClose(hDBF);

    return 0;
}


/* print the header used in the surrogate file that contains information
 * about the grid and map projection */
int print_hdr(FILE * file, PolyObject * g_poly)
{
    char mesg[256], outFileType[100];
    extern char *prog_name;

    char *cname[] = {
        "UNKNOWN",
        "LAT-LON",
        "LAMBERT",
        "MERCATOR",
        "STEREOGRAPHIC",
        "UTM",
        "POLGRD3",
        "EquatorialMERCATOR",
        "TransverseMERCATOR"
    };
    char unit[20]; 
    char gridStr[20];
    if(!getEnvtValue(ENVT_OUTPUT_FILE_TYPE, outFileType))
    {
        sprintf(mesg, "Unable to continue because %s has not been set",
                ENVT_OUTPUT_FILE_TYPE);
        ERROR(prog_name, mesg, 2);
    }
    if(strcmp(outFileType, "VariableGrid") == 0)
    {
        strcpy(gridStr, "#VARIABLE_GRID");
    }
    else if (strcmp(outFileType, "RegularGrid") == 0 || strcmp(outFileType, "EGrid") == 0)
    {
        strcpy(gridStr, "#GRID");
    }
    else if (strcmp(outFileType, "Polygon") == 0)
    {
	strcpy(gridStr, "#POLYGON");    
    }

    if (g_poly->map->ctype == 1) 
    {
      strcpy(unit,"degrees");
    }
    else
    {
      strcpy(unit,"meters");
    }

    if ( strcmp(cname[g_poly->map->ctype], "POLGRD3") == 0 )
    {
        return
        fprintf(file,
                "%s\t%s\t%f\t%f\t%f\t%f\t%d\t%d\t%d\t%s\t%s\t%f\t%f\t%f\t%f\t%f\n",
                gridStr,
                g_poly->map->gridname,
                g_poly->map->xorig,
                g_poly->map->yorig,
                g_poly->map->xcell,
                g_poly->map->ycell,
                g_poly->map->ncols,
                g_poly->map->nrows,
                1,
                cname[g_poly->map->ctype],
                unit,
                g_poly->map->p_gam,
                g_poly->map->p_alp,
                g_poly->map->p_bet,
                g_poly->map->xcent, g_poly->map->ycent);
    }
    else
    {
        return
        fprintf(file,
                "%s\t%s\t%f\t%f\t%f\t%f\t%d\t%d\t%d\t%s\t%s\t%f\t%f\t%f\t%f\t%f\n",
                gridStr,
                g_poly->map->gridname,
                g_poly->map->xorig,
                g_poly->map->yorig,
                g_poly->map->xcell,
                g_poly->map->ycell,
                g_poly->map->ncols,
                g_poly->map->nrows,
                1,
                cname[g_poly->map->ctype],
                unit,
                g_poly->map->p_alp,
                g_poly->map->p_bet,
                g_poly->map->p_gam, g_poly->map->xcent, g_poly->map->ycent);
    }
}
