#include "hdf.h"

#define  FILE_NAME        "General_Vdatas.hdf"
#define  N_RECORDS        10        /* number of records the vdata contains */
#define  ORDER_1          3         /* order of first field */
#define  ORDER_2          1         /* order of second field */
#define  ORDER_3          2         /* order of third field */
#define  CLASS_NAME       "Particle Data"
#define  VDATA_NAME       "Solid Particle"
#define  FIELD1_NAME      "Position"      /* contains x, y, z values */
#define  FIELD2_NAME      "Mass"          /* contains weight values */
#define  FIELD3_NAME      "Temperature"   /* contains min and max values */
#define  FIELDNAME_LIST   "Position,Mass,Temperature" /* No spaces b/w names */

/* number of values per record */
#define  N_VALS_PER_REC   (ORDER_1 + ORDER_2 + ORDER_3)

int main( ) 
{
   /************************* Variable declaration **************************/

   intn   status_n;     /* returned status for functions returning an intn  */
   int32  status_32,    /* returned status for functions returning an int32 */
          file_id, vdata_id,
          vdata_ref = -1,    /* ref number of a vdata, set to -1 to create  */
          num_of_records;    /* number of records actually written to vdata */
   int16  rec_num;           /* current record number */
   float32  data_buf[N_RECORDS][N_VALS_PER_REC]; /* buffer for vdata values */

   /********************** End of variable declaration **********************/

   /* 
   * Open the HDF file for writing.
   */
   file_id = Hopen (FILE_NAME, DFACC_WRITE, 0);

   /* 
   * Initialize the VS interface.
   */
   status_n = Vstart (file_id);

   /* 
   * Create a new vdata.
   */
   vdata_id = VSattach (file_id, vdata_ref, "w");

   /* 
   * Set name and class name of the vdata.
   */
   status_32 = VSsetname (vdata_id, VDATA_NAME);
   status_32 = VSsetclass (vdata_id, CLASS_NAME);

   /* 
   * Introduce each field's name, data type, and order.  This is the first
   * part in defining a field.
   */
   status_n = VSfdefine (vdata_id, FIELD1_NAME, DFNT_FLOAT32, ORDER_1 );
   status_n = VSfdefine (vdata_id, FIELD2_NAME, DFNT_FLOAT32, ORDER_2 );
   status_n = VSfdefine (vdata_id, FIELD3_NAME, DFNT_FLOAT32, ORDER_3 );

   /* 
   * Finalize the definition of the fields.
   */
   status_n = VSsetfields (vdata_id, FIELDNAME_LIST);

   /* 
   * Buffer the data by the record for fully interlaced mode.  Note that the
   * first three elements contain the three values of the first field, the
   * fourth element contains the value of the second field, and the last two
   * elements contain the two values of the third field.
   */
   for (rec_num = 0; rec_num < N_RECORDS; rec_num++)
   {
      data_buf[rec_num][0] = 1.0 * rec_num;
      data_buf[rec_num][1] = 2.0 * rec_num;
      data_buf[rec_num][2] = 3.0 * rec_num;
      data_buf[rec_num][3] = 0.1 + rec_num;
      data_buf[rec_num][4] = 0.0;
      data_buf[rec_num][5] = 65.0;
   }

   /* 
   * Write the data from data_buf to the vdata with full interlacing mode.
   */
   num_of_records = VSwrite (vdata_id, (uint8 *)data_buf, N_RECORDS, 
                             FULL_INTERLACE);

   /* 
   * Terminate access to the vdata and to the VS interface, then close 
   * the HDF file.
   */
   status_32 = VSdetach (vdata_id);
   status_n  = Vend (file_id);
   status_32 = Hclose (file_id);
   return 0;
}
