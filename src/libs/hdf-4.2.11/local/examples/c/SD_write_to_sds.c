#include "mfhdf.h"

#define FILE_NAME     "SDS.hdf"
#define X_LENGTH      5
#define Y_LENGTH      16

int main()
{
   /************************* Variable declaration **************************/

   int32 sd_id, sds_id, sds_index;
   intn  status;
   int32 start[2], edges[2];
   int32 data[Y_LENGTH][X_LENGTH];
   int   i, j;

   /********************* End of variable declaration ***********************/

   /*
   * Data set data initialization.
   */
   for (j = 0; j < Y_LENGTH; j++) {
       for (i = 0; i < X_LENGTH; i++)
            data[j][i] = (i + j) + 1;
   }

   /*
   * Open the file and initialize the SD interface.
   */
   sd_id = SDstart (FILE_NAME, DFACC_WRITE);

   /*
   * Attach to the first data set.
   */ 
   sds_index = 0;
   sds_id = SDselect (sd_id, sds_index);

   /* 
   * Define the location and size of the data to be written to the data set.
   */
   start[0] = 0;
   start[1] = 0;
   edges[0] = Y_LENGTH;
   edges[1] = X_LENGTH;

   /*
   * Write the stored data to the data set. The third argument is set to NULL
   * to specify contiguous data elements. The last argument must
   * be explicitly cast to a generic pointer since SDwritedata is designed
   * to write generic data. 
   */
   status = SDwritedata (sds_id, start, NULL, edges, (VOIDP)data);

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
