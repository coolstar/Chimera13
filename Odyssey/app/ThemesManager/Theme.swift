//
//  File.swift
//  Odyssey
//
//  Created by CoolStar on 7/6/20.
//  Copyright © 2020 coolstar. All rights reserved.
//

import UIKit

class Theme {
    let colorViewBackgrounds: [AnimatingColourView.GradientBackground]
    let backgroundImage: UIImage?
    let backgroundCenter: CGPoint
    let backgroundOverlay: UIColor?
    let enableBlur: Bool
    let copyrightString: String
    
    init(colorViewBackgrounds: [AnimatingColourView.GradientBackground],
         backgroundImage: UIImage?,
         backgroundCenter: CGPoint = .zero,
         backgroundOverlay: UIColor?,
         enableBlur: Bool,
         copyrightString: String = "") {
        self.colorViewBackgrounds = colorViewBackgrounds
        self.backgroundImage = backgroundImage
        self.backgroundCenter = backgroundCenter
        self.backgroundOverlay = backgroundOverlay
        self.enableBlur = enableBlur
        self.copyrightString = copyrightString
    }
}
