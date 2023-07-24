#include "file.h"
#include <stdio.h>

void read_bytes_from_file(char * filename, byte * buffer, int buffer_size) {
    int num;
    FILE *read_ptr = fopen(filename,"rb");
    num = fread(buffer, sizeof(byte), buffer_size, read_ptr);
    fclose(read_ptr);
}

void write_bytes_to_file(char * filename, byte * buffer, int buffer_size) {
    int num;
    FILE *write_ptr = fopen(filename,"wb");
    num = fwrite(buffer, sizeof(byte), buffer_size, write_ptr);
    fclose(write_ptr);
}
