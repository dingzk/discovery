//
// Created by zhenkai on 2020/7/11.
//

#ifndef DISCOVERY_GZIP_H
#define DISCOVERY_GZIP_H

#include "compressor/zlib/zlib.h"

int  gzdecode(unsigned char *source, int len, char **dest);


#endif //DISCOVERY_GZIP_H
