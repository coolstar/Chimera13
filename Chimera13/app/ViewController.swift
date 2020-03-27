//
//  ViewController.swift
//  Electra13
//
//  Created by CoolStar on 3/1/20.
//  Copyright © 2020 coolstar. All rights reserved.
//

import UIKit

class ViewController: UIViewController {

    override func viewDidLoad() {
        super.viewDidLoad()
        if (get_tfp0() != MACH_PORT_NULL){
            let our_task = rk64(task_self + Offsets.shared.ipc_port.ip_kobject)
            let electra = Electra(tfp0: tfpzero, any_proc: rk64(our_task + Offsets.shared.task.bsd_info), enable_tweaks: true, restore_rootfs: false, nonce: "")
            electra.jailbreak()
            // Do any additional setup after loading the view.
        } else {
            print("Failed to init jailbreak!");
        }
        // Do any additional setup after loading the view.
    }


}

