#ifndef BASEDEF_H
#define BASEDEF_H

#include <stdio.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <mqueue.h>

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif /* __cplusplus */


typedef unsigned char boolean; /* bool value, 0-false, 1-true       */
typedef unsigned char u8; /* Unsigned  8 bit quantity          */
typedef char s8; /* Signed    8 bit quantity          */
typedef unsigned short u16; /* Unsigned 16 bit quantity          */
typedef signed short s16; /* Signed   16 bit quantity          */
typedef unsigned int u32; /* Unsigned 32 bit quantity          */
typedef signed int s32; /* Signed   32 bit quantity          */
typedef unsigned long long u64; /* Unsigned 64 bit quantity   	   */
typedef signed long long s64; /* Unsigned 64 bit quantity          */
typedef float fp32; /* Single precision floating point   */
typedef double fp64; /* Double precision floating point   */

#define UINT8	u8
#define UINT32	u32
#define INT32 	s32





#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* __cplusplus */

#endif // BASEDEF_H
