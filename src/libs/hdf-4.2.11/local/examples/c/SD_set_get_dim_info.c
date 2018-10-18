#include "mfhdf.h"


#define FILE_NAME     "SDS.hdf"
#define SDS_NAME      "SDStemplate"
#define DIM_NAME_X     "X_Axis"
#define DIM_NAME_Y     "Y_Axis"
#define NAME_LENGTH   6
#define X_LENGTH      5
#define Y_LENGTH      16
#define RANK          2

int main()
{
   /************************* Variable declaration **************************/

   int32   sd_id, sds_id, sds_index;
   intn    status;
   int32   dim_index, dim_id;
   int32   n_values, data_type, n_attrs;
   int16   data_X[X_LENGTH];    /* X dimension dimension scale */
   int16   data_X_out[X_LENGTH];
   float64 data_Y[Y_LENGTH];  /* Y dimension dimension scale */
   float64 data_Y_out[Y_LENGTH]; 
   char    dim_name[NAME_LENGTH+1];
   int     i, j, nrow;

   /********************* End of variable declaration ***********************/

   /*
   * Initialize dimension scales.
   */
   for (i=0; i < X_LENGTH; i++) data_X[i] = i;
   for (i=0; i < Y_LENGTH; i++) data_Y[i] = 0.1 * i;
      
   /* 
   * Open the file and initialize SD interface.
   */
   sd_id = SDstart (FILE_NAME, DFACC_WRITE);

   /*
   * Get the index of the data set specified in SDS_NAME.
   */
   sds_index = SDnametoindex (sd_id, SDS_NAME);

   /*
   * Select the data set corresponding to the returned index.
   */
   sds_id = SDselect (sd_id, sds_index);

   /* For each dimension of the data set specified in SDS_NAME,
   *  get its dimension identifier and set dimension name
   *  and dimension scale. Note that data type of dimension scale 
   *  can be different between dimensions and can be different from 
   *  SDS data type.
   */
   for (dim_index = 0; dim_index < RANK; dim_index++) 
   {
       /* 
       * Select the dimension at position dim_index.
       */
       dim_id = SDgetdimid (sds_id, dim_index);

       /* 
       * Assign name and dimension scale to selected dimension.
       */
       switch (dim_index)
       {
  case 0:  status = SDsetdimname (dim_id, DIM_NAME_Y);
                n_values = Y_LENGTH;
                status = SDsetdimscale (dim_id,n_values,DFNT_FLOAT64, \
                                       (VOIDP)data_Y);  
    break;
  case 1:  status = SDsetdimname (dim_id, DIM_NAME_X);
                n_values = X_LENGTH; 
                status = SDsetdimscale (dim_id,n_values,DFNT_INT16, \
                                       (VOIDP)data_X);  
    break;
  default: break;
       }

       /*
       * Get and display info about the dimension and its scale values.
       * The following information is displayed:
       *                         
       *         Information about 1 dimension:
       *         dimension name is Y_Axis
       *         number of scale values is 16
       *         dimension scale data type is float64
       *         number of dimension attributes is 0
       *
       *         Scale values are :
       *               0.000    0.100    0.200    0.300  
       *               0.400    0.500    0.600    0.700  
       *               0.800    0.900    1.000    1.100  
       *               1.200    1.300    1.400    1.500  
       *
       *         Information about 2 dimension:
       *         dimension name is X_Axis
       *         number of scale values is 5
       *         dimension scale data type is int16
       *         number of dimension attributes is 0
       *
       *         Scale values are :
       *               0  1  2  3  4
       */

       status = SDdiminfo (dim_id, dim_name, &n_values, &data_type, &n_attrs);
       printf ("Information about %d dimension:\n", dim_index+1);
       printf ("dimension name is %s\n", dim_name);
       printf ("number of scale values is %d\n", n_values);
       if( data_type == DFNT_FLOAT64)
       printf ("dimension scale data type is float64\n");
       if( data_type == DFNT_INT16)
       printf ("dimension scale data type is int16\n");
       printf ("number of dimension attributes is %d\n", n_attrs);
       printf ("\n");
       printf ("Scale values are :\n");
       switch (dim_index) 
       {
         case 0:  status = SDgetdimscale (dim_id, (VOIDP)data_Y_out);
                  nrow = 4;
                  for (i=0; i<n_values/nrow; i++ )
                  {
                      for (j=0; j<nrow; j++)
                          printf ("  %-6.3f", data_Y_out[i*nrow + j]);
                          printf ("\n");
                  }
                  break; 
         case 1:  status = SDgetdimscale (dim_id, (VOIDP)data_X_out);
                  for (i=0; i<n_values; i++) printf ("  %d", data_X_out[i]);
                  break; 
         default: break;
        }
        printf ("\n");
   } /*for dim_index */

   /*
   * Terminate access to the data set.
   */
   status = SDendaccess (sds_id);

   /*
   * Terminate access to the SD interface and close the file.
   */
   status = SDend (sd_id);

   return 0;
}
