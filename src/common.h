#ifndef COMMON_H_
#define COMMON_H_

#include <stdint.h>

#define MAX(x, y) (((x) > (y)) ? (x) : (y))
#define MIN(x, y) (((x) < (y)) ? (x) : (y))

typedef uint8_t byte;
typedef uint16_t word;
typedef uint32_t dword;

typedef enum {FALSE, TRUE} bool;

#endif
