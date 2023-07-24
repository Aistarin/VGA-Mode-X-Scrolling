#ifndef FILE_H_
#define FILE_H_

#include "src/common.h"

void read_bytes_from_file(char * filename, byte * buffer, int buffer_size);
void write_bytes_to_file(char * filename, byte * buffer, int buffer_size);

#endif
