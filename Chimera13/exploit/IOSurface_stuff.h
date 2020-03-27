//
//  IOSurface_stuff.h
//  time_waste
//
//  Created by Jake James on 2/22/20.
//  Copyright © 2020 Jake James. All rights reserved.
//

#ifndef IOSurface_stuff_h
#define IOSurface_stuff_h

#import <stdio.h>
#import <stdlib.h>
#import <unistd.h>

#import "IOKitLib.h"
#import <mach/mach.h>

#import "offsets.h"

#define IOSurfaceRootUserClient_create_surface_selector 6
#define IOSurfaceRootUserClient_set_value_selector 9
#define IOSurfaceRootUserClient_get_value_selector 10
#define IOSurfaceRootUserClient_remove_value_selector 11
#define IOSurfaceRootUserClient_increment_use_count_selector 14
#define IOSurfaceRootUserClient_decrement_use_count_selector 15
#define IOSurfaceRootUserClient_set_notify_selector 17

struct IOSurfaceFastCreateArgs {
    uint64_t address;
    uint32_t width;
    uint32_t height;
    uint32_t pixel_format;
    uint32_t bytes_per_element;
    uint32_t bytes_per_row;
    uint32_t alloc_size;
};

struct IOSurfaceLockResult {
    uint8_t _pad1[0x18];
    uint32_t surface_id;
    uint8_t _pad2[0xdd0-0x18-0x4];
};

struct IOSurfaceValueArgs {
    uint32_t surface_id;
    uint32_t field_4;
    union {
        uint32_t binary[0];
        char xml[0];
    };
};

struct IOSurfaceValueResultArgs {
    uint32_t field_0;
};


enum {
    kOSSerializeDictionary          = 0x01000000U,
    kOSSerializeArray               = 0x02000000U,
    kOSSerializeSet                 = 0x03000000U,
    kOSSerializeNumber              = 0x04000000U,
    kOSSerializeSymbol              = 0x08000000U,
    kOSSerializeString              = 0x09000000U,
    kOSSerializeData                = 0x0a000000U,
    kOSSerializeBoolean             = 0x0b000000U,
    kOSSerializeObject              = 0x0c000000U,
    
    kOSSerializeTypeMask            = 0x7F000000U,
    kOSSerializeDataMask            = 0x00FFFFFFU,
    
    kOSSerializeEndCollection       = 0x80000000U,
    
    kOSSerializeBinarySignature     = 0x000000d3U,
};

int init_IOSurface(void);
void term_IOSurface(void);

int IOSurface_setValue(struct IOSurfaceValueArgs *args, size_t args_size);
int IOSurface_getValue(struct IOSurfaceValueArgs *args, int args_size, struct IOSurfaceValueArgs *output, size_t *out_size);
int IOSurface_removeValue(struct IOSurfaceValueArgs *args, size_t args_size);

int IOSurface_remove_property(uint32_t key);
int IOSurface_kalloc(void *data, uint32_t size, uint32_t kalloc_key);
int IOSurface_kalloc_spray(void *data, uint32_t size, int count, uint32_t kalloc_key);
int IOSurface_empty_kalloc(uint32_t size, uint32_t kalloc_key);

int IOSurface_kmem_alloc(void *data, uint32_t size, uint32_t kalloc_key);
int IOSurface_kmem_alloc_spray(void *data, uint32_t size, int count, uint32_t kalloc_key);

extern uint32_t pagesize;
extern io_connect_t IOSurfaceRoot;
extern io_service_t IOSurfaceRootUserClient;
extern uint32_t IOSurface_ID;

#endif /* IOSurface_stuff_h */
