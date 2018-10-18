#include "mfhdf.h"

#define FILE_NAME     "SLABS.hdf"
#define SDS_NAME      "FilledBySlabs"
#define X_LENGTH      4
#define Y_LENGTH      5
#define Z_LENGTH      6
#define RANK          3  

int main()
{
   /************************* Variable declaration **************************/

   int32 sd_id, sds_id;
   intn  status;
   int32 dim_sizes[3], start[3], edges[3];
   int32 data[Z_LENGTH][Y_LENGTH][X_LENGTH];
   int32 zx_data[Z_LENGTH][X_LENGTH];
   int   i, j, k;
 
   /********************* End of variable declaration ***********************/

   /*
   * Data initialization. 
   */
   for (k = 0; k < Z_LENGTH; k++)
       for (j = 0; j < Y_LENGTH; j++)
           for (i = 0; i < X_LENGTH; i++)
               data[k][j][i] = (i + 1) + (j + 1) + (k + 1);

   /*
   * Create the file and initialize the SD interface.
   */
   sd_id = SDstart (FILE_NAME, DFACC_CREATE);

   /*
   * Define dimensions of the array to be created.
   */
   dim_sizes[0] = Z_LENGTH;
   dim_sizes[1] = Y_LENGTH;
   dim_sizes[2] = X_LENGTH;

   /*
   * Create the array with the name defined in SDS_NAME.
   */
   sds_id = SDcreate (sd_id, SDS_NAME, DFNT_INT32, RANK, dim_sizes);

   /*
   * Set the parameters start and edges to write  
   * a 6x4 element slab of data to the data set; note
   * that edges[1] is set to 1 to define a 2-dimensional slab
   * parallel to the ZX plane.  
   * start[1] (slab position in the array) is initialized inside
   * the for loop.
   */
   edges[0] = Z_LENGTH;
   edges[1] = 1;
   edges[2] = X_LENGTH;
   start[0] = start[2] = 0;
   for (j = 0; j < Y_LENGTH; j++)
   {
       start[1] = j;
    
       /*
       * Initialize zx_data buffer (data slab).
       */ 
       for ( k = 0; k < Z_LENGTH; k++)
       {
           for ( i = 0; i < X_LENGTH; i++)
           {
                 zx_data[k][i] = data[k][j][i];
           }
   }

   /*
   * Write the data slab into the SDS array defined in SDS_NAME. 
   * Note that the 3rd parameter is NULL which indicates that consecutive
   * slabs in the Y direction are written.
   */
   status = SDwritedata (sds_id, start, NULL, edges, (VOIDP)zx_data); 
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
