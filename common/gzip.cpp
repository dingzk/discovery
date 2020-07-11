//
// Created by zhenkai on 2020/7/11.
//

#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "gzip.h"

#define CHUNK 16384

#define ZLIB_ENCODING_RAW       -0xf
#define ZLIB_ENCODING_GZIP      0x1f
#define ZLIB_ENCODING_DEFLATE   0x0f

#define PHP_ZLIB_ENCODING_ANY       0x2f /* automatic zlib or gzip decoding */

int gzdecode(unsigned char *source, int len, char **dest)
{
    int status, round = 0;
    unsigned int have;
    z_stream strm;
    unsigned char out[CHUNK];
    int totalsize = 0;

    if (len <= 0) {
        return Z_DATA_ERROR;
    }

    *dest = NULL;
    /*   allocate   inflate   state   */
    strm.zalloc = Z_NULL;
    strm.zfree = Z_NULL;
    strm.opaque = Z_NULL;
    strm.avail_in = 0;
    strm.next_in = Z_NULL;

    status = inflateInit2(&strm, ZLIB_ENCODING_GZIP);
    if (status != Z_OK)
        return status;
    strm.avail_in = len;
    strm.next_in = source;

    /*   run   inflate()   on   input   until   output   buffer   not   full   */
    do {
        strm.avail_out = CHUNK;
        strm.next_out = out;
        status = inflate(&strm, Z_NO_FLUSH);
        assert(status != Z_STREAM_ERROR);     /*   state   not   clobbered   */
        switch (status) {
            case Z_NEED_DICT:
                status = Z_DATA_ERROR;           /*   and   fall   through   */
            case Z_DATA_ERROR:
            case Z_MEM_ERROR:
                inflateEnd(&strm);
                if (*dest != NULL) {
                    free(*dest);
                }
                *dest = NULL;
                return status;
        }

        have = CHUNK - strm.avail_out;
        totalsize += have;
#if 0
        fprintf(stderr, "avail_in %d avail_out %d total size %d\n", strm.avail_in, strm.avail_out, totalsize);
#endif
        *dest = (char *)realloc(*dest, totalsize + 1);
        memcpy(*dest + totalsize - have, out, have);
    } while ((Z_BUF_ERROR == status || (Z_OK == status && strm.avail_in)) && ++round < 100);

    if (status == Z_STREAM_END) {
        *(*dest + totalsize) = '\0';
    } else {
        if (*dest) {
            free(*dest);
            *dest = NULL;
        }
        /* HACK: See zlib/examples/zpipe.c inf() function for explanation. */
        /* This works as long as this function is not used for streaming. Required to catch very short invalid data. */
        status = (status == Z_OK) ? Z_DATA_ERROR : status;
    }

    (void)inflateEnd(&strm);

    return status == Z_STREAM_END ? Z_OK : Z_DATA_ERROR;
}