#ifndef __PTL_H__
#define __PTL_H__
#include "tl_common.h"
#include "drivers.h"

enum {
    PTL_STA_VERIFY = 0,
    PTL_STA_NORMAL,
    PTL_STA_MAX,
};

void ptl_init(void);
void ptl_proc(void);
u8 ptl_get_sta(void);

#endif
