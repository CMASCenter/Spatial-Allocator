#include "hdf.h"

#define  FILE_NAME       "General_Vdatas.hdf"
#define  VDATA_NAME      "Solid Particle"
#define  N_RECORDS       5    /* number of records the vdata contains */
#define  RECORD_INDEX    3    /* position where reading starts - 4th record */
#define  ORDER_1         3    /* order of first field to be read */
#define  ORDER_2         2    /* order of second field to be read */
#define  FIELDNAME_LIST  "Position,Temperature" /* only two fields are read */
#define  N_VALS_PER_REC  (ORDER_1 + ORDER_2)   
                         /* number of values per record */

int main( ) 
{
   /************************* Variable declaration **************************/

   intn  status_n;      /* returned status for functions returning an intn  */
   int32 status_32,     /* returned status for functions returning an int32 */
         file_id, vdata_id, 
         vdata_ref,     /* vdata's reference number */
         num_of_records, /* number of records actually written to the vdata */
         record_pos;    /* position of the current record */
   int16 i, rec_num;    /* current record number in the vdata */
   float32 databuf[N_RECORDS][N_VALS_PER_REC];   /* buffer for vdata values */

   /********************** End of variable declaration **********************/

   /* 
   * Open the HDF file for reading.
   */
   file_id = Hopen (FILE_NAME, DFACC_READ, 0);

   /* 
   * Initialize the VS interface.
   */
   status_n = Vstart (file_id);

   /*
   * Get the reference number of the vdata, whose name is specified in 
   * VDATA_NAME, using VSfind, which will be discussed in Section 4.7.3.
   */
   vdata_ref = VSfind (file_id, VDATA_NAME);

   /* 
   * Attach to the vdata for reading if it is found, otherwise 
   * exit the program.
   */
   if (vdata_ref == 0) exit;
   vdata_id = VSattach (file_id, vdata_ref, "r");

   /* 
   * Specify the fields that will be read.
   */
   status_n = VSsetfields (vdata_id, FIELDNAME_LIST);

   /*
   * Place the current point to the position specified in RECORD_INDEX.
   */
   record_pos = VSseek (vdata_id, RECORD_INDEX);

   /* 
   * Read the next N_RECORDS records from the vdata and store the data 
   * in the buffer databuf with fully interlaced mode.
   */
   num_of_records = VSread (vdata_id, (uint8 *)databuf, N_RECORDS, 
                            FULL_INTERLACE);

   /*
   * Display the read data as many records as the number of records 
   * returned by VSread.
   */
   printf ("\n       Particle Position        Temperature Range\n\n");
   for (rec_num = 0; rec_num < num_of_records; rec_num++)
   {
      printf ("   %6.2f, %6.2f, %6.2f        %6.2f, %6.2f\n", 
        databuf[rec_num][0], databuf[rec_num][1], databuf[rec_num][2], 
        databuf[rec_num][3], databuf[rec_num][4]);
   }

   /* 
   * Terminate access to the vdata and to the VS interface, then close 
   * the HDF file.
   */
   status_32 = VSdetach (vdata_id);
   status_n = Vend (file_id);
   status_32 = Hclose (file_id);
   return 0;
}
