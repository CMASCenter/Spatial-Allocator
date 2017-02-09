#include "hdf.h"

#define  N_RECORDS       20      /* number of records to be read */
#define  N_FIELDS        2       /* number of fields to be read */
#define  FILE_NAME       "Packed_Vdata.hdf"
#define  VDATA_NAME      "Mixed Data Vdata"
#define  FIELDNAME_LIST  "Temp,Ident"

/* number of bytes of the data to be read */
#define  BUFFER_SIZE     ( sizeof(float32) + sizeof(char)) * N_RECORDS 

int main ()
{
   /************************* Variable declaration **************************/

   intn  status_n;      /* returned status for functions returning an intn  */
   int32 status_32,     /* returned status for functions returning an int32 */
         file_id, vdata_id, 
         num_of_records,        /* number of records actually read */
         vdata_ref,             /* reference number of the vdata to be read */
         buffer_size;           /* number of bytes the vdata can hold       */
   float32 itemp[N_RECORDS];    /* buffer to hold values of first field     */
   char  idents[N_RECORDS];     /* buffer to hold values of fourth field    */
   uint8 databuf[BUFFER_SIZE];  /* buffer to hold read data, still packed   */
   VOIDP fldbufptrs[N_FIELDS];/*pointers to be pointing to the field buffers*/
   int   i;

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
   * Attach to the vdata for reading. 
   */
   vdata_id = VSattach (file_id, vdata_ref, "r");

   /*
   * Specify the fields that will be read. 
   */
   status_n = VSsetfields(vdata_id, FIELDNAME_LIST);

   /*
   * Read N_RECORDS records of the vdata and store the values into the 
   * buffer databuf. 
   */
   num_of_records = VSread (vdata_id, (uint8 *)databuf, N_RECORDS, 
                            FULL_INTERLACE);

   /*
   * Build an array of pointers each of which points to an array that
   * will hold all values of a field after being unpacked.
   */
   fldbufptrs[0] = &itemp[0];
   fldbufptrs[1] = &idents[0];
   
   /*
   * Unpack the data from the buffer databuf and store the values into the 
   * appropriate field buffers pointed to by the set of pointers fldbufptrs.
   * Note that the second parameter is _HDF_VSUNPACK for unpacking and the
   * number of records is the one returned by VSread.
   */
   status_n = VSfpack (vdata_id, _HDF_VSUNPACK, FIELDNAME_LIST, (VOIDP)databuf,
               BUFFER_SIZE, num_of_records, NULL, (VOIDP)fldbufptrs);

   /*
   * Display the read data being stored in the field buffers.
   */
   printf ("\n     Temp      Ident\n");
   for (i=0; i < num_of_records; i++)
       printf ("   %6.2f        %c\n", itemp[i], idents[i]);

   /*
   * Terminate access to the vdata and the VS interface, then close 
   * the HDF file. 
   */
   status_32 = VSdetach (vdata_id);
   status_n = Vend (file_id);
   status_32 = Hclose (file_id);
   return 0;
}
