import Foundation

func getKqueueForPid(pid: pid_t) -> Int32 {
    let kq = kqueue()
    guard kq != -1 else {
        print("Unable to create kqueue")
        return -1
    }
    
    var ke = kevent()

    ke.ident = UInt(pid)
    ke.filter = Int16(EVFILT_PROC)
    ke.flags = UInt16(EV_ADD)
    ke.fflags = UInt32(NOTE_EXIT_DETAIL)
    ke.data = 0
    ke.udata = nil

    let rc = kevent(kq, &ke, 1, nil, 0, nil)
    guard rc >= 0 else {
        print("Unable to get kevent")
        return -1
    }
    return kq
}

var standardError = FileHandle.standardError

func startAmfid() {
    let dict = xpc_dictionary_create(nil, nil, 0)
    
    xpc_dictionary_set_uint64(dict, "subsystem", 3)
    xpc_dictionary_set_uint64(dict, "handle", UInt64(HANDLE_SYSTEM))
    xpc_dictionary_set_uint64(dict, "routine", UInt64(ROUTINE_START))
    xpc_dictionary_set_uint64(dict, "type", 1)
    xpc_dictionary_set_string(dict, "name", "com.apple.MobileFileIntegrity")
    
    var outDict: xpc_object_t?
    let rc = xpc_pipe_routine(xpc_bootstrap_pipe(), dict, &outDict)
    if rc == 0,
        let outDict = outDict {
        let rc2 = Int32(xpc_dictionary_get_int64(outDict, "error"))
        if rc2 != 0 {
            print(String(format: "Error starting service: %s", xpc_strerror(rc2)), to: &standardError)
            return
        }
    } else if rc != 0 {
        print(String(format: "Error starting service (no outdict): %s", xpc_strerror(rc)), to: &standardError)
        return
    }
}

let MEMORYSTATUS_CMD_SET_JETSAM_TASK_LIMIT = UInt32(6)
memorystatus_control(MEMORYSTATUS_CMD_SET_JETSAM_TASK_LIMIT, getpid(), 0, nil, 0)

let err = host_get_special_port(mach_host_self(), HOST_LOCAL_NODE, 4, &tfpzero)
guard err == KERN_SUCCESS else {
    print("Unable to get tfp0", to: &standardError)
    exit(5)
}

guard let allProcCStr = getenv("allProc") else {
    print("Unable to get allproc", to: &standardError)
    exit(5)
}

let allProc = strtoull(allProcCStr, nil, 16)

let electra = Electra(tfp0: tfpzero, all_proc: allProc)

while true {
    print("Searching for amfid...", to: &standardError)
    let last_amfidPid = electra.amfid_pid
    while electra.amfid_pid == last_amfidPid {
        electra.populate_procs() //gets us TF_PLATFORM
        if electra.amfid_pid != last_amfidPid {
            break
        }
        sleep(1)
        print("Asking launchd to restart amfid before we continue...", to: &standardError)
        startAmfid()
    }
    print("Found amfid: ", electra.amfid_pid, to: &standardError)

    let amfidtakeover = AmfidTakeover(electra: electra)
    guard amfidtakeover.grabEntitlements(our_proc: electra.our_proc) else {
        print("Unable to grab entitlements", to: &standardError)
        exit(5)
    }
    amfidtakeover.takeoverAmfid(amfid_pid: electra.amfid_pid)
    amfidtakeover.resetEntitlements(our_proc: electra.our_proc)
    
    let kq = getKqueueForPid(pid: pid_t(electra.amfid_pid))
    var ke = kevent()
    let rc = kevent(kq, nil, 0, &ke, 1, nil)
    if rc > 0 {
        close(kq)
        
        print("amfid dead...", to: &standardError)
        amfidtakeover.cleanupAmfidTakeover()
        
        print("Asking launchd to restart amfid before we continue...", to: &standardError)
        startAmfid()
    }
}
