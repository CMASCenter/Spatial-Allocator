#include "mfhdf.h"

#define FILE_NAME     "SDS.hdf"

int main()
{
   /************************* Variable declaration **************************/

   int32 sd_id, sds_id, sds_index;
   intn  status;
   int32 rank, data_type, dim_sizes[H4_MAX_VAR_DIMS];
   int32 n_datasets, n_file_attr, n_attrs; 
   char  sds_name[H4_MAX_NC_NAME];

   /********************* End of variable declaration ***********************/

   /*
   * Open the file and initialize the SD interface.
   */
   sd_id = SDstart(FILE_NAME, DFACC_READ);

   /*
   * Obtain information about the file.
   */ 
   status = SDfileinfo(sd_id, &n_datasets, &n_file_attr);

   /* Get information about each SDS in the file.
   *  Check whether it is a coordinate variable, then display retrieved 
   *  information.
   *  Output displayed:
   *
   *            SDS array with the name SDStemplate
   *            Coordinate variable with the name Y_Axis
   *            Coordinate variable with the name X_Axis
   *
   */
   for (sds_index=0; sds_index< n_datasets; sds_index++) 
   {
       sds_id = SDselect (sd_id, sds_index);
       status = SDgetinfo(sds_id, sds_name, &rank, dim_sizes, &data_type, &n_attrs);
       if (SDiscoordvar(sds_id)) 
          printf(" Coordinate variable with the name %s\n", sds_name);
       else
          printf(" SDS array with the name %s\n", sds_name);

   /*
   * Terminate access to the selected data set.
   */
   status = SDendaccess(sds_id);

   }

   /*
   * Terminate access to the SD interface and close the file.
   */
   status = SDend(sd_id);

   return 0;
}
