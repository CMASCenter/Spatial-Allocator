#include "mfhdf.h"

#define FILE_NAME      "SDS.hdf"
#define FILE_ATTR_NAME "File_contents"
#define SDS_ATTR_NAME  "Valid_range"
#define DIM_ATTR_NAME  "Dim_metric"

int main()
{
   /************************* Variable declaration **************************/

   int32   sd_id, sds_id, sds_index;
   intn    status;
   int32   dim_id, dim_index;
   int32   n_values;                /* number of values of the file, SDS or
                                       dimension attribute         */
   char8   file_values[] = "Storm_track_data"; 
                                   /* values of the file attribute */
   float32 sds_values[2] = {2., 10.};
                                   /* values of the SDS attribute  */
   char8   dim_values[]  = "Seconds"; 
                                  /* values of the dimension attribute */

   /********************* End of variable declaration ***********************/

   /*
   * Open the file and initialize the SD interface.
   */
   sd_id = SDstart (FILE_NAME, DFACC_WRITE);

   /*
   * Set an attribute that describes the file contents.
   */
   n_values = 16;
   status = SDsetattr (sd_id, FILE_ATTR_NAME, DFNT_CHAR, n_values, 
                       (VOIDP)file_values);

   /*
   * Select the first data set.
   */
   sds_index = 0;
   sds_id = SDselect (sd_id, sds_index);

   /* 
   * Assign attribute to the first SDS. Note that attribute values
   * may have different data type than SDS data.
   */
   n_values  = 2;
   status = SDsetattr (sds_id, SDS_ATTR_NAME, DFNT_FLOAT32, n_values, 
                       (VOIDP)sds_values);

   /*
   * Get the the second dimension identifier of the SDS.
   */
   dim_index = 1;
   dim_id = SDgetdimid (sds_id, dim_index);

   /*
   * Set an attribute of the dimension that specifies the dimension metric.
   */
   n_values = 7;
   status = SDsetattr (dim_id, DIM_ATTR_NAME, DFNT_CHAR, n_values, 
                       (VOIDP)dim_values);

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
