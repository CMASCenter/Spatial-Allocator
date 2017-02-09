/********************************************************************************
 * AttachIoapiAttribute.c
 *
 * File contains:
 * AttachDBFAttribute
 *
 * 7/1/2005 CAS -- Copied from AttachDBFAttribute.c
 *
 ********************************************************************************/

#ifdef USE_IOAPI

#include "shapefil.h"
#include "mims_spatl.h"
#include "mims_evs.h"
#include "iodecl3.h"

/*
 * =============================================================
 */
/*
 * connect attribute values to a polygon
 */
int attachIoapiAttribute(PolyObject * poly, char *attr_env_name, 
                         char *ctgr_name, char *file_name)
{
    int i;
    DBFFieldType eType;
    int found;
    int nr, recno;
    int ival, jc;
    int attr_id;
    char *attr_name;
    int n, oldn;
    int ncnt, nmax;
    char **list, **listc;
    char mesg[256];

    extern char *prog_name;

    IOAPI_Bdesc3 bdesc;
    IOAPI_Cdesc3 cdesc;
    int nvars;
    char varlist[MXVARS3][NAMLEN3+1];
    int    *intData = NULL;
    float  *fltData = NULL;
    double *dblData = NULL;
    int output_ioapi;
                                                                                    
    /* no attributes, so just create an empty structure and return */
    /* this is used for Regular Grids in ALLOCATE mode */
    /* added 5/25/2005 BDB */
    if(attr_env_name == NULL)
    {
        MESG("No attributes to attach, creating dummy header and NULL variables list");
                                                                                    
        if(!poly->attr_hdr)
        {
            poly->attr_hdr = getNewAttrHeader();
        }
        poly->attr_hdr->num_attr = 0;
                                                                                    
        poly->attr_val = NULL;
                                                                                    
        MESG("No attributes attached\n");
        return 1;
    }

    if(!poly)
    {
        sprintf(mesg, "%s", "PolyObject struct not set");
        ERROR(prog_name, mesg, 1);
    }
                                                                                    
    if(!poly->name)
    {
        sprintf(mesg, "%s", "PolyObject name not yet specified");
        ERROR(prog_name, mesg, 1);
    }
                                                                                    
    /* split list of attributes on commas */
    if(!strlistc(attr_env_name,
                 "Comma separated list of attribute names",
                 &nmax, &ncnt, &list))
    {
        sprintf(mesg, "%s:%s", "error reading list of attribute names",
                attr_env_name);
        ERROR(prog_name, mesg, 1);
    }
                                                                                    
    n = ncnt;
    
    if(ncnt <= 0)
    {
        sprintf(mesg, "%s:%s",
                "Attribute list empty or negative count for list of attributes",
                attr_env_name);
        ERROR(prog_name, mesg, 2);
    }
    
    /* surrogate category list */
    if(ctgr_name)
    {
        if(!strlistc
           (ctgr_name, "Comma separated list of srgt category values",
            &nmax, &ncnt, &listc))
        {
            sprintf(mesg, "%s",
                    "Surrogate category list empty or negative count for list of attributes");
            ERROR(prog_name, mesg, 2);
        }
        if(n != ncnt)
        {
            sprintf(mesg, "%s",
                    "Inconsistent number of attributes and categories");
            ERROR(prog_name, mesg, 2);
        }
    }
    
    /* get description of I/O API file */
    if(!desc3c(file_name, &bdesc, &cdesc))
    {
        sprintf(mesg, "Unable to get description of input file %s", file_name);
        ERROR(prog_name, mesg, 2);
    }

    /* make sure the I/O API file contains some variables */
    if((nvars = bdesc.nvars) == 0)
    {
        sprintf(mesg, "Input I/O API file %s contains no variables", file_name);
        ERROR(prog_name, mesg, 2);
    }

    /* store variable names as null terminated strings */
    for(i = 0; i < nvars; ++i)
    {
        strNullTerminate(varlist[i], cdesc.vname[i], NAMLEN3);
    }

    /* if the output file is also I/O API, then just store list of 
       attribute names and only allocate space for one attribute's 
       values - will loop through attribute values in allocateIoapi.c */
    output_ioapi = 0;
    getEnvtValue(ENVT_OUTPUT_FILE_TYPE, mesg);
    if(strcmp(mesg, "IoapiFile") == 0) output_ioapi = 1;

    if(!output_ioapi)
    {
        /* allocate space to store data from I/O API file */
        intData = (int *)    malloc(bdesc.nrows * bdesc.ncols * sizeof(int));
        fltData = (float *)  malloc(bdesc.nrows * bdesc.ncols * sizeof(float));
        dblData = (double *) malloc(bdesc.nrows * bdesc.ncols * sizeof(double));
    }

    /* added 4/13/2005 to support "ALL" keyword for attribute selection  BDB */
    if(!strcmp(list[0], "ALL"))
    {
        MESG("All attributes selected for outupt");
                                                                                    
        list = (char **) malloc(nvars * sizeof(char *));
        ncnt = nvars;
                                                                                    
        for(i = 0; i < nvars; ++i)
        {                                                 
            list[i] = (char *) strdup(varlist[i]);
        }
    }
                                                                                                                                                                        
    if(!strcmp(list[0], "USE_FUNCTION"))
    {
        sprintf(mesg, "Weight function is not supported for I/O API files");
        ERROR(prog_name, mesg, 1);
                                                                                        
    }
                                                                                        
    MESG("Not using function for weights\n");
                                                                                    
    for(attr_id = 0; attr_id < ncnt; attr_id++)
    {
        attr_name = list[attr_id];
                                                                                    
        ival = -1;
        if(ctgr_name)
        {
            ival = atoi(listc[attr_id]);
        }
                                                                                    
        if(!strcmp(attr_name, "NONE"))
        {
            MESG("Attribute specified as NONE: assuming AREA based surrogate\n");
            if(!poly->attr_hdr)
            {
                poly->attr_hdr = getNewAttrHeader();
            }
                                                                                    
            i = addNewAttrHeader(poly->attr_hdr, attr_name, FTInvalid);
                                                                                    
            poly->attr_hdr->attr_desc[attr_id]->category = ival;
                                                                                    
            continue;
            /*go to next_attr; */
        }
        else
        {
            sprintf(mesg, "Using attribute %s", attr_name);
            MESG(mesg);
        }
        
        /* find attribute in list of variables */
        found = 0;
        for(i = 0; i < nvars; i++)
        {
            if(!strcmp(varlist[i], attr_name))
            {
                found = 1;
                break;
            }
        }
                                                                                    
        if(!found)
        {
            sprintf(mesg, "Attribute %s cannot be found in I/O API file",
                    attr_name);
            ERROR(prog_name, mesg, 2);
        }
                                                                                    
        nr = bdesc.ncols * bdesc.nrows;
                                                                                    
        if(nr != poly->nObjects)
        {
            sprintf(mesg, "%s", "attribute/polygon count mismatch");
            ERROR(prog_name, mesg, 2);
        }                                                                                        

        if(!poly->attr_hdr)
        {
            poly->attr_hdr = getNewAttrHeader();
        }
        
        switch (bdesc.vtype[i])
        {
        case M3INT:
            eType = FTInteger;
            break;
        case M3REAL:
            eType = FTDouble;
            break;
        case M3DBLE:
            eType = FTDouble;
            break;
        default:
            sprintf(mesg, "Unknown type in I/O API input file");
            ERROR(prog_name, mesg, 2);
        }
        
        oldn = addNewAttrHeader(poly->attr_hdr, attr_name, eType);
                                                                                    
        poly->attr_hdr->attr_desc[attr_id]->category = ival;
                                                                                    
        n = poly->attr_hdr->num_attr;

        /* on first attribute, allocate space for values */
        if(!poly->attr_val)
        {
            poly->attr_val =
                (AttributeValue **) malloc(nr * sizeof(AttributeValue *));
                
            if(!poly->attr_val)
            {
                sprintf(mesg, "%s", "Attribute allocation error 1");
                ERROR(prog_name, mesg, 2);
            }
            
            for(recno = 0; recno < nr; recno++)
            {
                if((poly->attr_val[recno] =
                    (AttributeValue *) malloc(n *
                                       sizeof(AttributeValue)))
                                       == NULL)
                {
                    sprintf(mesg, "%s", "Attribute allocation error 2");
                    ERROR(prog_name, mesg, 2);
                }
                
                for(jc = 0; jc < n; jc++)
                {
                    poly->attr_val[recno][jc].str = NULL;
                }
             }
         }
         else
         {
            /* if using output I/O API file, don't allocate space for all
                attribute values */
            if(!output_ioapi)
            {
                for(recno = 0; recno < nr; recno++)
                {
                    if((poly->attr_val[recno] =
                        (AttributeValue *) realloc(poly->attr_val[recno],
                                                   n *
                                                   sizeof
                                                   (AttributeValue))) == NULL)
                    {
                        sprintf(mesg, "%s", "Attribute allocation error 3");
                        ERROR(prog_name, mesg, 2);
                    }
                                                                                        
                    for(jc = oldn; jc < n; jc++)
                    {
                         poly->attr_val[recno][jc].str = NULL;
                    }
                }
            }
        }
        
        if(!output_ioapi)
        {
            /* read first layer and first time step of attribute data */
            /* TODO: add options for layers and time steps */
            switch (bdesc.vtype[i])
            {
            case M3INT:
                if(!read3c(file_name, attr_name, 1, 
                           bdesc.sdate, bdesc.stime, intData))
                {
                    sprintf(mesg, "Could not read variable %s from file %s",
                            attr_name, file_name);
                    ERROR(prog_name, mesg, 2);
                }
                
                for(recno = 0; recno < nr; recno++)
                {
                    poly->attr_val[recno][attr_id].ival = intData[recno];
                }
                break;
            case M3REAL:
                if(!read3c(file_name, attr_name, 1,
                           bdesc.sdate, bdesc.stime, fltData))
                {
                    sprintf(mesg, "Could not read variable %s from file %s",
                            attr_name, file_name);
                    ERROR(prog_name, mesg, 2);
                }
                
                for(recno = 0; recno < nr; recno++)
                {
                    poly->attr_val[recno][attr_id].val = (double) fltData[recno];
                }
                break;
            case M3DBLE:
                if(!read3c(file_name, attr_name, 1,
                           bdesc.sdate, bdesc.stime, dblData))
                {
                    sprintf(mesg, "Could not read variable %s from file %s",
                            attr_name, file_name);
                    ERROR(prog_name, mesg, 2);
                }
                
                for(recno = 0; recno < nr; recno++)
                {
                    poly->attr_val[recno][attr_id].val = dblData[recno];
                }
                break;
            default:
                sprintf(mesg, "Unknown variable type %s in I/O API file", 
                        bdesc.vtype[i]);
                ERROR(prog_name, mesg, 2);
            }
        }  /* end check for output I/O API file */

    }  /* loop over attributes */
    
    if(intData != NULL) free(intData);
    if(fltData != NULL) free(fltData);
    if(dblData != NULL) free(dblData);
    
    return 1;
}

#endif
