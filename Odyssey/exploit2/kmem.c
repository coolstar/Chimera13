//
//  kmem.c
//  Odyssey
//
//  Created by CoolStar on 5/26/20.
//  Copyright © 2020 coolstar. All rights reserved.
//

#include "kmem.h"
#import <mach/mach.h>
#import <stdbool.h>

#define MAX_CHUNK_SIZE 0xFFF

mach_port_t tfpzero;

kern_return_t mach_vm_allocate(vm_map_t target, mach_vm_address_t *address, mach_vm_size_t size, int flags);
kern_return_t mach_vm_read_overwrite(vm_map_t target_task, mach_vm_address_t address, mach_vm_size_t size, mach_vm_address_t data, mach_vm_size_t *outsize);
kern_return_t mach_vm_write(vm_map_t target_task, mach_vm_address_t address, vm_offset_t data, mach_msg_type_number_t dataCnt);
kern_return_t mach_vm_deallocate(vm_map_t target, mach_vm_address_t address, mach_vm_size_t size);;
kern_return_t mach_vm_read(vm_map_t target_task, mach_vm_address_t address, mach_vm_size_t size, vm_offset_t *data, mach_msg_type_number_t *dataCnt);

#ifdef DEBUG
#define DEBUGLOG(fmt, args...) fprintf(stderr, fmt "\n", ##args);
#else
#define DEBUGLOG(fmt, args...)
#endif

size_t kread(uint64_t where, void *p, size_t size) {
    kern_return_t rv;
    size_t offset = 0;
    while (offset < size) {
        mach_vm_size_t sz, chunk = MAX_CHUNK_SIZE;
        if (chunk > size - offset) {
            chunk = size - offset;
        }
        rv = mach_vm_read_overwrite(tfpzero, where + offset, chunk, (mach_vm_address_t)p + offset, &sz);
        if (rv != KERN_SUCCESS || sz == 0) {
            DEBUGLOG("[e] error reading kernel @%p", (void *)(offset + where));
            break;
        }
        offset += sz;
    }
    return offset;
}

size_t kwrite(uint64_t where, const void *p, size_t size) {
    kern_return_t rv;
    size_t offset = 0;
    while (offset < size) {
        size_t chunk = MAX_CHUNK_SIZE;
        if (chunk > size - offset) {
            chunk = size - offset;
        }
        rv = mach_vm_write(tfpzero, where + offset, (mach_vm_offset_t)p + offset, (mach_msg_type_number_t)chunk);
        if (rv != KERN_SUCCESS) {
            DEBUGLOG("[e] error writing kernel @%p", (void *)(offset + where));
            break;
        }
        offset += chunk;
    }
    return offset;
}

uint64_t kalloc(vm_size_t size){
    mach_vm_address_t address = 0;
    mach_vm_allocate(tfpzero, (mach_vm_address_t *)&address, size, VM_FLAGS_ANYWHERE);
    return address;
}

void kfree(mach_vm_address_t address, vm_size_t size){
  mach_vm_deallocate(tfpzero, address, size);
}

uint32_t rk32(uint64_t kaddr) {
  uint32_t val = 0;
  kread(kaddr, &val, sizeof(val));
  return val;
}

uint64_t rk64(uint64_t kaddr) {
  uint64_t val = 0;
  kread(kaddr, &val, sizeof(val));
  return val;
}

void wk32(uint64_t kaddr, uint32_t val) {
  kwrite(kaddr, &val, sizeof(val));
}

void wk64(uint64_t kaddr, uint64_t val) {
  kwrite(kaddr, &val, sizeof(val));
}

unsigned long kstrlen(uint64_t string) {
    if (!string) return 0;
    
    unsigned long len = 0;
    char ch = 0;
    int i = 0;
    while (true) {
        kread(string + i, &ch, 1);
        if (!ch) break;
        len++;
        i++;
    }
    return len;
}
