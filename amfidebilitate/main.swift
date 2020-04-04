import CoreFoundation

let err = host_get_special_port(mach_host_self(), HOST_LOCAL_NODE, 4, &tfpzero)
guard err == KERN_SUCCESS else {
    print("Unable to get tfp0")
    exit(5)
}

guard let allProcCStr = getenv("allProc") else {
    print("Unable to get allproc")
    exit(5)
}

let allProc = strtoull(allProcCStr, nil, 16)

let electra = Electra(tfp0: tfpzero, all_proc: allProc)
electra.populate_procs() //gets us TF_PLATFORM

let amfidtakeover = AmfidTakeover(electra: electra)
guard amfidtakeover.grabEntitlements(our_proc: electra.our_proc) else {
    print("Unable to grab entitlements")
    exit(5)
}
amfidtakeover.takeoverAmfid(amfid_pid: electra.amfid_pid)
amfidtakeover.resetEntitlements(our_proc: electra.our_proc)

CFRunLoopRun()
