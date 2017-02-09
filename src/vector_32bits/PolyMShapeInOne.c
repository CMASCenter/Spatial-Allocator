/********************************************************************************
 * PolyMShapeInOne.c
 *
 * File contains:
 * PolyMShapeInOne
 *
 * Created: Nov. 2005, process PolyObject to have all shapes with the same ID into one record.  LR
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
#include "io.h"                 /* added 4/4/2005 BB */
#define COPIED_PS_ID_I -888888
#define COPIED_PS_ID_S "-888888"

                                                                    
/*
 * read through polygons and attributes from a PolyObject
 * Put all shapes with the same ID into obe shape record
 */
int PolyMShapeInOne(PolyObject * poly)
{
    extern char *prog_name;
    
    PolyShapeList *plist,*oldplist;
    Shape *shp;
    PolyShape *ps,*oldps;    
    
    int *iIDs;  /*define integer ID array*/
    char **sIDs; /*define string ID array*/
    char sID[80];
    int iID;
    
    int attrtype;
    int nObjects;  /*number of objects in poly*/
    int newPolyID = 0;   /*0=not new Poly ID  1=new Poly ID*/
    
    BoundingBox *polyBB,*newBB,*psBB,*oldpsBB;       /* bounding box for the poly */
    double xmin, ymin, xmax, ymax;
    double new_xmin, new_ymin, new_xmax, new_ymax;
    double ps_xmin, ps_ymin, ps_xmax, ps_ymax;
    int i,j,m,n,k;
    Vertex *v;
    int np, nv;
    int isHole;                                                                   
    char mesg[256];
    
    PolyShapeList **pslPointers;  /*define a pointer array to PolyShapes*/
    
    MESG("PolyMShapeInOne.c -- put multiple shapes with the same ID into one PolyObject...");
    
    if(poly == NULL){
       MESG("PolyObject is empty in PolyMShapesInOne program\n");  
       return 1; /*failed*/
    }
    /*initialize new bounding box*/
    polyBB = poly->bb;
    if (polyBB != NULL) {   
      new_xmin = polyBB->xmin;
      new_ymin = polyBB->ymin;
      new_xmax = polyBB->xmax;
      new_ymax = polyBB->ymax;        
    }
    else
    {
      new_xmin = 1E20;
      new_xmax = -1E20;
      new_ymin = 1E20;
      new_ymax = -1E20;
    }
    
    nObjects = poly->nObjects;  /*number of objects in poly*/
    if(nObjects == 0)
    {
        sprintf(mesg, "Base data shapefile contains no polygons");
        ERROR(prog_name, mesg, 2);
    }
    sprintf(mesg,"Number of shape Objects = %d\n",nObjects);
    MESG (mesg);
    
    pslPointers = (PolyShapeList **)malloc(nObjects*sizeof(PolyShapeList *)); 

    /* only one attribute for base data poygons */
    if (poly->attr_hdr) {
      if (poly->attr_hdr->attr_desc[0]->type == FTInteger) {
        MESG("Base data poly attribute type is INT\n");
        attrtype = FTInteger;
        iIDs = (int *) malloc(nObjects * sizeof(int));
      }
      else if (poly->attr_hdr->attr_desc[0]->type == FTString) {
        MESG("Base data poly attribute type is STRING\n");
        attrtype = FTString;
        sIDs = (char **) malloc(nObjects * sizeof(char *));
      }
      else {
        ERROR(prog_name, "Base data polyon ID is not integer or string type", 2);
      }
   }
   else {
     ERROR(prog_name,"NO ATTRIBUTE HEADER was found for base data polygons\n",2);
   }

   /*check each object for the same attribute*/
   plist = poly->plist;
   for(i = 0; i < nObjects; i++)
   {  
       if(plist == NULL)
       {
         fprintf(stdout,"Number of ps Object: i=%d\n",i);
         ERROR(prog_name,"plist NULL",2);
       }

       /*initialize current plist's bounding box*/
       psBB = plist->bb;
       if(plist->bb == NULL)
       {
          fprintf(stderr,"plist->bb was null for ps Object: i=%d\n",i);
          computeBoundingBox(plist->ps, &ps_xmin, &ps_ymin, &ps_xmax, &ps_ymax);
          plist->bb = newBBox(ps_xmin,ps_ymin,ps_xmax,ps_ymax);
       }
       else {    
         ps_xmin = psBB->xmin;
         ps_xmax = psBB->xmax;
         ps_ymin = psBB->ymin;
         ps_ymax = psBB->ymax;
       }
             
       newPolyID = 1;     /*assume new poly ID*/        
       if (attrtype == FTInteger) 
       {
         iID = poly->attr_val[i][0].ival;
         for (j=0;j<i;j++)
         {
           if (iID == iIDs[j] && iID != COPIED_PS_ID_I && iID != 0)
           {
             newPolyID = 0;  /*old poly id*/   
             oldplist = pslPointers[j];
             /*sprintf(mesg,"Same ID for i = %d and ID = %d", i,iID);
             MESG(mesg);*/
             break;  
           }      
         }
       } 
       else
       {
         strcpy(sID,poly->attr_val[i][0].str);
         for (j=0;j<i;j++)
         {
           if (strcmp(sID,sIDs[j])==0 && strcmp(sID,COPIED_PS_ID_S)!=0 && strcmp(sID,"")!=0)
           {
             newPolyID = 0;  /*old poly id*/      
             oldplist = pslPointers[j];        
             /*sprintf(mesg,"Same ID for i = %d and ID = %s", i,sID);
             MESG(mesg);*/
             break;  
           }      
         }
       }
 
      if (newPolyID == 1)
      {  /*new Polyshape and store ID array*/
        if (attrtype == FTInteger) 
        {
          iIDs[i] = iID;      
        }
        else 
        {
          sIDs[i] = (char *) strdup(sID);
        }               
      }  /*end of if newPolyID = 1*/
      else
      {  /*old poly ID shape and need to attach to old poly shape*/
         
         /*if it is old ID, append ps to old one and update shape BB
          change ID code, bb, and shape for new PS*/
         ps = plist->ps;
         oldps = oldplist->ps; 
         np = ps->num_contours;
         for(n = 0; n < np; n++) 
         { 
            nv = ps->contour[n].num_vertices;
            shp = getNewShape(nv);
            v = ps->contour[n].vertex;
            isHole = ps->hole[n];
            for(k = 0; k < nv; k++)
            {
               shp->vertex[k]=v[k]; 
               ps_xmin = MIN(ps_xmin, v[k].x);
               ps_xmax = MAX(ps_xmax, v[k].x);
               ps_ymin = MIN(ps_ymin, v[k].y);
               ps_ymax = MAX(ps_ymax, v[k].y);
            }
            gpc_add_contour(oldps,shp,isHole);       
          }  /*end of np loop*/
          oldplist->bb = newBBox(ps_xmin,ps_ymin,ps_xmax,ps_ymax);
          new_xmin = MIN(new_xmin,ps_xmin);
          new_xmax = MAX(new_xmax,ps_xmax);
          new_ymin = MIN(new_ymin,ps_ymin);
          new_ymax = MAX(new_ymax,ps_ymax);
          /*create a dummy shape with only 1 vertex outside poly bounding box to replace current ps*/
          shp = getNewShape(1);
          shp->vertex[0].x = v[0].x;
          shp->vertex[0].y = v[0].y;
          ps->num_contours = 0;  /*to get rid of old contours*/
          gpc_add_contour(ps,shp,NOT_A_HOLE);
          if (attrtype == FTInteger) 
          {
              poly->attr_val[i][0].ival = COPIED_PS_ID_I;
              iIDs[i] = COPIED_PS_ID_I;
          }
          else 
          {
             strcpy(poly->attr_val[i][0].str,COPIED_PS_ID_S);
             sIDs[i] = (char *) strdup(COPIED_PS_ID_S); 
          }     
       }  /*end of if newPolyID != 1*/
       pslPointers[i] = plist;    
       plist = plist->next;      
  } /*end of i looop*/                        
  
  /*handle the bounding box*/  
   newBB = newBBox(new_xmin, new_ymin, new_xmax, new_ymax);
   MESG("New limiting bounding box: ");
   printBoundingBox(newBB);
   MESG("Existing limiting bounding box: ");
   printBoundingBox(polyBB);
   if (polyBB == NULL) {  
     poly->bb = newBB;  /*only update it when it is NULL*/   
   }

   recomputeBoundingBox(poly);

   if (attrtype == FTInteger)
   {
      free(iIDs);
   }
   else
   {
      free(sIDs);
   }
   return 0;  /*success*/
}   
