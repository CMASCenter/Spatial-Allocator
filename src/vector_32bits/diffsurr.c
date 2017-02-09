/*
 * This program compares two surrogate files and notifies the user if 
 * the files have any differences larger than the specified tolerance.
 * The program expects the following command line arguments:
 *
 * srgt_file1 srgt_category_num_1 srgt_file2 srgt_category_num2 tolerance,
 *
 * The surrogate entries for the categories are looked up in the respective
 * surrogate files, and if the absolute value of the difference between
 * corresponding values is larger than the tolerance, an warning is logged.
 *
 * Developed by Atanas Trayanov of MCNC Environmental Modeling Center,
 * in support of the EPA Multimedia Integrated Modeling System, 2002.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "io.h"

typedef struct _Surrogate {
  float frac;
  int   id;
  int   col;
  int   row;
} Surrogate;

typedef struct _SurrogateList {
  float frac;
  int   id;
  int   col;
  int   row;
  struct _SurrogateList *next;
} SurrogateList;

char mesg[256];
char *prog_name;
char *prog_version = "Spatial Allocator Diffsurr Version 3.6 03/10/2009";

extern int debug_output;

int ReadSrgtFile(char *fname, int scat, Surrogate **s, double eps);

/* =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */

int compare_surrogates1(int n, Surrogate *a, Surrogate *b, float eps)
{
  int i;
  int flag=1;
  int numdiffs = 0;
  int didbreak = 0;

   for (i=0; i<n; i++) {
    if (a[i].id != b[i].id) {
      sprintf(mesg,"County IDs differ: %d %d %d %f != %d %d %d %f",
          a[i].id, a[i].col, a[i].row, a[i].frac,
		  b[i].id, b[i].col, b[i].row, b[i].frac);
      MESG(mesg);
      flag = 0;
	  didbreak = 1;
      break;
    }
    if (a[i].col != b[i].col) {
      sprintf(mesg,"Grid cell columns differ: %d %d %d %f != %d %d %d %f",
          a[i].id, a[i].col, a[i].row, a[i].frac, 
		  b[i].id, b[i].col, b[i].row, b[i].frac);
      MESG(mesg);    
      flag = 0;
	  didbreak = 1;
      break;
    }
    if (a[i].row != b[i].row) {
      sprintf(mesg,"Grid cell rows differ: %d %d %d %f != %d %d %d %f",
          a[i].id, a[i].col, a[i].row, a[i].frac,
		  b[i].id, b[i].col, b[i].row, b[i].frac);
      MESG(mesg);    
      flag = 0;
	  didbreak = 1;
      break;
    }
#ifdef DEBUG    
    sprintf(mesg,
          "n=%d Id=%d Col=%d Row=%d FRAC1=%f, FRAC2 = %f, DIFF=%f", 
          n, a[i].id, a[i].col, a[i].row,a[i].frac, b[i].frac, a[i].frac-b[i].frac);
    MESG(mesg);      
#endif    
    if (fabs(a[i].frac - b[i].frac) > eps) {
      sprintf(mesg,
          "Fractions differ for %d, %d, %d: %f vs %f, diff=%f", 
          a[i].id, a[i].col, a[i].row,a[i].frac, b[i].frac, a[i].frac-b[i].frac);
      MESG(mesg);
      flag = 0;
      numdiffs++;
    }
  }
  if (didbreak)
  {
	  WARN("Comparison stopped because ID or grid cell differed");
	  return flag;
  }
  /* only fractions differed */
  if (numdiffs > 0)
  {
     sprintf(mesg,"%d total differences out of %d possible\n",
		 numdiffs, n);
     WARN(mesg);
  }   
  return flag;
}

int compare_surrogates(int na, int nb, Surrogate *a, Surrogate *b, float eps)
{
  int i = 0, j = 0;
  int flag=1;
  int numdiffs = 0;
  int didbreak = 0;


  while ((i < na) && (j < nb))
  {
    if (a[i].id != b[j].id) {
	  /* handle the case when they are out of sync due to small values */
	  while ((a[i].id < b[j].id) && (a[i].frac <= eps) && (i < na))
	  {
		 i++;
	     sprintf(mesg,"County mismatch: Skipping small value for i=%d, val=%f",
			 i, a[i-1].frac);
         MESG(mesg);
	  }
	  while ((a[i].id > b[j].id) && (b[j].frac <= eps) && (j < nb))
	  {
		 j++;
	     sprintf(mesg,"County mismatch: Skipping small value for j=%d, val=%f",
			 j, b[j-1].frac);
         MESG(mesg);
	  }
	}
    if (a[i].id != b[j].id) 
	{
      sprintf(mesg,"County IDs differ: %d %d %d %f != %d %d %d %f",
          a[i].id, a[i].col, a[i].row, a[i].frac,
		  b[j].id, b[j].col, b[j].row, b[j].frac);
      MESG(mesg);
      flag = 0;
	  didbreak = 1;
      break;
    }
    if (a[i].col != b[j].col) {
	  while ((a[i].col < b[j].col) && (a[i].frac <= eps) && (i < na))
	  {
		 i++;
	     sprintf(mesg,"Column mismatch: Skipping small value for i=%d, val=%f",
			 i, a[i-1].frac);
         MESG(mesg);
	  }
	  while ((a[i].col > b[j].col) && (b[j].frac <= eps) && (j < nb))
	  {
  	     j++;
	     sprintf(mesg,"Column mismatch: Skipping small value for j=%d, val=%f",
			 j, b[j-1].frac);
         MESG(mesg);
	  }
	}
    if (a[i].col != b[j].col) {
      sprintf(mesg,"Grid cell columns differ: %d %d %d %f != %d %d %d %f",
          a[i].id, a[i].col, a[i].row, a[i].frac, 
		  b[j].id, b[j].col, b[j].row, b[j].frac);
      MESG(mesg);    
      flag = 0;
	  didbreak = 1;
      break;
    }
    if (a[i].row != b[j].row) {
	  while ((a[i].row < b[j].row) && (a[i].frac <= eps) && (i < na))
	  {
		 i++;
	     sprintf(mesg,"Row mismatch: Skipping small value for i=%d, val=%f",
			 i, a[i-1].frac);
         MESG(mesg);
	  }
	  while ((a[i].row > b[j].row) && (b[j].frac <= eps) && (j < nb))
	  {
  	     j++;
	     sprintf(mesg,"Row mismatch: Skipping small value for j=%d, val=%f",
			 j, b[j-1].frac);
         MESG(mesg);
	  }
	}
    if (a[i].row != b[j].row) {
      sprintf(mesg,"Grid cell rows differ: %d %d %d %f != %d %d %d %f",
          a[i].id, a[i].col, a[i].row, a[i].frac,
		  b[j].id, b[j].col, b[j].row, b[j].frac);
      MESG(mesg);    
      flag = 0;
	  didbreak = 1;
      break;
    }
#ifdef DEBUG    
    sprintf(mesg,
          "n=%d Id=%d Col=%d Row=%d FRAC1=%f, FRAC2 = %f, DIFF=%f", 
          n, a[i].id, a[i].col, a[i].row,a[i].frac, b[j].frac, a[i].frac-b[j].frac);
    MESG(mesg);      
#endif    
    if (fabs(a[i].frac - b[j].frac) > eps) {
      sprintf(mesg,
          "Fractions differ for %d, %d, %d: %f vs %f, diff=%f", 
          a[i].id, a[i].col, a[i].row,a[i].frac, b[j].frac, a[i].frac-b[j].frac);
      MESG(mesg);
      flag = 0;
      numdiffs++;
    }
	i++;
	j++;
  }
  if (didbreak)
  {
	  WARN("Comparison stopped because ID or grid cell differed");
	  return flag;
  }
  /* only fractions differed */
  if (numdiffs > 0)
  {
     sprintf(mesg,"%d total differences out of %d possible\n",
		 numdiffs, na);
     WARN(mesg);
  }   
  return flag;
}


/* =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */
/* =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */
/* =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */
int main(int argc, char *argv[])
{

  char *sfile1, *sfile2;
  int scat1, scat2;
  Surrogate *a, *b;
  int n_items_a, n_items_b;
  float eps;

  debug_output = 1;
  prog_name = argv[0];

  MESG(prog_version);

  /* process command line args */
  if (argc != 6) {
    fprintf(stderr,
	    "Usage: %s srgt_file1 srgt_category1 srgt_file2 srgt_category2 tolerance\n",
	    argv[0]);
    return 1;
  }

  sfile1 = argv[1];
  scat1  = atoi(argv[2]);
  sfile2 = argv[3];
  scat2  = atoi(argv[4]);
  eps = (float)atof(argv[5]);

  sprintf(mesg,"Tolerance = %f",eps);
  MESG(mesg);

  n_items_a = ReadSrgtFile(sfile1, scat1, &a, eps);
  if (n_items_a < 0)
  {
     if (a != NULL) free(a);
     ERROR(argv[0], "Could not read surrogate", 1);
     return 1; /*error*/
  }   
  if (n_items_a == 0) {
    sprintf(mesg,
       "No surrogate entries for category %d were found in file %s\n",
       scat1, sfile1);
    if (a != NULL) free(a);
    ERROR(argv[0], mesg, 1);
  }

  n_items_b = ReadSrgtFile(sfile2, scat2, &b, eps);
  if (n_items_b < 0) 
  {
     if (a != NULL) free(a);
     if (b != NULL) free(b);
     ERROR(argv[0], "Could not read surrogate", 1);
     return 1; /*error*/
  }   
  if (n_items_b == 0) {
     sprintf(mesg,
        "No surrogate entries for category %d were found in file %s\n",
        scat2, sfile2);
     if (a != NULL) free(a);
     if (b != NULL) free(b);
     ERROR(argv[0], mesg, 2);
  }

  /*if (n_items_a != n_items_b) {
     if (a != NULL) free(a);
     if (b != NULL) free(b);
	 sprintf(mesg,
		 "The number of surrogate entries for categories %d and %d differs",
		 scat1, scat2);
     ERROR(argv[0], mesg, 3);
  }*/

  if (!compare_surrogates(n_items_a, n_items_b, a, b, eps)) {
     if (a != NULL) free(a);
     if (b != NULL) free(b);
	 sprintf(mesg,"The surrogate values differ for categories %d and %d\n",
	    scat1, scat2);	 
     ERROR(argv[0], mesg, 4);
  }

  MESG("The surrogate comparison was successful!\n");
  return 0;
}

/* =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */
/* =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */
int new_surr(SurrogateList **list, int id, int col, int row, float frac)
{
  SurrogateList *p;
  SurrogateList **t;

  p = (SurrogateList *)malloc(sizeof(p[0]));
  if (p) {
    p->id  = id;
    p->col = col;
    p->row = row;
    p->frac = frac;
    p->next = NULL;
    for (t=list; *t != NULL; t = &(*t)->next) {};
    *t = p;

    return 1;

  }
  else {
    WARN("Allocation error in new_surr");
    return 0;
  }
}

/* =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */

#define MAXLINE 2000

/*
 * trims the spaces from before and after the string
 */


int ReadSrgtFile(char *fname, int scat, Surrogate **s, double eps)
{
  FILE *fp;
  int n;
  int i;
  int m;
  int line; 
  int col, row, id, cat;
  float frac;
  char buffer[MAXLINE];
  char buffer2[MAXLINE];
  SurrogateList *sl = NULL;
  SurrogateList *t;
  double tooSmallToRead;

  extern int comp_surr(const void *, const void *);

  if ((fp = fopen(fname,"r")) == NULL) {
    WARN2("Cannot open file for reading: ", fname);
    n = -1;
    return n;
  }

  /* skip header (1 line) */
  for (i=0; i<1; i++) {
    if (fgets(buffer, MAXLINE, fp) == NULL) {
      WARN2("Cannot read the first line of surrogate file ", fname);
      n = -1;
      goto cleanup;
    }
  }

  n = 0;
  line = 1;
  tooSmallToRead=eps/100;

  while (fgets(buffer, MAXLINE, fp) != NULL) {
    if ((strlen(buffer) > 0) && (buffer[0] != '#'))
    {
      trim(buffer,buffer2);
      if (strlen(buffer2) > 0)
      {
         m = sscanf(buffer2,"%d %d %d %d %f", &cat, &id, &col, &row, &frac);
         if (m != 5)
         {
            sprintf(buffer, 
              "Failed to read the five required values from line %d of file\n%s\n%s",
              line, fname, buffer2);
            WARN(buffer);
            n = -1;
            goto cleanup;
         }
         if(cat==scat) 
		 {
			//if (frac > tooSmallToRead)  
			//{
               if (!new_surr(&sl, id, col, row, frac)) 
			   {
                  n = -1;
                  goto cleanup;
			   }
               n++;
            //}
			//else /* if the surrogate value is really tiny, skip it */
			//{
			//	sprintf(mesg,"Skipping line: %s",buffer2);
			//	MESG(mesg);
			//}
         }   
      }   
    }
    line++;
  }
  sprintf(buffer,"Read %d entries for category %d from file %s",
     n, scat, fname);
  MESG(buffer);

  if (n==0) goto cleanup;

  *s = (Surrogate *)malloc(n*sizeof(Surrogate));
  if (*s == NULL) {
    WARN("Malloc failure in ReadSrgtFile");
    n = -1;
    goto cleanup;
  }

  t = sl;
  for (i=0; i < n; i++) {
    (*s)[i].id = t->id;
    (*s)[i].col = t->col;
    (*s)[i].row = t->row;
    (*s)[i].frac = t->frac;
    t = t->next;
  }

  qsort((*s), n, sizeof(Surrogate), comp_surr);

  /* printing surrogate can be useful for debugging */
  /*fprintf(stdout,"file = %s, cat = %d\n",fname, scat);
  printsurr(*s, n);*/

  /*
    free_surr(sl);
  */
 cleanup:
  fclose(fp);

  return n;
}

/* =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */
int comp_surr(const void *v1, const void *v2)
{
  Surrogate *V1 = (Surrogate *)v1;
  Surrogate *V2 = (Surrogate *)v2;

  if (V1->id < V2->id) return -1;
  if (V1->id > V2->id) return  1;

  /* we get here only if id's are the same */
  if (V1->col < V2->col) return -1;
  if (V1->col > V2->col) return  1;

  /* we get here only if id's and cols are the same */

  if (V1->row < V2->row) return -1;
  if (V1->row > V2->row) return  1;

  return 0;

}
/* =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */

int printsurr(Surrogate *a, int n)
{

  int i;

  for (i=0; i<n; i++) {
      fprintf(stdout,"%d %d %d %.6f\n",
          a[i].id, a[i].col, a[i].row, a[i].frac);
  }        
  return 0;

}
/* =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= */

