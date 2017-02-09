#include "mfhdf.h"

#define FILE_NAME     "SDSchunked.hdf"
#define SDS_NAME      "ChunkedData"
#define RANK          2

int main()
{
   /************************* Variable declaration **************************/

   int32         sd_id, sds_id, sds_index;
   intn          status;
   int32         flag, maxcache, new_maxcache;
   int32         dim_sizes[2], origin[2];
   HDF_CHUNK_DEF c_def, c_def_out; /* Chunking definitions */ 
   int32         comp_flag, c_flags;
   int16         all_data[9][4];
   int32         start[2], edges[2];
   int16         chunk_out[3][2];
   int16         row[2] = { 5, 5 };
   int16         column[3] = { 4, 4, 4 };
   int16         fill_value = 0;   /* Fill value */
   int           i,j;
   /*
   * Declare chunks data type and initialize some of them. 
   */
          int16 chunk1[3][2] = { 1, 1,
                                 1, 1,
                                 1, 1 }; 

          int16 chunk2[3][2] = { 2, 2,
                                 2, 2,
                                 2, 2 }; 

          int16 chunk3[3][2] = { 3, 3,
                                 3, 3,
                                 3, 3 }; 

          int16 chunk6[3][2] = { 6, 6,
                                 6, 6,
                                 6, 6 };

    /********************* End of variable declaration ***********************/
    /*
    * Define chunk's dimensions.
    *
    *         In this example we do not use compression. 
    *         To use chunking with RLE, Skipping Huffman, and GZIP
    *         compression, initialize
    *
    *                c_def.comp.chunk_lengths[0] = 3;
    *                c_def.comp.chunk_lengths[1] = 2; 
    *
    *         To use chunking with NBIT, initialize
    *
    *                c_def.nbit.chunk_lengths[0] = 3;
    *                c_def.nbit.chunk_lengths[1] = 2; 
    *
    */
    c_def.chunk_lengths[0] = 3;
    c_def.chunk_lengths[1] = 2;

    /*
    * Create the file and initialize SD interface.
    */
    sd_id = SDstart (FILE_NAME, DFACC_CREATE);

    /*
    * Create 9x4 SDS.
    */
    dim_sizes[0] = 9;
    dim_sizes[1] = 4;
    sds_id = SDcreate (sd_id, SDS_NAME,DFNT_INT16, RANK, dim_sizes);

    /*
    * Fill the SDS array with the fill value.
    */
    status = SDsetfillvalue (sds_id, (VOIDP)&fill_value);

    /*
    * Create chunked SDS.
    * In this example we do not use compression ( third
    * parameter of SDsetchunk is set to HDF_CHUNK).
    *
    * To use RLE compresssion, set compression type and flag
    *
    *            c_def.comp.comp_type = COMP_CODE_RLE;
    *            comp_flag = HDF_CHUNK | HDF_COMP;
    *          
    * To use Skipping Huffman compression, set compression type, flag
    * and skipping size skp_size
    *
    *            c_def.comp.comp_type = COMP_CODE_SKPHUFF;
    *            c_def.comp.cinfo.skphuff.skp_size = value;             
    *            comp_flag = HDF_CHUNK | HDF_COMP;
    *
    * To use GZIP compression, set compression type, flag and
    * deflate level
    * 
    *            c_def.comp.comp_type = COMP_CODE_DEFLATE;
    *            c_def.comp.cinfo.deflate.level = value;             
    *            comp_flag = HDF_CHUNK | HDF_COMP;
    *
    * To use NBIT compression, set compression flag and
    * compression parameters
    *          
    *            comp_flag = HDF_CHUNK | HDF_NBIT;
    *            c_def.nbit.start_bit = value1;             
    *            c_def.nbit.bit_len   = value2;             
    *            c_def.nbit.sign_ext  = value3;             
    *            c_def.nbit.fill_one  = value4;             
    */
    comp_flag = HDF_CHUNK;
    status = SDsetchunk (sds_id, c_def, comp_flag);

    /*
    * Set chunk cache to hold maximum of 3 chunks.
    */
    maxcache = 3;
    flag = 0;
    new_maxcache = SDsetchunkcache (sds_id, maxcache, flag);

    /* 
    * Write chunks using SDwritechunk function.
    * Chunks can be written in any order.
    */

    /* 
    * Write the chunk with the coordinates (0,0).
    */
    origin[0] = 0;
    origin[1] = 0;
    status = SDwritechunk (sds_id, origin, (VOIDP) chunk1);

    /*   
    * Write the chunk with the coordinates (1,0).
    */
    origin[0] = 1;
    origin[1] = 0;
    status = SDwritechunk (sds_id, origin, (VOIDP) chunk3);

    /*   
    * Write the chunk with the coordinates (0,1). 
    */
    origin[0] = 0;
    origin[1] = 1;
    status = SDwritechunk (sds_id, origin, (VOIDP) chunk2);

    /* 
    * Write chunk with the coordinates (1,2) using
    * SDwritedata function.
    */
    start[0] = 6;
    start[1] = 2;
    edges[0] = 3;
    edges[1] = 2;
    status = SDwritedata (sds_id, start, NULL, edges, (VOIDP) chunk6); 

    /* 
    * Fill second column in the chunk with the coordinates (1,1)
    * using SDwritedata function.
    */
    start[0] = 3;
    start[1] = 3;
    edges[0] = 3;
    edges[1] = 1;
    status = SDwritedata (sds_id, start, NULL, edges, (VOIDP) column); 

    /* 
    * Fill second row in the chunk with the coordinates (0,2)
    * using SDwritedata function.
    */
    start[0] = 7;
    start[1] = 0;
    edges[0] = 1;
    edges[1] = 2;
    status = SDwritedata (sds_id, start, NULL, edges, (VOIDP) row); 
           
    /* 
    * Terminate access to the data set.
    */
    status = SDendaccess (sds_id);

    /*
    * Terminate access to the SD interface and close the file.
    */
    status = SDend (sd_id);

    /*
    * Reopen the file and access the first data set.
    */
    sd_id = SDstart (FILE_NAME, DFACC_READ);
    sds_index = 0;
    sds_id = SDselect (sd_id, sds_index);

    /*
    * Get information about the SDS. Only chunk lengths and compression
    * flag can be returned. Compression information is not available if
    * NBIT, Skipping Huffman, or GZIP compression is used.
    */
    status = SDgetchunkinfo (sds_id, &c_def_out, &c_flags);
    if (c_flags == HDF_CHUNK )
       printf(" SDS is chunked\nChunk's dimensions %dx%d\n",
              c_def_out.chunk_lengths[0],
              c_def_out.chunk_lengths[1]);
    else if (c_flags == (HDF_CHUNK | HDF_COMP))
         printf("SDS is chunked and compressed\nChunk's dimensions %dx%d\n",
                  c_def_out.comp.chunk_lengths[0],
                  c_def_out.comp.chunk_lengths[1]);
    else if (c_flags == (HDF_CHUNK | HDF_NBIT))
         printf ("SDS is chunked (NBIT)\nChunk's dimensions %dx%d\n",
                  c_def_out.nbit.chunk_lengths[0],
                  c_def_out.nbit.chunk_lengths[1]);

    /*
    * Read the entire data set using SDreaddata function.
    */
    start[0] = 0;
    start[1] = 0;
    edges[0] = 9;
    edges[1] = 4;
    status = SDreaddata (sds_id, start, NULL, edges, (VOIDP)all_data);

    /* 
    * Print out what we have read.
    * The following information should be displayed:
    *
    * SDS is chunked 
    * Chunk's dimensions 3x2
    *          1 1 2 
    *          1 1 2 2
    *          1 1 2 2
    *          3 3 0 4
    *          3 3 0 4
    *          3 3 0 4
    *          0 0 6 6
    *          5 5 6 6
    *          0 0 6 6
    */
    for (j=0; j<9; j++) 
    {
         for (i=0; i<4; i++) printf (" %d", all_data[j][i]);
         printf ("\n");
    }

    /* 
    * Read chunk with the coordinates (2,0) and display it.
    */
    origin[0] = 2;
    origin[1] = 0;    	
    status = SDreadchunk (sds_id, origin, chunk_out);
    printf (" Chunk (2,0) \n");
    for (j=0; j<3; j++) 
    {
         for (i=0; i<2; i++) printf (" %d", chunk_out[j][i]);
         printf ("\n");
    }

    /* 
    * Read chunk with the coordinates (1,1) and display it.
    */
    origin[0] = 1;
    origin[1] = 1;    	
    status = SDreadchunk (sds_id, origin, chunk_out);
    printf (" Chunk (1,1) \n");
    for (j=0; j<3; j++) 
    {
         for (i=0; i<2; i++) printf (" %d", chunk_out[j][i]);
         printf ("\n");
    }

    /*  The following information is displayed:
    *
    *   Chunk (2,0) 
    *   0 0
    *   5 5
    *   0 0
    *   Chunk (1,1) 
    *   0 4
    *   0 4
    *   0 4
    */

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
