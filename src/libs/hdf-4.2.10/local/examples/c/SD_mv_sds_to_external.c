#include "mfhdf.h"

#define FILE_NAME     "SDS.hdf"
#define EXT_FILE_NAME "ExternalSDS"
#define OFFSET        24

int main()
{

   /************************* Variable declaration **************************/

   int32 sd_id, sds_id, sds_index, offset;
   intn  status;

   /********************* End of variable declaration ***********************/

   /*
   * Open the file and initialize the SD interface.
   */
   sd_id = SDstart (FILE_NAME, DFACC_WRITE);

   /*
   * Select the first data set.
   */
   sds_index = 0;   
   sds_id = SDselect (sd_id, sds_index);

   /*
   * Create a file with the name EXT_FILE_NAME and move the data set
   * values into it, starting at byte location OFFSET.
   */
   status = SDsetexternalfile (sds_id, EXT_FILE_NAME, OFFSET);

   /*
   * Terminate access to the data set, SD interface, and file.
   */
   status = SDendaccess (sds_id);
   status = SDend (sd_id);

   return 0;
}
