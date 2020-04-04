//
//  ViewController.swift
//  Electra13
//
//  Created by CoolStar on 3/1/20.
//  Copyright © 2020 coolstar. All rights reserved.
//

import UIKit

class ViewController: UIViewController {
    var electra: Electra?
    @IBOutlet var jailbreakButton: UIButton?
    
    override func viewDidLoad() {
        super.viewDidLoad()

        // Do any additional setup after loading the view.
    }
    
    @IBAction func jailbreak() {
        jailbreakButton?.isEnabled = false
        
        DispatchQueue.global(qos: .default).async {
            get_tfp0()
            let our_task = rk64(task_self + Offsets.shared.ipc_port.ip_kobject)
            let electra = Electra(tfp0: tfpzero,
                                  any_proc: rk64(our_task + Offsets.shared.task.bsd_info),
                                  enable_tweaks: true,
                                  restore_rootfs: false,
                                  nonce: "0xbd34a880be0b53f3")
            self.electra = electra
            let err = electra.jailbreak()
            if err == .ERR_NOERR {
                DispatchQueue.main.async {
                    let controller = UIAlertController(title: "Jailbroken", message: "SSH is running! Enjoy", preferredStyle: .alert)
                    controller.addAction(UIAlertAction(title: "Exit", style: .default, handler: { _ in
                        controller.dismiss(animated: true) {
                            UIApplication.shared.beginBackgroundTask {
                                print("odd. this should never be called.")
                            }
                            UIApplication.shared.suspend()
                            DispatchQueue.main.asyncAfter(deadline: .now() + .seconds(1)) {
                                exit(0)
                            }
                        }
                    }))
                    self.present(controller, animated: true, completion: nil)
                }
            }
        }
    }
}
