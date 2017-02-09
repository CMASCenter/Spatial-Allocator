/* allocateIoapi.c 
 * 
 * 6/22/2005 CAS -- copied from allocate.c
 * 9/21/2005 CAS -- updates for better layer handling
 * 10/05/2006 LR -- Added type area percent compuation for a attribute
 */

#ifdef USE_IOAPI

#include "shapefil.h"
#include "mims_spatl.h"
#include "mims_evs.h"
#include "parseAllocModes.h"
#include "iodecl3.h"

/* ============================================================= */
/* Write results of AGGREGATE, AVERAGE, or AREAPERCENT operation to output I/O API file.
 */

char mesg[256];
extern char *prog_name;


int getNumTypes(PolyObject * poly, char ***list)
{
  char **list_array;
  int ncnt, nmax;
  int max_size = 2;   /*maximum types for this variable */
  int numTypes = 0;
  PolyShape *ps;
  PolyShapeList *plist;
  int attrtype;
  int nObjects;  
  char strVal[50];
  int intVal;
  double douVal;
  int i, j, ret;
 
  list_array = (char **)malloc(max_size*sizeof(char *));
  if(!list_array)
  {
     sprintf(mesg,"Memory allocation error in getNumTypes()");
     *list = NULL;
     sprintf(mesg, "Unable to allocate memory for unique attribute value string array");
     ERROR(prog_name, mesg, 2);
  }

  nObjects = poly->nObjects;
  if(nObjects == 0)
  {
     sprintf(mesg, "Weight shapefile contains no polygons");
     ERROR(prog_name, mesg, 2);
  } 
 
  /* only one attribute for weight type data poygons */
  if (poly->attr_hdr) {

    /*check the type of the attribute*/
    if (poly->attr_hdr->attr_desc[0]->type == FTInteger) {
      MESG("Base data poly attribute type is INT\n");
      attrtype = FTInteger;
    }
    else if (poly->attr_hdr->attr_desc[0]->type == FTString) {
      MESG("Base data poly attribute type is STRING\n");
      attrtype = FTString;
    }
    else if (poly->attr_hdr->attr_desc[0]->type == FTDouble) {
      MESG("Base data poly attribute type is DOUBLE\n");
      attrtype = FTDouble;
    }
    else {
      ERROR(prog_name, "Weight polyon type is not integer, double or string type", 2);
    }
  }
  else {
    ERROR(prog_name,"NO ATTRIBUTE HEADER was found for weight polygons\n",2);
  }

  numTypes = 0;  
  for(i = 0; i < nObjects; i++)
  { 
     if (attrtype == FTInteger)
     {
        intVal = poly->attr_val[i][0].ival; 
        ret = sprintf(strVal, "%d", intVal);
     }
     else if (attrtype == FTDouble)
     {
        douVal = poly->attr_val[i][0].val; 
        ret = sprintf(strVal, "%lf", douVal);
     }
     else if (attrtype == FTString)
     {
        sprintf(strVal,"%s",poly->attr_val[i][0].str); 
     } 
     else
     {
        sprintf(mesg, "NO ATTRIBUTE value for weight shape %d\n",i);
        ERROR(prog_name,mesg,2);
     }

     for(j=0; j<numTypes;j++)
     {
        if(strcmp(strVal,list_array[j])==0)
        {
           break;
        }
     }
     if (j == numTypes)
     {
        if (numTypes >= max_size)
        {
          max_size *= 2;
          list_array = (char **)realloc(list_array,max_size*sizeof(char *));
          if (!list_array)
          {
             sprintf(mesg,"Memory re-allocation error in getNumTypes");
             ERROR(prog_name,mesg,2);
          }
        } 
        list_array[numTypes] = (char *) strdup(strVal);
        numTypes++;
        sprintf(mesg, "numTypes=%d   strVal=%s\n",numTypes,strVal);
        MESG(mesg);
     }
   
  }  

  /*attribute type array will be return to the calling program*/
  *list = list_array;
  return numTypes;
}


int allocateIoapi(PolyObject *poly,   /* intersected weight and data polygons */
                  char *ename,        /* environment variable for output file */
                  int use_weight_val, /* true: use weight attributes */
                  int input_ioapi)    /* true: attributes came from input I/O API file */
{
    double *sum;
    int *max = NULL;
    int *centroid = NULL;
    int overlapsCalculated = 0;
    int centroidsCalculated = 0;
    PolyObject *d_poly, *w_poly;
    int attrtype;
    int attr_id,i;
    int dPolyNumAttr, wPolyNumAttr;

    char name[100], modeFileName[50];
    char attrType[50];    /*type of attribute such as SURF_ZONE*/
    amode mode;

    IOAPI_Bdesc3 bdesc, bdesc_in;
    IOAPI_Cdesc3 cdesc, cdesc_in;
    char varlist[MXVARS3][NAMLEN3+1];
    int found;
    int nInputVars, varnum;
    int recno, nrec_in, nrec_out;
    char *attrname;
    int jdate, jtime;
    int mxrec, nlays;
    int tstep, layer;
    int tmp_id;
    
    /*area percent calculation*/
    int numTypes = 0;
    char typeNameHead[50],typeName[50];
    char **typeList;
    double **areaP;   /*2d array to store area percent for each type of a attribute in a grid*/
    double gridArea;  /*grid area*/
    
    int    *intAttData, *intAllData, *tmpIntData;
    float  *fltAttData, *fltAllData, *tmpFltData;
    double *dblAttData, *dblAllData, *tmpDblData;

    /* specify the data and weight polygons */
    /* data is output poly, weight is input poly */
    d_poly = poly->parent_poly2;
    w_poly = poly->parent_poly1;

    fprintf(stderr,"In I/O API writer\n");
    /* get the output file name */
    fprintf(stderr,"ename=%s\n",ename);
    if(!getEnvtValue(ename, name))
    {
        sprintf(mesg, "Unable to continue because %s has not been set", ename);
        ERROR(prog_name, mesg, 1);
    }

    /* get the allocation mode file name */
    if(!getEnvtValue(ENVT_ALLOC_MODE_FILE, modeFileName))
    {
        sprintf(mesg, "Unable to continue because %s has not been set",
                modeFileName);
        ERROR(prog_name, mesg, 1);
    }
    //fprintf(stderr,"modefilename=%s\n",modeFileName);

    parseAllocModes(modeFileName);

    /* check that we have polygons to output - we assume the output is a grid */
    if(d_poly->nSHPType == SHPT_POLYGON)
    {
         MESG("Opening I/O API output file\n");
    }
    else
    {
         sprintf(mesg, "Unable to continue, no polygons in %s", d_poly->name);
         ERROR(prog_name, mesg, 1);
    }

    /* if input file was I/O API, get description */
    nInputVars = 0;
    //fprintf(stderr,"input_ioapi=%d\n",input_ioapi);
    if(input_ioapi)
    {
        if(!desc3c(ENVT_INPUT_FILE_NAME, &bdesc_in, &cdesc_in))
        {
            sprintf(mesg, "Unable to get description of input file %s", 
                    ENVT_INPUT_FILE_NAME);
            ERROR(prog_name, mesg, 2);
        }
        
        /* store variable names as null terminated strings */
        nInputVars = bdesc_in.nvars;
        for(varnum = 0; varnum < nInputVars; ++varnum)
        {
            strNullTerminate(varlist[varnum], cdesc_in.vname[varnum], NAMLEN3);
        }
    }

    /* Set up output file description - the I/O API requires blank-padded
       strings that are not null terminated, so use strBlankCopy as needed.
       If input file was I/O API, use it for descriptions where possible. */

    bdesc.ftype = GRDDED3;
    
    /* Set grid projection */
    bdesc.gdtyp = d_poly->map->ctype;
    bdesc.p_alp = d_poly->map->p_alp;
    bdesc.p_bet = d_poly->map->p_bet;
    bdesc.p_gam = d_poly->map->p_gam;
    bdesc.xcent = d_poly->map->xcent;
    bdesc.ycent = d_poly->map->ycent;
    bdesc.xorig = d_poly->map->xorig;
    bdesc.yorig = d_poly->map->yorig;
    bdesc.xcell = d_poly->map->xcell;
    bdesc.ycell = d_poly->map->ycell;

    /* Set grid name and size */
    //fprintf(stderr,"d_poly gridname=%s\n",d_poly->map->gridname);
    strBlankCopy(cdesc.gdnam, d_poly->map->gridname, sizeof(cdesc.gdnam));
    //fprintf(stderr,"d_poly ncols=%d nrows=%d\n",d_poly->map->ncols,d_poly->map->nrows);
    bdesc.ncols = d_poly->map->ncols;
    bdesc.nrows = d_poly->map->nrows;

    if(input_ioapi)
    {
        /* Set layer structure */
        bdesc.nlays = bdesc_in.nlays;
        bdesc.nthik = bdesc_in.nthik;
        bdesc.vgtyp = bdesc_in.vgtyp;
        bdesc.vgtop = bdesc_in.vgtop;
        memcpy(bdesc.vglvs, bdesc_in.vglvs, sizeof(bdesc.vglvs));
        
        /* Set start time and time step */
        bdesc.sdate = bdesc_in.sdate;
        bdesc.stime = bdesc_in.stime;
        bdesc.tstep = bdesc_in.tstep;
        
        /* Set file description */
        memcpy(cdesc.fdesc, cdesc_in.fdesc, sizeof(cdesc.fdesc));
    }
    else
    {
        bdesc.nlays = 1;
        bdesc.nthik = 1;
        bdesc.vgtyp = IMISS3;
        bdesc.vgtop = AMISS3;
        memset(bdesc.vglvs, 0, sizeof(bdesc.vglvs));
        
        bdesc.sdate = 0;
        bdesc.stime = 0;
        bdesc.tstep = 0;
        
        memset(cdesc.fdesc, ' ', sizeof(cdesc.fdesc));
        strBlankCopy(cdesc.fdesc[0], "Created by MIMS Spatial Allocator", MXDLEN3);
    }
    
    /* Set variable names, units, types, and descriptions */
    bdesc.nvars = 0;
    /* Variable arrays that will be set in loops below:
       cdesc.vname, cdesc.units, cdesc.vdesc, bdesc.vtype */
    
    /* loop over all attribs in target poly BDB  added loop 5/18/2005 */
    dPolyNumAttr = d_poly->attr_hdr->num_attr;
    wPolyNumAttr = w_poly->attr_hdr->num_attr;

    //fprintf(stderr,"dpolyattr=%d\n",dPolyNumAttr);
    //fprintf(stderr,"wpolyattr=%d\n",wPolyNumAttr);

    /* define variables -- these are the pass through fields for the data */
    
    /*if compute area percent for each type, skip data poly attributes, only need weight attribute type variables*/
    if(strcmp(modeFileName, "ALL_AREAPERCENT"))
    {
      for(attr_id = 0; attr_id < dPolyNumAttr; attr_id++)
      {
        /* check to see which attributes should be written to the output file */
        if(d_poly->attr_hdr)
        {
            attrtype = d_poly->attr_hdr->attr_desc[attr_id]->type;
            attrname = d_poly->attr_hdr->attr_desc[attr_id]->name;
            
            if(attrtype == FTInteger)
            {
                strBlankCopy(cdesc.vname[bdesc.nvars], attrname, NAMLEN3);
                strBlankCopy(cdesc.units[bdesc.nvars], "UNKNOWN", NAMLEN3);
                strBlankCopy(cdesc.vdesc[bdesc.nvars], attrname, MXDLEN3);
                bdesc.vtype[bdesc.nvars] = M3INT;
                bdesc.nvars++;
            }
            else if(attrtype == FTString)
            {
                sprintf(mesg, 
                  "Unable to store string attribute %s in I/O API file %s", 
                  attrname, name);
                WARN(mesg);
            }
            /* added this block for FTDouble 5/18/2005 BDB */
            else if(attrtype == FTDouble)
            {
                strBlankCopy(cdesc.vname[bdesc.nvars], attrname, NAMLEN3);
                strBlankCopy(cdesc.units[bdesc.nvars], "UNKNOWN", NAMLEN3);
                strBlankCopy(cdesc.vdesc[bdesc.nvars], attrname, MXDLEN3);
                bdesc.vtype[bdesc.nvars] = M3REAL;
                bdesc.nvars++;
            }
            else
            {
                sprintf(mesg,
                  "Unable to store attribute %s with unknown type in I/O API file %s",
                  attrname, name);
                WARN(mesg);
            }
        }
        else
        {
            WARN("No attribute header");
        }
      } /*end of for*/
    }  /*end of string comparison*/

    /* for the allocated attributes */
    /*compute area percent for different types of one attribute, such as surf zone*/
    if(!strcmp(modeFileName, "ALL_AREAPERCENT"))
    {
       /*deal with one variable but with different types.
         Each type will be a varible in output file with area% in grid cell*/
	 numTypes = getNumTypes(w_poly, &typeList);
	 sprintf(mesg,"Number of Type = %d\n",numTypes); 
         MESG(mesg); 

	 /*set new attribute name to: att_name(wieght)_"type"*/
         strcpy(typeNameHead,w_poly->attr_hdr->attr_desc[0]->name);
	 strcat(typeNameHead,"_");
	 for (attr_id = 0; attr_id < numTypes; attr_id++)
	 {
	    strcpy(typeName,typeNameHead);
	    strcat(typeName,typeList[attr_id]);
	    attrname = (char *) strdup(typeName);
          
            sprintf(mesg,"Name of output variable for a type = %s\n", attrname); 
            MESG(mesg);
            
            /*for surf zone area percent computation*/
            if(getEnvtValue(ENVT_ALLOC_ATTR_TYPE, attrType) && strcmp(attrType,"SURF_ZONE")==0)
            {
               if(strcmp(typeList[attr_id],"2")==0)
               {
                  /*it is land, but will be converted to open ocean area*/
                  attrname = (char *) strdup("OPEN");
               }
               else if (strcmp(typeList[attr_id],"3")==0)
               {
                  attrname = (char *) strdup("SURF");
               }
            }
            
	   /* check for attribute in list of I/O API variables - if I/O API
            was not the input, nInputVars will be 0 */
           /* cas - is attribute guaranteed to be in file? */
           found = 0;
           for(varnum = 0; varnum < nInputVars; ++varnum)
           {
              if(!strcmp(varlist[varnum], attrname))
              {
                 found = 1;
                 break;
              }
           }	 
	   if(found)
           {
                memcpy(cdesc.vname[bdesc.nvars], cdesc_in.vname[varnum],
                       sizeof(cdesc_in.vname[varnum]));
                memcpy(cdesc.units[bdesc.nvars], cdesc_in.units[varnum],
                       sizeof(cdesc_in.units[varnum]));
                memcpy(cdesc.vdesc[bdesc.nvars], cdesc_in.vdesc[varnum],
                       sizeof(cdesc_in.vdesc[varnum]));
           }
           else
           {
                strBlankCopy(cdesc.vname[bdesc.nvars], attrname, NAMLEN3);
                strBlankCopy(cdesc.units[bdesc.nvars], "UNKNOWN", NAMLEN3);
                strBlankCopy(cdesc.vdesc[bdesc.nvars], attrname, MXDLEN3);
           }
            
           /* Calculated percent will be real number */
           bdesc.vtype[bdesc.nvars] = M3REAL;
           bdesc.nvars++;		 
	 }

    sprintf(mesg,"Finished writing type variable names...");
    }    
    else
    {
      for(attr_id = 0; attr_id < wPolyNumAttr; attr_id++)
      {
        attrtype = w_poly->attr_hdr->attr_desc[attr_id]->type;
	
        attrname = w_poly->attr_hdr->attr_desc[attr_id]->name;
        
        /* check for attribute in list of I/O API variables - if I/O API
           was not the input, nInputVars will be 0 */
        /* cas - is attribute guaranteed to be in file? */
        found = 0;
        for(varnum = 0; varnum < nInputVars; ++varnum)
        {
            if(!strcmp(varlist[varnum], attrname))
            {
                found = 1;
                break;
            }
        }
        
        mode = getMode(attrname);

        if(mode == Aggregate || mode == Average)
        {
            /* make sure original attribute type is not FTString */
            if(attrtype == FTString)
            {
                sprintf(mesg, 
                  "Unable to perform AGGREGATE or AVERAGE operation on a String attribute");
                ERROR(prog_name, mesg, 1);
            } 
            
            if(found)
            {
                memcpy(cdesc.vname[bdesc.nvars], cdesc_in.vname[varnum],
                       sizeof(cdesc_in.vname[varnum]));
                memcpy(cdesc.units[bdesc.nvars], cdesc_in.units[varnum],
                       sizeof(cdesc_in.units[varnum]));
                memcpy(cdesc.vdesc[bdesc.nvars], cdesc_in.vdesc[varnum],
                       sizeof(cdesc_in.vdesc[varnum]));
            }
            else
            {
                strBlankCopy(cdesc.vname[bdesc.nvars], attrname, NAMLEN3);
                strBlankCopy(cdesc.units[bdesc.nvars], "UNKNOWN", NAMLEN3);
                strBlankCopy(cdesc.vdesc[bdesc.nvars], attrname, MXDLEN3);
            }
            
            /* aggregated or allocated attributes will always be floats */
            bdesc.vtype[bdesc.nvars] = M3REAL;
            bdesc.nvars++;
        }
        else if(mode == DiscreteOverlap || mode == DiscreteCentroid)
        {
            fprintf(stderr,"In discrete mode\n");
            if(attrtype == FTString)
            {
                sprintf(mesg,
                  "Unable to store string attribute %s in I/O API file %s",
                  attrname, name);
                WARN(mesg);
            }
            else if(attrtype == FTInteger)
            {
                if(found)
                {
                    memcpy(cdesc.vname[bdesc.nvars], cdesc_in.vname[varnum],
                           sizeof(cdesc_in.vname[varnum]));
                    memcpy(cdesc.units[bdesc.nvars], cdesc_in.units[varnum],
                           sizeof(cdesc_in.units[varnum]));
                    memcpy(cdesc.vdesc[bdesc.nvars], cdesc_in.vdesc[varnum],
                           sizeof(cdesc_in.vdesc[varnum]));
                }
                else
                {
                    strBlankCopy(cdesc.vname[bdesc.nvars], attrname, NAMLEN3);
                    strBlankCopy(cdesc.units[bdesc.nvars], "UNKNOWN", NAMLEN3);
                    strBlankCopy(cdesc.vdesc[bdesc.nvars], attrname, MXDLEN3);
                }
                bdesc.vtype[bdesc.nvars] = M3INT;
                bdesc.nvars++;
            }
            else if(attrtype == FTDouble)
            {
                fprintf(stderr,"Processing double, found=%d\n",found);
                if(found)
                {
                    memcpy(cdesc.vname[bdesc.nvars], cdesc_in.vname[varnum],
                           sizeof(cdesc_in.vname[varnum]));
                    memcpy(cdesc.units[bdesc.nvars], cdesc_in.units[varnum],
                           sizeof(cdesc_in.units[varnum]));
                    memcpy(cdesc.vdesc[bdesc.nvars], cdesc_in.vdesc[varnum],
                           sizeof(cdesc_in.vdesc[varnum]));
                }
                else
                {
                    strBlankCopy(cdesc.vname[bdesc.nvars], attrname, NAMLEN3);
                    strBlankCopy(cdesc.units[bdesc.nvars], "UNKNOWN", NAMLEN3);
                    strBlankCopy(cdesc.vdesc[bdesc.nvars], attrname, MXDLEN3);
                }
                bdesc.vtype[bdesc.nvars] = M3REAL;
                bdesc.nvars++;
            }
         }
       }
    }
    
    /* Open output I/O API file */
    /* cas - need to append .ncf to file name? */

    fprintf(stderr,"Calling open3c for %s\n",getenv(ename));
    if(!open3c(ename, &bdesc, &cdesc, FSUNKN3, prog_name))
    {
        sprintf(mesg, "Unable to open output I/O API file %s", name);
        ERROR(prog_name, mesg, 1);
    }

    /* check that number of objects equals number of rows times number of columns */
    nrec_out = d_poly->nObjects;
    if(nrec_out != bdesc.nrows * bdesc.ncols)
    {
        sprintf(mesg, "Unexpected number of objects");
        ERROR(prog_name, mesg, 1);
    }
    
    /* allocate space to store data before writing to file */
    intAllData = malloc(bdesc.nrows * bdesc.ncols * bdesc.nlays * sizeof(int));
    fltAllData = malloc(bdesc.nrows * bdesc.ncols * bdesc.nlays * sizeof(float));
        
    /* assign values to variables -- pass through fields */
    /*if compute area percent for each type of a attribute, skip data poly attributes,*/
    if(strcmp(modeFileName, "ALL_AREAPERCENT"))
    {
      for(attr_id = 0; attr_id < dPolyNumAttr; attr_id++)
      {
        attrtype = d_poly->attr_hdr->attr_desc[attr_id]->type;
        attrname = d_poly->attr_hdr->attr_desc[attr_id]->name;
        
        if(attrtype == FTInteger)
        {
            for(recno = 0; recno < nrec_out; ++recno)
            {
                /* for layered output, put first layer's data in all layers */
                for(layer = 1; layer <= bdesc.nlays; ++layer)
                {
                    tmpIntData = intAllData + (layer-1) * nrec_out;
                    tmpIntData[recno] = d_poly->attr_val[recno][attr_id].ival;
                }
            }
            
            if(!write3c(ename, attrname, bdesc.sdate, bdesc.stime, intAllData))
            {
                sprintf(mesg, "Unable to write attribute %s to I/O API file %s",
                        attrname, name);
                ERROR(prog_name, mesg, 2);
            }
        }
        else if(attrtype == FTDouble)
        {
            for(recno = 0; recno < nrec_out; ++recno)
            {
                /* for layered output, put first layer's data in all layers */
                for(layer = 1; layer <= bdesc.nlays; ++layer)
                {
                    tmpFltData = fltAllData + (layer-1) * nrec_out;
                    tmpFltData[recno] = (float) d_poly->attr_val[recno][attr_id].val;
                }
            }
            
            if(!write3c(ename, attrname, bdesc.sdate, bdesc.stime, fltAllData))
            {
                sprintf(mesg, "Unable to write attribute %s to I/O API file %s",
                        attrname, name);
                ERROR(prog_name, mesg, 2);
            }
        } /* already warned about string and unknown types so don't need to
             do anything here */
      }  /*end of for*/
    }  /*end of if*/

    /* for floating point attributes, sum or average the values and 
     * write them to the file */

    /* allocate space for reading attribute data */
    intAttData = malloc(bdesc_in.nrows * bdesc_in.ncols * sizeof(int));
    fltAttData = malloc(bdesc_in.nrows * bdesc_in.ncols * sizeof(float));
    dblAttData = malloc(bdesc_in.nrows * bdesc_in.ncols * sizeof(double));
    
    
    /*for area percent computation--such as surf zone*/
    /*get multiple type area percentage for a grid in an array areaP[numTypes][d_poly->nObjects] */
    if(!strcmp(modeFileName, "ALL_AREAPERCENT"))
    {
      /*compute area percent for each type of a attribute in a grid cell*/
      tmp_id = 0;
      if(typeAreaPercent(poly, &areaP, &gridArea, tmp_id, numTypes, &typeList))
      {
         return 1;
      }
      wPolyNumAttr = numTypes;  /*reset number of weight variables*/
      strcpy(typeNameHead,w_poly->attr_hdr->attr_desc[0]->name);
      strcat(typeNameHead,"_");
    }
    
    /* for each attribute */   
    for(attr_id = 0; attr_id < wPolyNumAttr; attr_id++)
    {
	if(!strcmp(modeFileName, "ALL_AREAPERCENT"))
        {		
  	    strcpy(typeName,typeNameHead);
	    strcat(typeName,typeList[attr_id]);
	    attrname = (char *) strdup(typeName);
	    attrtype = FTDouble;
           
            sprintf(mesg,"Name of output variable for a type %d = %s gridArea=%lf\n", attr_id,attrname,gridArea);
            MESG(mesg);
           
            /*for surf zone area percent computation*/
            if(getEnvtValue(ENVT_ALLOC_ATTR_TYPE, attrType) && strcmp(attrType,"SURF_ZONE")==0)
            {
               if(strcmp(typeList[attr_id],"2")==0)
               {
                  /*it is land, but will be converted to open ocean area*/
                  attrname = (char *) strdup("OPEN");
               }
               else if (strcmp(typeList[attr_id],"3")==0)
               {
                  attrname = (char *) strdup("SURF");
               }
            }
	}
	else
	{
          attrname = w_poly->attr_hdr->attr_desc[attr_id]->name;
          attrtype = w_poly->attr_hdr->attr_desc[attr_id]->type;
	}
        
	
        /* skip attributes of string type since the I/O API can't handle them -
           already warned user above */
        if(attrtype == FTString) continue;
        
        /* check for attribute in list of I/O API variables */
        found = 0;
        for(varnum = 0; varnum < nInputVars; ++varnum)
        {
            if(!strcmp(varlist[varnum], attrname))
            {
                found = 1;
                break;
            }
        }

        mode = getMode(attrname);

        /* loop through layers and time steps if input is I/O API file */
        jdate = bdesc.sdate;
        jtime = bdesc.stime;
        nlays = bdesc.nlays;
        mxrec = 1;
        if(input_ioapi) mxrec = bdesc_in.mxrec;
        if(input_ioapi) nrec_in = bdesc_in.nrows * bdesc_in.ncols;

        /* if input is I/O API file, then we have only allocated space for
           one attribute's values - each allocation routine only cares about
           the attribute's type so reset as needed */
        tmp_id = attr_id;
        if(input_ioapi) 
        {
            tmp_id = 0;
            w_poly->attr_hdr->attr_desc[tmp_id]->type =
                w_poly->attr_hdr->attr_desc[attr_id]->type;
        }

        for(tstep = 0; tstep < mxrec; ++tstep)
        {
            for(layer = 1; layer <= nlays; ++layer)
            {
                /* read data for multiple layers and time steps */
                if(found)
                {
                    switch (bdesc_in.vtype[varnum])
                    {
                    case M3INT:
                        if(!read3c(ENVT_INPUT_FILE_NAME, attrname, layer,
                                   jdate, jtime, intAttData))
                        {
                            sprintf(mesg, "Could not read variable %s from file %s",
                                    attrname, ENVT_INPUT_FILE_NAME);
                            ERROR(prog_name, mesg, 2);
                        }
                        
                        for(recno = 0; recno < nrec_in; ++recno)
                        {
                            w_poly->attr_val[recno][tmp_id].ival = 
                                intAttData[recno];
                        }
                        break;
                    case M3REAL:
                        if(!read3c(ENVT_INPUT_FILE_NAME, attrname, layer,
                                   jdate, jtime, fltAttData))
                        {
                            sprintf(mesg, "Could not read variable %s from file %s",
                                    attrname, ENVT_INPUT_FILE_NAME);
                            ERROR(prog_name, mesg, 2);
                        }
                        
                        for(recno = 0; recno < nrec_in; ++recno)
                        {
                            w_poly->attr_val[recno][tmp_id].val = 
                                (double) fltAttData[recno];
                        }
                        break;
                    case M3DBLE:
                        if(!read3c(ENVT_INPUT_FILE_NAME, attrname, layer,
                                   jdate, jtime, dblAttData))
                        {
                            sprintf(mesg, "Could not read variable %s from file %s",
                                    attrname, ENVT_INPUT_FILE_NAME);
                            ERROR(prog_name, mesg, 2);
                        }
                        
                        for(recno = 0; recno < nrec_in; ++recno)
                        {
                            w_poly->attr_val[recno][tmp_id].val = 
                                dblAttData[recno];
                        }
                        break;
                    default:
                        sprintf(mesg, "Unknown variable type %s in I/O API file", 
                                bdesc_in.vtype[varnum]);
                        ERROR(prog_name, mesg, 2);
                    }
                }

                if(mode == NotFound)
                {
                    sprintf(mesg, "Error retrieving allocation mode for attribute %s",
                            attrname);
                    ERROR(prog_name, mesg, 1);
        
                }
                else if(mode == Aggregate)
                {
                    if(sum1Poly(poly, &sum, &nrec_out, tmp_id, use_weight_val))
                    {
                        return 1;
                    }
                }
                else if(mode == Average)
                {
                    if(avg1Poly(poly, &sum, &nrec_out, tmp_id, use_weight_val))
                    {
                        return 1;
                    }
                }
		else if (mode == AreaPercent)
		{
		   /*get area percent for one type variable -- type attr_id*/
                   sum = (double *) malloc(nrec_out * sizeof(double));
                   /*for surf zone area percent computation*/
                   if(getEnvtValue(ENVT_ALLOC_ATTR_TYPE, attrType) && strcmp(attrType,"SURF_ZONE")==0)
                   {
                      if(strcmp(typeList[attr_id],"2")==0)
                      {
                         for (i=0;i<nrec_out;i++)
                         {
                           sum[i] = 1.0 - areaP[0][i] - areaP[1][i];
                           /*sprintf(mesg,"area percent type=%d  ID=%d   Area= %lf", attr_id,i,sum[i]);
                           MESG(mesg);*/
                         }
                      }
                      else if (strcmp(typeList[attr_id],"3")==0)
                      {
                         for (i=0;i<nrec_out;i++)
                         {    
                           sum[i]=areaP[attr_id][i];
                           /*sprintf(mesg,"area percent type=%d  ID=%d   Area= %lf", attr_id,i,sum[i]);
                           MESG(mesg);*/
                         }
                      }
                   }
                   else
                   {
                       for (i=0;i<nrec_out;i++)
                       {
                         sum[i]=areaP[attr_id][i];  
                         /*sprintf(mesg,"area percent type=%d  ID=%d   Area= %lf", attr_id,i,sum[i]);
                         MESG(mesg);*/
                       }
                   }
		}
                else if(mode == DiscreteOverlap && !overlapsCalculated)
                {
                    /* only need to do this once 
                       it won't change from attribute to attribute 
                    */
                    if(discreteOverlap(poly, &max, &nrec_out, tmp_id)) 
                    {
                        return 1;
                    }
                    overlapsCalculated = 1;            
                }
                //else if(mode == DiscreteCentroid && !centroidsCalculated)
                else if(mode == DiscreteCentroid )
                {
                    /* only need to do this once, also */
                    if(discreteCentroid(poly, &centroid, &nrec_out, tmp_id)) 
                    {
                        return 1;
                    }
                    centroidsCalculated = 1;
        
                }
                else 
                {
                    /* mode not found  */
                    sprintf(mesg, "Allocation mode not supported");
                    ERROR(prog_name, mesg, 1);
                }
                
                /* store data for this layer */
                tmpIntData = intAllData + (layer-1) * nrec_out;
                tmpFltData = fltAllData + (layer-1) * nrec_out;
                
                if(mode == Aggregate || mode == Average || mode == AreaPercent)
                {
                    for(recno = 0; recno < nrec_out; ++recno)
                    {
                        tmpFltData[recno] = (float) sum[recno];
                    }
                
                    free(sum);
                }
                else if(mode == DiscreteOverlap)
                {
                    if(attrtype == FTInteger)
                    {
                        for(recno = 0; recno < nrec_out; ++recno)
                        {
                            if(max[recno] > -1)
                            {
                                tmpIntData[recno] = w_poly->attr_val[max[recno]][tmp_id].ival;
                            }
                            else
                            {
                                tmpIntData[recno] = 0;
                            }
                        }
                    }
                    else if(attrtype == FTDouble)
                    {
                        for(recno = 0; recno < nrec_out; ++recno)
                        {
                            if(max[recno] > -1)
                            {
                                tmpFltData[recno] = (float) w_poly->attr_val[max[recno]][tmp_id].val;
                            }
                            else
                            {
                                tmpFltData[recno] = 0;
                            }
                        }
                    }
                }
                else if(mode == DiscreteCentroid)
                {
                    if(attrtype == FTInteger)
                    {
                        for(recno = 0; recno < nrec_out; ++recno)
                        {
                            if(centroid[recno] > -1)
                            {
                                tmpIntData[recno] = w_poly->attr_val[centroid[recno]][tmp_id].ival;
                            }
                            else
                            {
                                tmpIntData[recno] = 0;
                            }
                        }
                    }
                    else if (attrtype == FTDouble)
                    {
                        for(recno = 0; recno < nrec_out; ++recno)
                        {
                            if(centroid[recno] > -1)
                            {
                                tmpFltData[recno] = (float) w_poly->attr_val[centroid[recno]][tmp_id].val;
                            }
                            else
                            {
                                tmpFltData[recno] = 0;
                            }
                        }
                    }
                }
                
            }  /* end loop over layers */
        
            /* write the resulting value for the attribute */
            
            /* check the type of the attribute */
            if((mode == Aggregate || mode == Average || mode == AreaPercent) || attrtype == FTDouble)
            {
                if(!write3c(ename, attrname, jdate, jtime, fltAllData))
                {
                    sprintf(mesg, "Unable to write attribute %s to I/O API file %s",
                            attrname, name);
                    ERROR(prog_name, mesg, 2);
                }
            }
            else if(attrtype == FTInteger)
            {
                if(!write3c(ename, attrname, jdate, jtime, intAllData))
                {
                    sprintf(mesg, "Unable to write attribute %s to I/O API file %s",
                            attrname, name);
                    ERROR(prog_name, mesg, 2);
                }
            }
            
            nextimec(&jdate, &jtime, bdesc.tstep);
            
        }  /* end loop over time steps */
    }

    if(max != NULL)
    {
         free(max);
    }

    if(centroid != NULL)
    {
        free(centroid);
    }

    if(!close3c(ename))
    {
        sprintf(mesg, "Unable to close I/O API file %s", name);
        ERROR(prog_name, mesg, 2);
    }
    
    free(intAttData);
    free(fltAttData);
    free(dblAttData);
    
    free(intAllData);
    free(fltAllData);

    /*free memory used for area percent computation for a variable type*/
    if(getEnvtValue(ENVT_ALLOC_ATTR_TYPE, attrType) && strcmp(attrType,"SURF_ZONE")==0)
    { 
      for(i=0;i<numTypes;i++)
      {
        free(areaP[i]);
      }
      free (areaP);
    }

    return 0;
}

#endif
