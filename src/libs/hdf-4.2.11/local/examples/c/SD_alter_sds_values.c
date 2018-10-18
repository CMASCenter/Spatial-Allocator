#include "mfhdf.h"

#define FILE_NAME     "SDS.hdf"

int main()
{
   /************************* Variable declaration **************************/

   int32 sd_id, sds_id, sds_index;
   intn  status;
   int32 start[2], edges[2];
   int32 new_data[2];

   /********************* End of variable declaration ***********************/
   /* 
   * Open the file and initialize the SD interface with write access.
   */
   sd_id = SDstart (FILE_NAME, DFACC_WRITE);

   /*
   * Select the first data set.
   */
   sds_index = 0;
   sds_id = SDselect (sd_id, sds_index);

   /*
   * Set up the start and edge parameters to write new element values
   * into 10th row, 2nd column place, and 11th row, 2nd column place. 
   */
   start[0] = 9;     /* starting at 10th row   */
   start[1] = 1;     /* starting at 2nd column */
   edges[0] = 2;     /* rows 10th and 11th     */
   edges[1] = 1;     /* column 2nd only        */
       
   /*
   * Initialize buffer with the new values to be written.
   */
   new_data[0] = new_data[1] = 1000; 
        
   /*
   * Write the new values. 
   */
   status = SDwritedata (sds_id, start, NULL, edges, (VOIDP)new_data);

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
