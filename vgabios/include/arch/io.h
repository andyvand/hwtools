#ifndef _ASM_IO_H
#define _ASM_IO_H

#ifndef __APPLE__
#include <sys/io.h>
#else
#include <sys/ioctl.h>
#include "../../DirectHW.h"

#define u8 __u8
#define u16 __u16
#define u32 __u32
#define u64 __u64

#define s8 __s8
#define s16 __s16
#define s32 __s32
#define s64 __s64

#ifndef __u8
#define __u8 unsigned char
#endif

#ifndef __u16
#define __u16 unsigned short
#endif

#ifndef __u32
#define __u32 unsigned int
#endif

#ifndef __u64
#define __u64 unsigned long long
#endif

#ifndef __s8
#define __s8 char
#endif

#ifndef __s16
#define __s16 short
#endif

#ifndef __s32
#define __s32 long
#endif

#ifndef __s64
#define __s64 long long
#endif

#endif
#endif
