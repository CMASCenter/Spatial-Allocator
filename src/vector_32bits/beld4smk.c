/* This program transforms BELD4 land use data to the modeling grid of interest. */

#ifdef USE_IOAPI

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>

#include "iodecl3.h"
#include "io.h"
#include "mims_evs.h"
#include "shapefil.h"
#include "mims_spatl.h"

#define MAX_TILES 24    /* number of tiles of data */
#define MAX_VARS  276   /* number of variables in data */


/* environment variables */
const char IOAPI_FILE_OUT[16] = "IOAPI_FILE_OUT";
const char IOAPI_FILE_IN[16] = "IOAPI_FILE_IN";

/* program name */
char *prog_name    ="beld4smk";
char *prog_version = "Spatial Allocator BELD4 to SMOKE convertor Version 3.6 - 09/21/2015\n";

/* functions */
int buildFileName(char *dest, const char *dir, 
                  const char *name, int tilenum, int isIntermediate);
int getDescription(const char *file_name, IOAPI_Bdesc3 *bdesc,
                   IOAPI_Cdesc3 *cdesc);
int file_exist (char *filename);

int main(int argc, char *argv[])
{
  /* environment variable values */
  char debug[10];            /* print debug messages flag */
  char gridname[NAMLEN3+1];  /* output grid name */
  char griddesc[256];        /* grid description file */
  char input_dir[128];       /* input data directory */
  char tmp_dir[128];         /* tmp directory for intermediate files */
  char input_pre[128];       /* input prefix */
  char output_pre[128];      /* output prefix */
  char out_type[15];         /*output file type: RegularGrid, EGrid, or Polygon*/
  char out_ellip[256];       /*output ellipsoid*/
  char allocatorEXE[256];        /* allocator exe program */

  /* allocatable arrays */
  float *inData;             /* values read in from I/O API files */
  float *outData;            /* summed output values */
  float *qaData;             /* summed data across variables for QA */

  /* fixed size arrays */
  char var_names[MAX_VARS][NAMLEN3+1];    /* variable names */
  int  var_files[MAX_VARS][MAX_TILES]; /* variable-to-file mapping array */
  int  tilelist[MAX_TILES]=0;               /* list of tiles to process */
  int i;

  /* other local variables */
  int  tileidx, tilenum;
  int  varidx, varnum;
  int  recno;
  int  row, col;
  int  exisnum=0;              /* a file number processed */
  int  num_tiles;            /* number of tiles to process */
  int  files_processed;      /* number of files processed */

  float pctdiff;             /* percent difference in summed values */

  IOAPI_Bdesc3 bdesc;        /* I/O API file description - non-character data */
  IOAPI_Bdesc3 bdesc_grid;
  IOAPI_Cdesc3 cdesc;        /* I/O API file description - character data */
  IOAPI_Cdesc3 cdesc_grid;
  
  FILE *fileptr;             /* file pointer */

  char file_name[256];       /* generated complete file name */
  char varname[NAMLEN3+1];   /* variable name */
  char mesg[256];            /* message buffer */
  char debugOutput[10];

  extern int debug_output;
/* ========================================================================== */

//prog_name = argv[0];
  if(getEnvtValue(ENVT_DEBUG_OUTPUT, debugOutput))
  {
        if(!strcmp(debugOutput, "Y"))
        {
            debug_output = 1;
        }
        else if(!strcmp(debugOutput, "N"))
        {
            debug_output = 0;
        }
        else
        {
            sprintf(mesg, "%s can only be set to 'Y' or 'N'",
                    ENVT_DEBUG_OUTPUT);
            ERROR(prog_name, mesg, 2);
        }
  }
  else
  {
        sprintf(mesg, "Please specify a %s of 'Y' or 'N' in your script",
                ENVT_DEBUG_OUTPUT);
        ERROR(prog_name, mesg, 2);
  }

  MESG(prog_version);

  if (!getEnvtValue("ALLOCATOR_EXE", allocatorEXE) )
  {
     sprintf(mesg, "%s environment variable not set", "ALLOCATOR_EXE");
     ERROR(prog_name, mesg, 1);
  }


  if(!getEnvtValue("OUTPUT_GRID_NAME", gridname))
  {
    sprintf(mesg, "%s environment variable not set", "OUTPUT_GRID_NAME");
    ERROR(prog_name, mesg, 1);
  }
  
  if(!getEnvtValue("OUTPUT_FILE_TYPE", out_type))
  {
    sprintf(mesg, "%s environment variable not set", "OUTPUT_FILE_TYPE");
    ERROR(prog_name, mesg, 1);
  }

   if(!getEnvtValue("OUTPUT_FILE_ELLIPSOID", out_ellip))
  {
    sprintf(mesg, "%s environment variable not set", "OUTPUT_FILE_ELLIPSOID");
    ERROR(prog_name, mesg, 1);
  }

  if(!getEnvtValue("GRIDDESC", griddesc))
  {
    sprintf(mesg, "%s environment variable not set", "GRIDDESC");
    ERROR(prog_name, mesg, 1);
  }

  if(!getEnvtValue("INPUT_DATA_DIR", input_dir))
  {
    sprintf(mesg, "%s environment variable not set", "INPUT_DATA_DIR");
    ERROR(prog_name, mesg, 1);
  }

  if(!getEnvtValue("TMP_DATA_DIR", tmp_dir))
  {
    sprintf(mesg, "%s environment variable not set", "TMP_DATA_DIR");
    ERROR(prog_name, mesg, 1);
  }

  if(!getEnvtValue("INPUT_FILE_PREFIX", input_pre))
  {
    sprintf(mesg, "%s environment variable not set", "INPUT_FILE_PREFIX");
    ERROR(prog_name, mesg, 1);
  }
  
  if(!getEnvtValue("OUTPUT_FILE_PREFIX", output_pre))
  {
    sprintf(mesg, "%s environment variable not set", "OUTPUT_FILE_PREFIX");
    ERROR(prog_name, mesg, 1);
  }

  /* use overlay mode of spatial allocator to determine which tiles to process */
  if(debug_output)
  {
    setenv("DEBUG_OUTPUT", "Y", 1);
  }
  else
  {
    setenv("DEBUG_OUTPUT", "N", 1);
  }
  
  setenv("MIMS_PROCESSING", "OVERLAY", 1);
  
  setenv("OVERLAY_TYPE", out_type, 1);
  setenv("OVERLAY_SHAPE", gridname, 1);
  setenv("OVERLAY_MAP_PRJN", gridname, 1);
  setenv("OVERLAY_ELLIPSOID", out_ellip, 1);
  setenv("GRIDDESC", griddesc, 1);
  
  setenv("OVERLAY_ATTRS", "TILES24_ID", 1);
  setenv("OVERLAY_OUT_TYPE", "DelimitedFile", 1);
  sprintf(file_name, "%stiles.txt", tmp_dir);
  setenv("OVERLAY_OUT_NAME", file_name, 1);
  setenv("OVERLAY_OUT_DELIM", " ", 1);
  setenv("WRITE_HEADER", "N", 1);
  
  setenv("INPUT_FILE_TYPE", "ShapeFile", 1);
  sprintf(file_name, "%stiles24", input_dir);
  setenv("INPUT_FILE_NAME", file_name, 1);
  setenv("INPUT_FILE_MAP_PRJN", 
         "+proj=lcc,+lon_0=-90.0,+lat_1=30.0,+lat_2=60.0,+lat_0=40.0", 1);
  setenv("INPUT_FILE_ELLIPSOID", "+a=6370000.0,+b=6370000.0", 1);
 
  if(system( allocatorEXE ))
  {
    sprintf(mesg, "Problem running program: %s", allocatorEXE);
    ERROR(prog_name, mesg, 2);
  }
  
  /* read tile list output from spatial allocator */
  sprintf(file_name, "%stiles.txt", tmp_dir);
  if((fileptr = fopen(file_name, "r")) == NULL)
  {
    sprintf(mesg, "Could not open intermediate file %s \n", file_name);
    ERROR(prog_name, mesg, 2);
  }
  
  while(fgets(mesg, sizeof(mesg), fileptr) != NULL)
  {
     tilenum = atoi(mesg);
     tilelist[tilenum-1] = tilenum;
  }
  
  /*get rid of elements with 0 title number*/
  num_tiles=0;
  for (i=0;i<MAX_TILES;i++)
  {
    if (tilelist[i] != 0)
    {
     tilelist[num_tiles] = tilelist[i];
     num_tiles++;
    } 
  }

  sprintf(mesg,"num_tiles = %d\n", num_tiles);
  MESG(mesg); 

  /* use spatial allocator to process each tile */
  if(debug_output)
  {
    setenv("DEBUG_OUTPUT", "Y", 1);
  }
  else
  {
    setenv("DEBUG_OUTPUT", "N", 1);
  }
  
  setenv("MIMS_PROCESSING", "ALLOCATE", 1);
  
  setenv("GRIDDESC", griddesc, 1);
  files_processed = 0;
  
  for(tileidx = 0; tileidx < num_tiles; ++tileidx)
  {
      tilenum = tilelist[tileidx];
      buildFileName(file_name, input_dir, input_pre, tilenum, 0);

      /* check if file exists */
      if(file_exist(file_name) )
      {
        files_processed++;
   
        if ( exisnum == 0 ) 
        {
          exisnum = tilenum;
        }

        setenv("INPUT_FILE_NAME", file_name, 1);
        setenv("INPUT_FILE_TYPE", "IoapiFile", 1);
    
        setenv("ALLOCATE_ATTRS", "ALL", 1);
        setenv("ALLOC_MODE_FILE", "ALL_AVERAGE", 1);
    
        buildFileName(file_name, tmp_dir, input_pre, tilenum, 1);
        
        /* delete intermediate file if it exists */
        if(file_exist(file_name) )
        {
          if(remove(file_name) != 0)
          {
            sprintf(mesg, "Could not delete existing intermediate file %s", 
                    file_name);
            ERROR(prog_name, mesg, 2);
          }
        }
        
        setenv("OUTPUT_FILE_NAME", file_name, 1);
        setenv("OUTPUT_FILE_TYPE", "IoapiFile", 1);
        setenv("OUTPUT_GRID_NAME", gridname, 1);
        setenv("OUTPUT_FILE_ELLIPSOID", out_ellip, 1);

        printf("Processing tile %d ...\n", tilenum);
        fflush(stdout);

        if(system( allocatorEXE ))
        {
          sprintf(mesg, "Problem running program: %s  \n", allocatorEXE);
          ERROR(prog_name, mesg, 2);
        }
        printf("finish %s \n", file_name);
      }
      else 
      {
         sprintf(mesg, "File %s does not exist\n",file_name);
         WARN(mesg);
      }
  }  /* end loop over tiles */

  /* check that at least one file was processed */
  if(files_processed == 0)
  {
    sprintf(mesg, "Could not process any tiles files. %s %s",
            "Please check your INPUT_DATA_DIR directory", input_dir);
    ERROR(prog_name, mesg, 2);
  }
  
  /* create output file headers and store master list of variable names */
  
  /* get grid description from just created intermediate file */
  buildFileName(file_name, tmp_dir, input_pre, exisnum, 1);
  getDescription(file_name, &bdesc_grid, &cdesc_grid);
  
  /* get variable names from master variable files */
  buildFileName(file_name, input_dir, input_pre, exisnum, 0);
  getDescription(file_name, &bdesc, &cdesc);

  /* store list of variable names */
  for(varnum = 0; varnum < bdesc.nvars; ++varnum)
  {
      strNullTerminate(var_names[varnum], cdesc.vname[varnum], NAMLEN3);
  }

  /* reset grid description */
  bdesc.p_alp = bdesc_grid.p_alp;
  bdesc.p_bet = bdesc_grid.p_bet;
  bdesc.p_gam = bdesc_grid.p_gam;
  bdesc.xcent = bdesc_grid.xcent;
  bdesc.ycent = bdesc_grid.ycent;
  bdesc.xorig = bdesc_grid.xorig;
  bdesc.yorig = bdesc_grid.yorig;
  bdesc.xcell = bdesc_grid.xcell;
  bdesc.ycell = bdesc_grid.ycell;
    
  bdesc.gdtyp = bdesc_grid.gdtyp;
  memcpy(cdesc.gdnam, cdesc_grid.gdnam, sizeof(cdesc.gdnam));
  bdesc.ncols = bdesc_grid.ncols;
  bdesc.nrows = bdesc_grid.nrows;

  /* open output file */
  sprintf(file_name, "%s.ncf", output_pre);
  setenv(IOAPI_FILE_OUT, file_name, 1);
  
  if( file_exist(file_name) )
  {
    if(remove(file_name) != 0)
    {
      sprintf(mesg, "Could not delete existing output file %s",
                file_name);
      WARN(mesg);
    }
  }
    
  if(!open3c(IOAPI_FILE_OUT, &bdesc, &cdesc, FSNEW3, prog_name))
  {
      sprintf(mesg, "Could not open I/O API file %s", file_name);
      ERROR(prog_name, mesg, 2);
  }

  /* allocate space to store input and output data */
  inData = malloc(bdesc.nrows * bdesc.ncols * sizeof(float));
  outData = malloc(bdesc.nrows * bdesc.ncols * sizeof(float));
  qaData = malloc(bdesc.nrows * bdesc.ncols * sizeof(float));
  
  /* initialize QA data array */
  memset(qaData, 0, bdesc.nrows * bdesc.ncols * sizeof(float));
  
  /* loop through master list of variables */
  for(varnum = 0; varnum < MAX_VARS; ++varnum)
  {
    
    /* initialize output data array */
    memset(outData, 0, bdesc.nrows * bdesc.ncols * sizeof(float));
    
    /* loop through tiles */
    for(tileidx = 0; tileidx < num_tiles; ++tileidx)
    {
      tilenum = tilelist[tileidx];
      
      buildFileName(file_name, tmp_dir, input_pre, tilenum, 1);
      setenv(IOAPI_FILE_IN, file_name, 1);
          
      if(file_exist(file_name) )
      {
        if(!open3c(IOAPI_FILE_IN, &bdesc, &cdesc, FSREAD3, prog_name))
        {
          sprintf(mesg, "Could not open I/O API file %s", file_name);
          ERROR(prog_name, mesg, 2);
        }
      
        strNullTerminate(varname, var_names[varnum], NAMLEN3);   
      
        if(!read3c(IOAPI_FILE_IN, varname, 1, 0, 0, inData))
        {
          sprintf(mesg, "Could not read variable %s from I/O API file %s",
                varname, file_name);
          ERROR(prog_name, mesg, 2);
        }
          
        if(!close3c(IOAPI_FILE_IN))
        {
          sprintf(mesg, "Could not close I/O API file %s", file_name);
          ERROR(prog_name, mesg, 2);
        }
          
        /* sum data from each tile into output array */
        for(recno = 0; recno < bdesc.nrows * bdesc.ncols; ++recno)
        {
          outData[recno] = outData[recno] + inData[recno];
          qaData[recno] = qaData[recno] + inData[recno];
        }
          
        /*break;*/
      }
    }  /* end loop over vars tile files */
    

    /* write to output file */      
  
    if(!write3c(IOAPI_FILE_OUT, varname, 0, 0, outData))
    {
      sprintf(mesg, "Could not write variable %s to I/O API file %s",
              varname, file_name);
      ERROR(prog_name, mesg, 2);
    }
    
  }  /* end loop over variables */
  
  /* check that all variables sum to 100% */
  for(row = 0; row < bdesc.nrows; ++row)
  {
    for(col = 0; col < bdesc.ncols; ++col)
    {
      pctdiff = qaData[(row * bdesc.ncols) + col] - 100.;
           
      if(abs(pctdiff) > 0.01)
      {
        printf("WARNING: Summed land use differs from 100%% by %.2f%% for row %d, column %d\n",
               pctdiff, row+1, col+1);
      }
    }
  }
  
  
  if(!close3c(IOAPI_FILE_OUT))
  {
    sprintf(mesg, "Could not close I/O API file %s", file_name);
    ERROR(prog_name, mesg, 2);
  }
  
  return 0;
}

/* ========================================================================== */

int buildFileName(char *dest, const char *dir, 
                  const char *name, int tilenum, int isIntermediate)
{
  return(sprintf(dest, 
                 isIntermediate ? "%s%s.tile%d.out.ncf"
                                : "%s%s.tile%d.ncf", 
                 dir, name, tilenum));
}

/* ========================================================================== */

int getDescription(const char *file_name, IOAPI_Bdesc3 *bdesc,
                   IOAPI_Cdesc3 *cdesc)
{
  char mesg[256];

  setenv(IOAPI_FILE_IN, file_name, 1);
  
  if(!open3c(IOAPI_FILE_IN, bdesc, cdesc, FSREAD3, prog_name))
  {
    sprintf(mesg, "Could not open I/O API file %s", file_name);
    ERROR(prog_name, mesg, 2);
  }
  
  if(!desc3c(IOAPI_FILE_IN, bdesc, cdesc))
  {
    sprintf(mesg, "Could not get description of I/O API file %s", file_name);
    ERROR(prog_name, mesg, 2);
  }
  
  if(!close3c(IOAPI_FILE_IN))
  {
    sprintf(mesg, "Could not close I/O API file %s", file_name);
    ERROR(prog_name, mesg, 2);
  }
  
  return 0;
}

int file_exist (char *filename)
{
  return (access( filename, F_OK ) != -1) ;
}
#else

int main(int argc, char *argv[])
{
    printf("I/O API support is required for beld4smk.exe\n");
    return 0;
}

#endif
