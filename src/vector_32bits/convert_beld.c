/* convert_beld.c
 * Written by Atanas Trayanov, MCNC Environmental Modeling Center in support of
 * the EPA Multimedia Integrated Modeling System, 2002.
 *
 * Put in its own file 4/5/2005 BB
 */

#include <stdio.h>
#include <stdlib.h> 
#include <string.h>
#include <math.h>

#include "shapefil.h"
#include "mims_spatl.h"
#include "mims_evs.h"
#include "parms3.h"



/* ============================================================= */
/* Create output for convert process.  
 * This is very similar to, but not the same as, the surrogate 
 * calculations. The primary difference is in the resetting of 
 * a and b if either is less than 0, and the output of a and
 * b in addition to the fraction. */
/* put in convert_beld.c */
int createConvertOutput(PolyObject *poly, char *ename)
{
  double **num;
  double *denom;
  double frac, a, b;
  int n1, n2, d1;
  int i, j;
  /* weight, data, and grid polygons, plus intersected weight & data polys */  
  PolyObject *wd_poly, *d_poly, *g_poly, *w_poly;
    int attrtype;
  int nrows;
  char data_poly_id[80];
  int attr_id;
  char mesg[256];

  FILE *sfile;
  char *fname;
  char *mims_hdr;
  int output_hdr;

  char last_data_poly_id[80];
  int use_weight_val = 0; /* don't need attribute weight for this */

  PolyIntStruct *polyIntInfo;
   
  /* retrieve name of and open output file for surrogates */  
  if (ename == NULL) {
    sprintf(mesg,"%s","No logical name specified for output");
    goto error;
  }

  fname = (char *) strdup(getenv(ename));
  if (fname == NULL ) {
    sprintf(mesg,"Env. var. \"%s\" not set", ename);
    goto error;
  }
  
  sfile = fopen(fname,"w");
  if (sfile == NULL) {
    sprintf(mesg,"Cannot open file \"%s\" for writing", fname);
    goto error;
  }

  wd_poly = poly->parent_poly1;
  d_poly = wd_poly->parent_poly2;
  w_poly = wd_poly->parent_poly1;
  g_poly = poly->parent_poly2;

  output_hdr = 1;
  mims_hdr=getenv("WRITE_HEADER");
  if (mims_hdr != NULL) {
    if (!strcmp(mims_hdr,"NO")) {
      output_hdr = 0;
    }
  }

  print_hdr(sfile, g_poly);


  if (d_poly->attr_hdr) {
    if (d_poly->attr_hdr->attr_desc[0]->type == FTInteger) {
      attrtype = FTInteger;
    }
    else if (d_poly->attr_hdr->attr_desc[0]->type == FTString) {
      attrtype = FTString;
    }
    else {
      attrtype = FTInvalid;
    }
  }
  else {
    attrtype = FTInvalid;
  }
  
  for (attr_id=0; attr_id<w_poly->attr_hdr->num_attr ;attr_id++) {
    /* compute the numerator for the surrogate fraction */    
#ifdef OLD_SUM  
    if (sum2Poly(poly, &num, &n1, &n2, attr_id) !=0 ) return 1;
#else    
    if (sum2Poly(poly, &num, &n1, &n2, attr_id, &polyIntInfo, use_weight_val) !=0 ) return 1;
#endif    
    /* compute the denominator for the surrogate fraction */    
    if (sum1Poly(wd_poly, &denom, &d1, attr_id, use_weight_val) !=0 ) return 2;
    if (n1 != d1) return 3;
    nrows = g_poly->map->nrows;
    
    last_data_poly_id[0] = '\0';
    for (i=0; i<n1; i++) {
      if ((b=denom[i]) != 0.0) {
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
        for (j=0; j<n2; j++) {
          /*frac = 1/b;*/  /* this line seems unncessary since frac is reset below */
          
#ifdef OLD_SUM           
          /* if the next line is commented out, then the memory usage doesn't go above
           * 60 in this method, otherwise it goes to > 300 */
          if ((a=num[i][j]) != 0.0)
#else
          a = getPolyIntValue(polyIntInfo, i, j, n1);
          if (a == MISSING)
          {
             ERROR("createConvertOutput",
                "getPolyIntValue failed in createConvertOutput", 2);
          }
          else if (a != 0.0)
#endif
          {
            frac = a/b;

            /** if a or b < 0, set a to 0 **/
            if( b < 0.0 || a < 0.0 ){
               frac = -a; 
               b = 0;
               a = 0;
            }

            /* output attribute info, the fraction, plus a and b */
            fprintf(sfile,
                      "%5d %5s %5d %5d %15.6f %15.6f %15.6f\n",
                      w_poly->attr_hdr->attr_desc[attr_id]->category,
                      data_poly_id,j/nrows +1, j%nrows+1, frac, a, b);
           
          }          
        }
        fflush(sfile);
      }
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
  
  return 1;
 error:
  WARN(mesg);
  return 0;
}

