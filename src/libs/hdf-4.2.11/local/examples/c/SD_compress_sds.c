#include "mfhdf.h"

#define FILE_NAME      "SDScompressed.hdf"
#define SDS_NAME       "SDSgzip"
#define X_LENGTH       5
#define Y_LENGTH       16
#define RANK           2

int main()
{
   /************************* Variable declaration **************************/

   int32     sd_id, sds_id;
   intn      status;
   int32     comp_type;    /* Compression flag */
   comp_info c_info;   /* Compression structure */
   int32     start[2], edges[2], dim_sizes[2];
   int32     data[Y_LENGTH][X_LENGTH];
   int       i, j;

   /********************* End of variable declaration ***********************/

   /*
   * Buffer array data and define array dimensions.
   */
   for (j = 0; j < Y_LENGTH; j++) 
   {
	for (i = 0; i < X_LENGTH; i++)
		data[j][i] = (i + j) + 1;
   }
   dim_sizes[0] = Y_LENGTH;
   dim_sizes[1] = X_LENGTH;

   /*
   * Create the file and initialize the SD interface.
   */
   sd_id = SDstart (FILE_NAME, DFACC_CREATE);

   /*
   * Create the data set with the name defined in SDS_NAME. 
   */ 
   sds_id = SDcreate (sd_id, SDS_NAME, DFNT_INT32, RANK, dim_sizes);

   /*
   * Ininitialize compression structure element and compression
   * flag for GZIP compression and call SDsetcompress.
   *
   *   To use the Skipping Huffman compression method, initialize
   *          comp_type = COMP_CODE_SKPHUFF
   *          c_info.skphuff.skp_size = value
   *
   *   To use the RLE compression method, initialize
   *          comp_type = COMP_CODE_RLE
   *   No structure element needs to be initialized.
   */
   comp_type = COMP_CODE_DEFLATE;
   c_info.deflate.level = 6;
   status = SDsetcompress (sds_id, comp_type, &c_info); 

   /* 
   * Define the location and size of the data set
   * to be written to the file.
   */
   start[0] = 0;
   start[1] = 0;
   edges[0] = Y_LENGTH;
   edges[1] = X_LENGTH;

   /*
   * Write the stored data to the data set. The last argument 
   * must be explicitly cast to a generic pointer since SDwritedata
   * is designed to write generic data. 
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
