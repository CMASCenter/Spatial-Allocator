#include "hdf.h"

#define  FILE_NAME        "Packed_Vdata.hdf"
#define  VDATA_NAME       "Mixed Data Vdata"
#define  CLASS_NAME       "General Data Class"
#define  FIELD1_NAME      "Temp" 
#define  FIELD2_NAME      "Height" 
#define  FIELD3_NAME      "Speed" 
#define  FIELD4_NAME      "Ident"
#define  ORDER            1        /* number of values in the field         */
#define  N_RECORDS        20       /* number of records the vdata contains  */
#define  N_FIELDS         4        /* number of fields in the vdata         */
#define  FIELDNAME_LIST   "Temp,Height,Speed,Ident"  /* No spaces b/w names */

/* number of bytes of the data to be written, i.e., the size of all the
   field values combined times the number of records */
#define BUF_SIZE (2*sizeof(float32) + sizeof(int16) + sizeof(char)) * N_RECORDS

int main( ) 
{
   /************************* Variable declaration **************************/

   intn  status_n;      /* returned status for functions returning an intn  */
   int32 status_32,     /* returned status for functions returning an int32 */
         file_id, vdata_id,
         vdata_ref = -1,   /* vdata's reference number, set to -1 to create */
         num_of_records; /* number of records actually written to the vdata */
   float32 temp[N_RECORDS];       /* buffer to hold values of first field   */
   int16   height[N_RECORDS];     /* buffer to hold values of second field  */
   float32 speed[N_RECORDS];      /* buffer to hold values of third field   */
   char8   ident[N_RECORDS];      /* buffer to hold values of fourth field  */
   VOIDP   fldbufptrs[N_FIELDS];/*pointers to be pointing to the field buffers*/
   uint16  databuf[BUF_SIZE]; /* buffer to hold the data after being packed*/
   int     i;

   /********************** End of variable declaration **********************/

   /* 
   * Create an HDF file. 
   */
   file_id = Hopen (FILE_NAME, DFACC_CREATE, 0);

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
   * part in defining a vdata field.
   */
   status_n = VSfdefine (vdata_id, FIELD1_NAME, DFNT_FLOAT32, ORDER); 
   status_n = VSfdefine (vdata_id, FIELD2_NAME, DFNT_INT16, ORDER); 
   status_n = VSfdefine (vdata_id, FIELD3_NAME, DFNT_FLOAT32, ORDER); 
   status_n = VSfdefine (vdata_id, FIELD4_NAME, DFNT_CHAR8, ORDER); 

   /*
   * Finalize the definition of the fields of the vdata.
   */
   status_n = VSsetfields (vdata_id, FIELDNAME_LIST);

   /* 
   * Enter data values into the field buffers by the records.
   */
   for (i = 0; i < N_RECORDS; i++)
   {
      temp[i] = 1.11 * (i+1);
      height[i] = i;
      speed[i] = 1.11 * (i+1);
      ident[i] = 'A' + i;
   }

   /* 
   * Build an array of pointers each of which points to a field buffer that
   * holds all values of the field.
   */
   fldbufptrs[0] = &temp[0];
   fldbufptrs[1] = &height[0];
   fldbufptrs[2] = &speed[0];
   fldbufptrs[3] = &ident[0];

   /* 
   * Pack all data in the field buffers that are pointed to by the set of
   * pointers fldbufptrs, and store the packed data into the buffer 
   * databuf.  Note that the second parameter is _HDF_VSPACK for packing.
   */
   status_n = VSfpack (vdata_id,_HDF_VSPACK, NULL, (VOIDP)databuf,
           BUF_SIZE, N_RECORDS, NULL, (VOIDP)fldbufptrs);

   /* 
   * Write all records of the packed data to the vdata. 
   */
   num_of_records = VSwrite (vdata_id, (uint8 *)databuf, N_RECORDS, 
                             FULL_INTERLACE); 

   /* 
   * Terminate access to the vdata and the VS interface, then close 
   * the HDF file.
   */
   status_32 = VSdetach (vdata_id);
   status_n = Vend (file_id);
   status_32 = Hclose (file_id);
   return 0;
}
