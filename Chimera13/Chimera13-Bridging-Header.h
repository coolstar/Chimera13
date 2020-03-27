//
//  Use this file to import your target's public headers that you would like to expose to Swift.
//

#import <mach/mach.h>
#import "mach_vm.h"
#import "kernel_memory.h"
#import "nvramutils.h"
#include "iokit.h"

kern_return_t
IORegistryEntrySetCFProperty(io_registry_entry_t, CFStringRef, CFTypeRef);

int get_tfp0();
extern mach_port_t tfpzero;
extern uint64_t task_self;
