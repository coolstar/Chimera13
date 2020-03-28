//
//  Use this file to import your target's public headers that you would like to expose to Swift.
//

#import <mach/mach.h>
#import <sys/mount.h>
#import <sys/stat.h>
#import <sys/snapshot.h>
#import "mach_vm.h"
#import "kernel_memory.h"
#import "nvramutils.h"
#include "iokit.h"

kern_return_t
IORegistryEntrySetCFProperty(io_registry_entry_t, CFStringRef, CFTypeRef);

int get_tfp0();
extern mach_port_t tfpzero;
extern uint64_t task_self;

struct hfs_mount_args {
    char    *fspec;            /* block special device to mount */
    uid_t    hfs_uid;        /* uid that owns hfs files (standard HFS only) */
    gid_t    hfs_gid;        /* gid that owns hfs files (standard HFS only) */
    mode_t    hfs_mask;        /* mask to be applied for hfs perms  (standard HFS only) */
    u_int32_t hfs_encoding;    /* encoding for this volume (standard HFS only) */
    struct    timezone hfs_timezone;    /* user time zone info (standard HFS only) */
    int        flags;            /* mounting flags, see below */
    int     journal_tbuffer_size;   /* size in bytes of the journal transaction buffer */
    int        journal_flags;          /* flags to pass to journal_open/create */
    int        journal_disable;        /* don't use journaling (potentially dangerous) */
};
