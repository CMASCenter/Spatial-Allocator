#include "mfhdf.h"

#define FILE_NAME     "SDS.hdf" 
#define SDS_NAME      "SDStemplate" 
#define X_LENGTH      5
#define Y_LENGTH      16
#define RANK          2  /* Number of dimensions of the SDS */

int main()
{
   /************************* Variable declaration **************************/

   int32 sd_id, sds_id;     /* SD interface and data set identifiers */
   int32 dim_sizes[2];      /* sizes of the SDS dimensions */
   intn  status;            /* status returned by some routines; has value
                               SUCCEED or FAIL */

   /********************* End of variable declaration ***********************/

   /*
   * Create the file and initialize the SD interface.
   */
   sd_id = SDstart (FILE_NAME, DFACC_CREATE);

   /*
   * Define the dimensions of the array to be created.
   */
   dim_sizes[0] = Y_LENGTH;
   dim_sizes[1] = X_LENGTH;

   /*
   * Create the data set with the name defined in SDS_NAME. Note that 
   * DFNT_INT32 indicates that the SDS data is of type int32. Refer to
   * Table 2F, "Standard HDF Data Types and Flags," for definitions of
   * other types. 
   */
   sds_id = SDcreate (sd_id, SDS_NAME, DFNT_INT32, RANK, dim_sizes);

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
