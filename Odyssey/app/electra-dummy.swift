//
//  electra-dummy.swift
//  OdysseyUI
//
//  Created by CoolStar on 7/25/20.
//  Copyright © 2020 coolstar. All rights reserved.
//

import Foundation

enum JAILBREAK_RETURN_STATUS {
    case ERR_NOERR
    case ERR_VERSION
    case ERR_EXPLOIT
    case ERR_UNSUPPORED
    case ERR_TFP0
    case ERR_ALREADY_JAILBROKEN
    case ERR_ROOTFS_RESTORE
    case ERR_REMOUNT
    case ERR_SNAPSHOT
    case ERR_JAILBREAK
    case ERR_CONFLICT
}

protocol ElectraUI {
    
}

class Electra {
    public var ui: ElectraUI
    public var tfp0: mach_port_t
    public var any_proc: UInt64
    public var enable_tweaks: Bool
    public var restore_rootfs: Bool
    public var nonce: String
    
    init(ui: ElectraUI,
         tfp0: mach_port_t,
         any_proc: UInt64,
         enable_tweaks: Bool,
         restore_rootfs: Bool,
         nonce: String) {
        self.ui = ui
        self.tfp0 = tfp0
        self.any_proc = any_proc
        self.enable_tweaks = enable_tweaks
        self.restore_rootfs = restore_rootfs
        self.nonce = nonce
    }
    
    public func jailbreak() -> JAILBREAK_RETURN_STATUS {
        .ERR_JAILBREAK
    }
}

var tfpzero: mach_port_t = mach_port_t(MACH_PORT_NULL)
var task_self: UInt64 = 0

func isArm64e() -> Bool {
    false
}

func tardy0n() {
    
}

func get_tfp0() {
    
}

func getTaskPort() -> mach_port_t {
    tfpzero
}

func getOurTask() -> UInt64 {
    0
}

func rk64(_ addr: UInt64) -> UInt64 {
    0
}

func csops(_ pid: pid_t, _ status: UInt32, _ flags: inout UInt32, _ size: size_t) {
    
}

func ObjcTryCatch(_ closure:() -> Void) {
    closure()
}

func isGeneratorValid(generator: String) -> Bool {
    let generatorInput = generator.cString(using: .utf8)
    var rawGeneratorValue = UInt64(0)
    var generatorString = [Int8](repeating: 0, count: 22)
    
    var retval = false
    withUnsafeMutablePointer(to: &rawGeneratorValue) { rawGeneratorValuePtr in
        let args: [CVarArg] = [rawGeneratorValuePtr]
        vsscanf(generatorInput, "0x%16llx", getVaList(args))
        _ = snprintf(ptr: &generatorString, 22, "0x%016llx", rawGeneratorValuePtr.pointee)
        retval = (strcmp(generatorString, generatorInput) == 0)
    }
    return retval
}
