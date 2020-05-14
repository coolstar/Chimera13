//
//  tardy0n.h
//  tardy0n
//
//  Created by tihmstar on 19.06.20.
//  Copyright © 2020 tihmstar. All rights reserved.
//

#ifndef tardy0n_h
#define tardy0n_h

#include <stdio.h>
#include <mach/mach.h>

#ifdef __cplusplus
extern "C"
#endif
uint64_t getOurTask(void);

#ifdef __cplusplus
extern "C"
#endif
mach_port_t getTaskPort(void);

#ifdef __cplusplus
extern "C"
#endif
int tardy0n(void );



#endif /* tardy0n_h */
