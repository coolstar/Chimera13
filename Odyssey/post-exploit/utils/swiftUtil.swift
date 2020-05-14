//
//  swiftUtil.swift
//  Odyssey
//
//  Created by CoolStar on 4/3/20.
//  Copyright © 2020 coolstar. All rights reserved.
//

import Foundation

extension FileHandle: TextOutputStream {
    public func write(_ string: String) {
        guard let data = string.data(using: .utf8) else { return }
        self.write(data)
    }
}
