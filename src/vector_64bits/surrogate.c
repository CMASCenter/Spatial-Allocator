/* This program produces spatial surrogates for emission processing (e.g. with 
 * SMOKE) and performs spatial aggregation for which the attributes are either 
 * summed e.g. population) or averaged (e.g. population density).
 * 
 * Written by Atanas Trayanov, MCNC Environmental Modeling Center in support of
 * the EPA Multimedia Integrated Modeling System, 2002.
 *
 * Split into surrogate.c, allocate.c & overlay.c 4/5/2005 BB
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

/* ============================================================= */
/* main routine for computing emission surrogates.  Weight polygons 
 * are used for surrogate weights, data polygons are used for  
 * surrogate output resolution along with grid polygons.  The surrogate 
 * calculation is based on a fraction.  The numerator of the fraction is 
 * a function of the weight, data, and grid cell polygons (which results 
 * in the value of the attribute apportioned to a specific grid cell), and  
 * the denominator is the sum of the attribute for the data polygons.  */
int reportSurrogate(PolyObject *wdg_poly, char *ename, int use_weight_val,
   char *outputGridFileName)
{
  double **num;
  double *denom;
  double frac, a, b, last_b, sum_a=0.0;
  int n1, n2, d1;
  int i, j;
  /* weight, data, and grid polygons, plus intersected weight & data polys */  
  PolyObject *wd_poly, *d_poly, *g_poly, *w_poly;
  int attrtype;
  int outIDType;
  int ncols,polyID;
  char data_poly_id[80];
  char out_poly_id[80];
  char msg[100];
  int attr_id;   /* loop index */
  int attr_num;  /* written to output file */

  FILE *sfile;
  char tmpEnvVar[10];

  int output_hdr;
  int output_qasum;
  int output_numerator;
  int output_denominator;
  int valuesOK = 1;
  int lastrow = 0;
  int lastcol = 0;
  double qasum = 0.0;
  double *num_gridsum = NULL;

  extern char *prog_name;
  char qasum_str[20];  
  char numerator_str[20];  
  char denominator_str[20];  
  char fname[100];
  char last_data_poly_id[80];
  PolyIntStruct *polyIntInfo;
  char mesg[256];
  char outputType[100];
  char denomThreshold_s[100];
  double  denomThreshold = 0.00001;  /*default denominator threshold for surrogate ratio computation*/

  /* retrieve name of and open output file for surrogates */  
  if (ename == NULL) {
    sprintf(mesg,"%s","No logical name specified for output");
    goto error;
  }

  if(!getEnvtValue(ename, fname))
  {
    sprintf(mesg,"Unable to continue because %s has not been set", ename);
    ERROR(prog_name, mesg, 2);
  }

  if(!getEnvtValue(ENVT_OUTPUT_FILE_TYPE, outputType))
  {
     sprintf(mesg,"%s","Can not get OUTPUT_FILE_TYPE environmental variable in surrogate.c");
     goto error;
  } 
  
  sfile = fopen(fname,"w");
  if (sfile == NULL) 
  {
    sprintf(mesg,"Cannot open file \"%s\" for writing", fname);
    goto error;
  }

  /*set denominator threshold for surrogate ratio computation*/
  if(!getEnvtValue(ENVT_DENOMINATOR_THRESHOLD,denomThreshold_s))
  {
     sprintf(mesg,"DENOMINATOR_THRESHOLD for surrogate ratio computation is set to the default: \"%lf\"\n",denomThreshold);
     MESG(mesg);
  }
  else 
  {
     denomThreshold = atof(denomThreshold_s);
     sprintf(mesg,"DENOMINATOR_THRESHOLD for surrogate ratio computation is set to: \"%lf\"\n",denomThreshold);
     MESG(mesg);
  }

  wd_poly = wdg_poly->parent_poly1;
  d_poly = wd_poly->parent_poly2;
  w_poly = wd_poly->parent_poly1;
  g_poly = wdg_poly->parent_poly2;

  output_hdr = 1;

  getEnvtValue(ENVT_SRG_HEADER, tmpEnvVar);
  /* no reason to check for an error condition since if this 
     env. var. is not set, we can assume a default processing
     mode (write the header) */

  if(tmpEnvVar[0] != '\0') 
  {
    if (!strcmp(tmpEnvVar, "NO")) 
    {
      output_hdr = 0;
    }
  }

  output_qasum = 0;
  getEnvtValue(ENVT_SRG_QASUM, tmpEnvVar);
 
  if(tmpEnvVar[0] != '\0') 
  {
    if(!strcmp(tmpEnvVar, "YES")) 
    {
       output_qasum = 1;
    }
  }

  output_numerator = 0;

  getEnvtValue(ENVT_SRG_OUTPUTNUM, tmpEnvVar);

  if (tmpEnvVar[0] != '\0') 
  {
    if (!strcmp(tmpEnvVar,"YES")) 
    {
      output_numerator = 1;
    }
  }
  output_denominator = 0;

  getEnvtValue(ENVT_SRG_OUTPUTDEN, tmpEnvVar);

  if(tmpEnvVar[0] != '\0') 
  {
    if(!strcmp(tmpEnvVar, "YES")) 
    {
      output_denominator = 1;
    }
  }

  
  /* initialize strings if they won't be updated */  
  if (!output_qasum) 
  {
     strcpy(qasum_str,"");
  }
  if (!output_numerator) 
  {
     strcpy(numerator_str,"");
  }
  if (!output_denominator) 
  {
     strcpy(denominator_str,"");
  }
  if (strcmp(outputGridFileName,"") != 0)
  {
     num_gridsum = (double *)malloc(g_poly->nObjects*sizeof(double));
     if (num_gridsum == NULL)
     {
        ERROR("reportSurrogate",
           "Cannot allocate space for numerator grid cell sum",2);
     }
  }
  
  for (j = 0; j < g_poly->nObjects; j++)
  { 
     num_gridsum[j] = 0.0;
  }

  if (output_hdr) {
  /* Header line */
    print_hdr(sfile, g_poly);
  }

  /* only one attribute for data poygons */
  if (d_poly->attr_hdr) {
    if (d_poly->attr_hdr->attr_desc[0]->type == FTInteger) {
      MESG("data poly attribute type is INT\n");
      attrtype = FTInteger;
    }
    else if (d_poly->attr_hdr->attr_desc[0]->type == FTString) {
      MESG("data poly attribute type is STRING\n");
      attrtype = FTString;
    }
    else {
      MESG("data poly attribute type is INVALID\n");
      attrtype = FTInvalid;
    }
  }
  else {
    ERROR("reportSurrogate",
        "NO ATTRIBUTE HEADER was found for data polygons\n",2);
    attrtype = FTInvalid;
  }
  
  /*if output type is Polygon, chech attribute type for the polygon ID, only attached one attribute*/
  if(strcmp(outputType, "Polygon")==0 || strcmp(outputType, "EGrid")==0)
  {
    if (g_poly->attr_hdr) {
      if (g_poly->attr_hdr->attr_desc[0]->type == FTInteger) {
        MESG("Output poly attribute type is INT\n");
        outIDType = FTInteger;
      }
      else if (g_poly->attr_hdr->attr_desc[0]->type == FTString) {
        MESG("Output poly attribute type is STRING\n");
        outIDType = FTString;
      }
      else {
        MESG("Output poly attribute type is INVALID\n");
        outIDType = FTInvalid;
      }
    }
    else {
      ERROR("reportSurrogate",
          "NO ATTRIBUTE HEADER was found for output polygons\n",2);
      outIDType = FTInvalid;
    }
  }
 
  /* loop over all requested attributes for weight polygons */    
  for (attr_id=0; attr_id < w_poly->attr_hdr->num_attr ;attr_id++) 
  {    
    /* compute the numerator for the surrogate fraction */    
    MESG("Compute numerator\n");
#ifdef OLD_SUM  
    if (sum2Poly(wdg_poly, &num, &n1, &n2, attr_id) !=0 ) return 1;
#else    
    if (sum2Poly(wdg_poly, &num, &n1, &n2, attr_id, &polyIntInfo, use_weight_val) !=0 ) return 1;
#endif
    /* compute the denominator for the surrogate fraction */    
    MESG("Compute denominator\n");
    if (sum1Poly(wd_poly, &denom, &d1, attr_id, use_weight_val) !=0 ) return 2;
    
    attr_num = w_poly->attr_hdr->attr_desc[attr_id]->category;
    
    if (n1 != d1) return 3;
    /* for variable grid, nrows should be for vgrid, not reg grid */
/*     nrows = g_poly->map->nrows; */
 
    if(strcmp(outputType, "RegularGrid")==0 || strcmp(outputType, "EGrid")==0)
    {
      ncols = g_poly->map->ncols;
    }
         
    last_data_poly_id[0] = '\0';

    for (i=0; i<n1; i++) 
    {
      if ((b=denom[i]) != 0.0) {
        data_poly_id[0] = '\0';
        if (attrtype == FTInteger) {
          sprintf(data_poly_id,"%d",d_poly->attr_val[i][0].ival);
        }
        /*else if (d_poly->attr_hdr->attr_desc[0]->type == FTString) {*/
        else if (attrtype == FTString) {
          sprintf(data_poly_id,"%s",d_poly->attr_val[i][0].str);
        }
        else {
          sprintf(data_poly_id,"poly%d",i);
        }
        
        /* check to see if the current data poly id (for denominator is 
         * the same as the previous one.  If not, reset the qa sum */
        if (strcmp(data_poly_id, last_data_poly_id)) 
        {
          frac = 1.0 - qasum;   /*missing percentage in a data polygon*/
          //fprintf(sfile,"QA sum for attr %d, county %s was %.4lf not 1; last c,r=%d,%d,%d\n",
          //       attr_num, last_data_poly_id, qasum, lastcol, lastrow, n1);


          if ( fabs(frac) > 0.00001 && qasum > 0.00001 )
          {   
            if (strlen(last_data_poly_id) >= 1)
            {            
              if (output_qasum || output_numerator || output_denominator)
              {         
                /*change****************************************************/
                sprintf(mesg,"QA sum for attr %d, county %s was %.4lf not 1; last c,r=%d,%d",
                   attr_num, last_data_poly_id, qasum, lastcol, lastrow);             
                WARN(mesg);
      
                sprintf(qasum_str,"\t%lf",1-qasum);
                /* AME: changed str to 1-qasum to show remainder */
                qasum = 1.0;
                sprintf(numerator_str,"\t%lf",last_b-sum_a);
                sprintf(denominator_str,"\t%lf",last_b);

                if(strcmp(outputType, "RegularGrid")==0 || strcmp(outputType, "EGrid")==0)
                {
                   lastcol = 0;
                   lastrow = 0;         
                   fprintf(sfile,"#REMAINDER %5d\t%s\t%5d\t%5d\t%10.8lf\t!%s%s%s\n",
                       attr_num, last_data_poly_id, lastcol, lastrow,frac,
                       numerator_str, denominator_str, qasum_str);
                   sprintf(mesg,"#REMAINDER %5d\t%s\t%5d\t%5d\t%10.8lf\t!%s%s%s\n",
                       attr_num, last_data_poly_id, lastcol, lastrow,frac,
                       numerator_str, denominator_str, qasum_str);
                   MESG(mesg);
                }
                else if (strcmp(outputType, "Polygon")==0)
                {
                   sprintf(out_poly_id,"0");    
                   fprintf(sfile,"#REMAINDER %5d\t%s\t%s\t%10.8lf\t!%s%s%s\n",
                      attr_num, last_data_poly_id, out_poly_id, frac,
                      numerator_str, denominator_str, qasum_str);
                   sprintf(mesg,"#REMAINDER %5d\t%s\t%s\t%10.8lf\t!%s%s%s\n",
                      attr_num, last_data_poly_id, out_poly_id, frac,
                      numerator_str, denominator_str, qasum_str);
                   MESG(mesg);
                }
              }
              else
              {
                 if(strcmp(outputType, "RegularGrid")==0 || strcmp(outputType, "EGrid")==0)
                 {
                   lastcol = 0;
                   lastrow = 0;         
                   fprintf(sfile,"#REMAINDER %5d\t%s\t%5d\t%5d\t%10.8lf\n",
                           attr_num, last_data_poly_id, lastcol, lastrow,frac);
                   sprintf(mesg,"#REMAINDER %5d\t%s\t%5d\t%5d\t%10.8lf\n",
                           attr_num, last_data_poly_id, lastcol, lastrow,frac);
                   MESG(mesg);
                 }
                 else if (strcmp(outputType, "Polygon")==0)
                 {
                   sprintf(out_poly_id,"0");    
                   fprintf(sfile,"#REMAINDER %5d\t%s\t%s\t%10.8lf\n",
                      attr_num, last_data_poly_id, out_poly_id, frac);
                   sprintf(mesg,"#REMAINDER %5d\t%s\t%s\t%10.8lf\n",
                      attr_num, last_data_poly_id, out_poly_id, frac);
                   MESG(mesg);
                 }                  
              }         
            }
          }
          qasum=0.0;
          strcpy(last_data_poly_id,data_poly_id);
          sum_a = 0.0;
          last_b = b;
        }
       
        
        /* loop over grid cells */
        for (j=0; j<n2; j++) 
        {
       
           /*get polygon id*/
           if (strcmp(outputType, "Polygon")==0)
           {
              out_poly_id[0] = '\0';
              if (outIDType == FTInteger) {
              sprintf(out_poly_id,"%d",g_poly->attr_val[j][0].ival);
              }
              /*else if (g_poly->attr_hdr->attr_desc[0]->type == FTString) {*/
              else if (outIDType == FTString) {
                sprintf(out_poly_id,"%s",g_poly->attr_val[j][0].str);
              }
              else {
                sprintf(out_poly_id,"poly%d",j);
              }
           }
          
           /*get egrid id*/
           if (strcmp(outputType, "EGrid")==0)
           {
             polyID = g_poly->attr_val[j][0].ival;
           }
        
#ifdef OLD_SUM
           a = num[i][j];
#else
           a = getPolyIntValue(polyIntInfo, i, j, n1);
#endif         
           /* won't need = 0, because only non-zero stored.  If denom is really small-- it is not stored */          
           if (a != 0.0) 
           {
               frac = a/b; /* a = val(k), b is known above */
               if (valuesOK && ((frac <=0.0) || (frac > 1.0)))
               {
                  sprintf(msg,
                     "At least one surrogate for attribute %d, data polygon %d is not between 0 and 1\n",
                      attr_num, data_poly_id);
                  WARN(msg);
                  valuesOK = 0;                  
               }
            
                        
               qasum += frac;
               sprintf(qasum_str,"\t%lf",qasum);
               sprintf(numerator_str,"\t%lf",a);
               sum_a += a;
               sprintf(denominator_str,"\t%lf",b);
               
               if (output_qasum || output_numerator || output_denominator)
               {
                 if(strcmp(outputType, "RegularGrid")==0 || strcmp(outputType, "EGrid")==0)
                 {
                   if(strcmp(outputType, "RegularGrid")==0) 
                   {
                     lastcol = j%ncols + 1;
                     lastrow = j/ncols + 1;
                   }
                   else
                   {
                     lastcol = (polyID-1)%ncols + 1;
                     lastrow = (polyID-1)/ncols + 1;
                   }
                     
                   if (b >= denomThreshold && strlen(data_poly_id) >= 1) 
                   {
                        fprintf(sfile,"%5d\t%s\t%5d\t%5d\t%10.8lf\t!%s%s%s\n",
                           attr_num, data_poly_id, lastcol, lastrow,frac,
                           numerator_str, denominator_str, qasum_str);
                   }
                   else
                   {
                      fprintf(sfile,
                        "#SKIPPED %5d\t%s\t%5d\t%5d\t%10.8lf\t!%s%s%s\n",attr_num, 
                        data_poly_id, lastcol, lastrow,frac,
                        numerator_str, denominator_str, qasum_str);

                      sprintf(mesg,
                        "#SKIPPED %5d\t%s\t%5d\t%5d\t%10.8lf\t!%s%s%s\n",attr_num, 
                        data_poly_id, lastcol, lastrow,frac,
                        numerator_str, denominator_str, qasum_str);
                      MESG(mesg);
                   }
                 }
                 else if (strcmp(outputType, "Polygon")==0)
                 {
                   if (b >= denomThreshold && strlen(data_poly_id) >= 1) 
                   {
                       fprintf(sfile,"%5d\t%s\t%s\t%10.8lf\t!%s%s%s\n",
                          attr_num, data_poly_id, out_poly_id, frac,
                          numerator_str, denominator_str, qasum_str);
                   }
                   else
                   {
                      fprintf(sfile,"#SKIPPED %5d\t%s\t%s\t%10.8lf\t!%s%s%s\n",
                          attr_num, data_poly_id, out_poly_id, frac,
                          numerator_str, denominator_str, qasum_str);

                      sprintf(mesg,"#SKIPPED %5d\t%s\t%s\t%10.8lf\t!%s%s%s\n",
                          attr_num, data_poly_id, out_poly_id, frac,
                          numerator_str, denominator_str, qasum_str);
                      MESG(mesg);
                   }
                 }
               }  /*output with QA*/
               else /* just output the basic surrogate*/
               {
                  /* just output the basic surrogate info */
                  if(strcmp(outputType, "RegularGrid")==0 || strcmp(outputType, "EGrid")==0 )
                  {
                    if(strcmp(outputType, "RegularGrid")==0)
                    {
                       lastcol = j%ncols + 1;
                       lastrow = j/ncols + 1;
                    }
                    else
                    {
                       lastcol = (polyID-1)%ncols + 1;
                       lastrow = (polyID-1)/ncols + 1;
                    }
                    if (b >= denomThreshold && strlen(data_poly_id) >= 1)
                    {
                       fprintf(sfile,"%5d\t%s\t%5d\t%5d\t%10.8lf\n",
                         attr_num, data_poly_id, lastcol, lastrow, frac);
                    }
                    else 
                    {
                       fprintf(sfile,"#SKIPPED %5d\t%s\t%5d\t%5d\t%10.8lf\n",
                         attr_num, data_poly_id, lastcol, lastrow, frac);
                       sprintf(mesg,"#SKIPPED %5d\t%s\t%5d\t%5d\t%10.8lf\n",
                         attr_num, data_poly_id, lastcol, lastrow, frac);
                       MESG(mesg);
                    }
                  }
                  else if (strcmp(outputType, "Polygon")==0)
                  {
                    if (b >= denomThreshold && strlen(data_poly_id) >= 1)
                    {
                      fprintf(sfile,"%5d\t%s\t%s\t%10.8lf\n",
                         attr_num, data_poly_id, out_poly_id, frac);   
                    }
                    else
                    {
                      fprintf(sfile,"#SKIPPED %5d\t%s\t%s\t%10.8lf\n",
                         attr_num, data_poly_id, out_poly_id, frac); 
                      sprintf(mesg,"#SKIPPED %5d\t%s\t%s\t%10.8lf\n",
                         attr_num, data_poly_id, out_poly_id, frac); 
                      MESG(mesg);
                    }
                 }              
               } /*end no QA output*/                     

               num_gridsum[j] += a;                             
             }  /* if a != 0 */
          }  /* for (j= 0; j < n2; j++) */   
      }  /* if (denom[i] != 0) */
      else  /* denom[i] == 0 */
      {
        data_poly_id[0] = '\0';
        if (attrtype == FTInteger) {
          sprintf(data_poly_id,"%d",d_poly->attr_val[i][0].ival);
        }
        else if (attrtype == FTString) {
          sprintf(data_poly_id,"%s",d_poly->attr_val[i][0].str);
        }
        else {
          sprintf(data_poly_id,"poly%d",i);
        }
#ifdef DEBUG
        fprintf(stderr,"zero for %s\n", data_poly_id);
#endif
      } /* denom[i] == 0 */ 
    }
    free(denom);
#ifdef OLD_SUM
    for (i=0; i<n1; i++) {
      free(num[i]);
    }
    free(num);
#endif    
  }
  fclose(sfile);

  if (num_gridsum != NULL)
  { 
     writeOutputGridShapeFile(num_gridsum, g_poly, outputGridFileName);
     free(num_gridsum);
  }

  return 1;
 error:
  WARN(mesg);
  return 0;
}

