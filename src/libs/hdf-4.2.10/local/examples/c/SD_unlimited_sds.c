#include "mfhdf.h"

#define FILE_NAME     "SDSUNLIMITED.hdf"
#define SDS_NAME      "AppendableData"
#define X_LENGTH      10
#define Y_LENGTH      10
#define RANK          2

int main() 
{
   /************************* Variable declaration **************************/

   int32 sd_id, sds_id, sds_index;
   intn  status;
   int32 dim_sizes[2];
   int32 data[Y_LENGTH][X_LENGTH], append_data[X_LENGTH];
   int32 start[2], edges[2];
   int   i, j;

   /********************* End of variable declaration ***********************/

   /*
   * Data initialization.
   */
   for (j = 0; j < Y_LENGTH; j++) 
   {
       for (i = 0; i < X_LENGTH; i++)
           data[j][i] = (i + 1) + (j + 1);
   }

   /*
   * Create the file and initialize the SD interface.
   */
   sd_id = SDstart (FILE_NAME, DFACC_CREATE);

   /*
   * Define dimensions of the array. Make the first dimension 
   * appendable by defining its length to be unlimited.
   */
   dim_sizes[0] = SD_UNLIMITED;
   dim_sizes[1] = X_LENGTH;

   /*
   * Create the array data set.
   */
   sds_id = SDcreate (sd_id, SDS_NAME, DFNT_INT32, RANK, dim_sizes);

   /*
   * Define the location and the size of the data to be written 
   * to the data set. 
   */
   start[0] = start[1] = 0;
   edges[0] = Y_LENGTH;
   edges[1] = X_LENGTH;

   /*
   * Write the data. 
   */
   status = SDwritedata (sds_id, start, NULL, edges, (VOIDP)data);

   /*
   * Terminate access to the array data set, terminate access 
   * to the SD interface, and close the file.
   */
   status = SDendaccess (sds_id);
   status = SDend (sd_id);

   /*
   * Store the array values to be appended to the data set.
   */
   for (i = 0; i < X_LENGTH; i++)
       append_data[i] = 1000 + i;

   /*
   * Reopen the file and initialize the SD interface.
   */
   sd_id = SDstart (FILE_NAME, DFACC_WRITE);

   /*
   * Select the first data set. 
   */ 
   sds_index = 0; 
   sds_id = SDselect (sd_id, sds_index);
 
   /*  
   * Check if selected SDS is unlimited. If it is not, then terminate access
   * to the SD interface and close the file. 
   */
   if ( SDisrecord (sds_id) ) 
   {

   /*
   * Define the location of the append to start at the first column 
   * of the 11th row of the data set and to stop at the end of the
   * eleventh row.
   */
   start[0] = Y_LENGTH;
   start[1] = 0;
   edges[0] = 1;
   edges[1] = X_LENGTH;

   /*
   * Append data to the data set.
   */
   status = SDwritedata (sds_id, start, NULL, edges, (VOIDP)append_data);
   }

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
