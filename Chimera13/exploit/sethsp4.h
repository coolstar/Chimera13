//
//  sethsp4.h
//  Elysian
//
//  Created by chris  on 4/2/20.
//  Copyright © 2020 chr1s_0x1. All rights reserved.
//

#ifndef sethsp4_h
#define sethsp4_h

#include <stdio.h>

#define io_makebits(active, otype, kotype)    \
(((active) ? IO_BITS_ACTIVE : 0) | ((otype) << 16) |  (kotype))

#define    IOT_PORT        0

#define    IKOT_HOST_PRIV                4

int set_tfp0_hsp4(mach_port_t tfp0);
#endif /* sethsp4_h */
