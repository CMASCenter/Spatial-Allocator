/* Example 4_struct. Writing a multi-field and mixed-type vdata
with packing - using struct instead of array to buffer field values.

This example illustrates the use of VSfpack and VSwrite to write a
vdata with data of different types.  The problem in this example is
identical to that in Example 4 of Chapter Vdata in the User's Guide.
However, the two programs differ in the implementation of the
data structures that hold the user's data.

In this example, the program creates an HDF file named
"Packed_Vdata.hdf" then defines a vdata, which is named "Mixed
Data Vdata" and belongs to class "General Data Class".  The vdata
contains four order-1 fields, "Temp", "Height", "Speed", and "Ident" of
type float32, int16, float32, and char, respectively.  The program then
packs the data in fully interlaced mode into a buffer and writes the
packed data to the vdata.  Note that, in this example, each VSfpack
call packs 1 record while in Example 4, a VSfpack call packs all
N_RECORDS.  This difference is the result of using an array of
structs in this example to hold the field values instead of 
individual arrays as in Example 4.  */

#include "hdf.h"

#define  FILE_NAME        "Packed_Vdata.hdf"
#define  VDATA_NAME       "Mixed Data Vdata"
#define  CLASS_NAME       "General Data Class"
#define  N_RECORDS        20        /* number of records the vdata contains */
#define  N_FIELDS         4         /* number of fields in the vdata */
#define  FIELD1_NAME      "Temp" 
#define  FIELD2_NAME      "Height" 
#define  FIELD3_NAME      "Speed" 
#define  FIELD4_NAME      "Ident"
#define  FIELDNAME_LIST   "Temp,Height,Speed,Ident"  /* No spaces b/w names */

/* RECORD_SIZE is the number of bytes of all the field values combined and
   BUFFER_SIZE is the number of bytes of data to be written */
#define  RECORD_SIZE      (2 * sizeof(float32) + sizeof(int16) + sizeof(char))
#define  BUFFER_SIZE      (RECORD_SIZE * N_RECORDS)

int main( ) 
{
   /************************* Variable declaration **************************/

   intn  status_n;      /* returned status for functions returning an intn  */
   int32 status_32,     /* returned status for functions returning an int32 */
         file_id, vdata_id,
         num_of_records; /* number of records actually written to the vdata */
   uint8 databuf[BUFFER_SIZE];/* buffer to hold the data after being packed */
   uint8 *pntr; /* pointer pointing to the current record in the data buffer*/
   int16 rec_num;        /* current record number */ 

   struct {
      float32      temp;   /* to hold value of the first field of the vdata */
      int16        height;/* to hold value of the second field of the vdata */
      float32      speed;  /* to hold value of the third field of the vdata */
      char         ident; /* to hold value of the fourth field of the vdata */
   } source[N_RECORDS];

   /* pointers to be pointing to the fields in the struct buffer */
   VOIDP  fldbufptrs[N_FIELDS];

   /********************** End of variable declaration **********************/

   /* 
   * Create the HDF file. 
   */
   file_id = Hopen (FILE_NAME, DFACC_CREATE, 0);

   /* 
   * Initialize the VS interface. 
   */
   status_n = Vstart (file_id);

   /* 
   * Create a new vdata. 
   */
   vdata_id = VSattach (file_id, -1, "w");

   /*
   * Set name and class name of the vdata.
   */
   status_32 = VSsetname (vdata_id, VDATA_NAME);
   status_32 = VSsetclass (vdata_id, CLASS_NAME);

   /* 
   * Introduce each field's name, data type, and order.  This is the first
   * part in defining a vdata field.
   */
   status_n = VSfdefine (vdata_id, FIELD1_NAME, DFNT_FLOAT32, 1); 
   status_n = VSfdefine (vdata_id, FIELD2_NAME, DFNT_INT16, 1); 
   status_n = VSfdefine (vdata_id, FIELD3_NAME, DFNT_FLOAT32, 1); 
   status_n = VSfdefine (vdata_id, FIELD4_NAME, DFNT_CHAR8, 1); 

   /*
   * Finalize the definition of the fields to be written to.
   */
   status_n = VSsetfields (vdata_id, FIELDNAME_LIST);

   /*
   * Initialize pointer for traversing the buffer to pack each record.
   */
   pntr = &databuf[0];

   /* 
   * Enter data values into each record.
   */
   for (rec_num = 0; rec_num < N_RECORDS; rec_num++) {
      source[rec_num].temp = 1.11 * (rec_num+1);
      source[rec_num].height = rec_num;
      source[rec_num].speed = 1.11 * (rec_num+1);
      source[rec_num].ident = 'A' + rec_num;
   }

   /* 
   * Pack one record at a time.
   */
   for (rec_num = 0; rec_num < N_RECORDS; rec_num++)
   {
      /* 
      * Build an array of pointers each of which points to a space that
      * holds the value of the corresponding field in this record.
      */
      fldbufptrs[0] = &source[rec_num].temp;
      fldbufptrs[1] = &source[rec_num].height;
      fldbufptrs[2] = &source[rec_num].speed;
      fldbufptrs[3] = &source[rec_num].ident;

      /* 
      * Pack the data in the field buffers into the data buffer at the 
      * current record, i.e. indicated by "pntr".
      */
      status_n = VSfpack (vdata_id,_HDF_VSPACK, NULL, (VOIDP)pntr,
              RECORD_SIZE, 1, NULL, fldbufptrs);

      /*
      * Advance the current position in the buffer.
      */
      pntr = pntr + RECORD_SIZE;
   }        

   /* 
   * Write all records of the packed data to the vdata. 
   */
   num_of_records = VSwrite (vdata_id, (uint8 *)databuf, N_RECORDS, FULL_INTERLACE); 

   /* 
   * Terminate access to the Vdata and the VS interface, 
   * then close the HDF file.
   */
   status_32 = VSdetach (vdata_id);
   status_n = Vend (file_id);
   status_32 = Hclose (file_id);
   return 0;
}
