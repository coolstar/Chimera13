//
//  helpers.c
//  Odyssey
//
//  Created by tihmstar on 28.06.20.
//  Copyright © 2020 coolstar. All rights reserved.
//

#include "helpers.h"

#include <mach/mach.h>
#include <stdlib.h>
#include <stdio.h>

kern_return_t mach_vm_allocate(vm_map_t target, mach_vm_address_t *address, mach_vm_size_t size, int flags);
kern_return_t mach_vm_deallocate(mach_port_name_t target, mach_vm_address_t address, mach_vm_size_t size);

void suspend_all_threads() {
    thread_act_t other_thread, current_thread;
    unsigned int thread_count;
    thread_act_array_t thread_list;
    
    current_thread = mach_thread_self();
    int result = task_threads(mach_task_self(), &thread_list, &thread_count);
    if (result == -1) {
        exit(1);
    }
    if (!result && thread_count) {
        for (unsigned int i = 0; i < thread_count; ++i) {
            other_thread = thread_list[i];
            if (other_thread != current_thread) {
                thread_suspend(other_thread);
            }
        }
        mach_vm_deallocate(mach_task_self(), (mach_vm_address_t)thread_list, thread_count*sizeof(mach_port_t));
    }
}

void resume_all_threads() {
    thread_act_t other_thread, current_thread;
    unsigned int thread_count;
    thread_act_array_t thread_list;
    
    current_thread = mach_thread_self();
    int result = task_threads(mach_task_self(), &thread_list, &thread_count);
    if (result == -1) {
        exit(1);
    }
    if (!result && thread_count) {
        for (unsigned int i = 0; i < thread_count; ++i) {
            other_thread = thread_list[i];
            if (other_thread != current_thread) {
                thread_resume(other_thread);
            }
        }
        mach_vm_deallocate(mach_task_self(), (mach_vm_address_t)thread_list, thread_count*sizeof(mach_port_t));
    }
}
