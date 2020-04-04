#import <mach/mach.h>
#include "cutils.h"
#include "kmem.h"

#pragma pack(4)
typedef struct {
  mach_msg_header_t Head;
  mach_msg_body_t msgh_body;
  mach_msg_port_descriptor_t thread;
  mach_msg_port_descriptor_t task;
  NDR_record_t NDR;
} exception_raise_request; // the bits we need at least

typedef struct {
  mach_msg_header_t Head;
  NDR_record_t NDR;
  kern_return_t RetCode;
} exception_raise_reply;
#pragma pack()

extern mach_port_t tfpzero;

kern_return_t mach_vm_write(vm_map_t target_task, mach_vm_address_t address, const uint8_t *data, mach_msg_type_number_t dataCnt);
kern_return_t mach_vm_region(vm_map_t, mach_vm_address_t *, mach_vm_size_t *, vm_region_flavor_t, vm_region_info_t, mach_msg_type_number_t *, mach_port_t *);
kern_return_t mach_vm_read_overwrite(vm_map_t target_task, mach_vm_address_t address, mach_vm_size_t size, uint8_t *data, mach_vm_size_t *outsize);

int memorystatus_control(uint32_t comand, pid_t pid, uint32_t flags, void *buffer, size_t buffersize);
