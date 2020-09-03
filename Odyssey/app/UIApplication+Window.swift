//
//  UIApplication+Window.swift
//  Odyssey
//
//  Created by CoolStar on 7/3/20.
//  Copyright © 2020 coolstar. All rights reserved.
//

import UIKit

extension UIApplication {
    var currentWindow: UIWindow? {
        connectedScenes
            .filter { $0.activationState == .foregroundActive }
            .compactMap { $0 as? UIWindowScene }
            .first?.windows
            .first(where: { $0.isKeyWindow })
    }
}
