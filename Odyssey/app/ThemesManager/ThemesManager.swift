//
//  ThemesManager.swift
//  Odyssey
//
//  Created by CoolStar on 7/6/20.
//  Copyright © 2020 coolstar. All rights reserved.
//

import UIKit

class ThemesManager {
    static let shared = ThemesManager()
    static let themeChangeNotification = NSNotification.Name("ThemeChangeNotification")
    
    private let themes: [String: Theme] = [
        "default": Theme(colorViewBackgrounds: [
            .init(baseColour: .black, linearGradients: [
                .init(colours: [UIColor(red: 210/255, green: 135/255, blue: 244/255, alpha: 1), UIColor(red: 247/255, green: 107/255, blue: 28/255, alpha: 1)], angle: 47)
            ], overlayImage: nil)
        ], backgroundImage: nil, backgroundOverlay: nil, enableBlur: false),
        
        "dark": Theme(colorViewBackgrounds: [
            .init(baseColour: .black, linearGradients: [
                .init(colours: [UIColor(red: 54/255, green: 17/255, blue: 113/255, alpha: 1), UIColor(red: 117/255, green: 28/255, blue: 0/255, alpha: 1)], angle: 47)
            ], overlayImage: nil)
        ], backgroundImage: nil, backgroundOverlay: nil, enableBlur: false),
        
        "meridianDark": Theme(colorViewBackgrounds: [
            .init(baseColour: .black, linearGradients: [
                .init(colours: [UIColor(red: 25/255, green: 25/255, blue: 25/255, alpha: 1), UIColor(red: 227/255, green: 1/255, blue: 37/255, alpha: 1)], angle: 47)
            ], overlayImage: nil)
        ], backgroundImage: nil,
           backgroundOverlay: nil,
           enableBlur: false),

        "azurLane": Theme(
            colorViewBackgrounds: [
                .init(baseColour: .black, linearGradients: [
                    .init(colours: [.black], angle: 0)
                ], overlayImage: nil)
            ],
            backgroundImage: UIImage(named: "azurLane"),
            backgroundCenter: CGPoint(x: 1510, y: 800),
            backgroundOverlay: UIColor(white: 0, alpha: 0.3),
            enableBlur: true,
            copyrightString: "Neptune and Monarch (Azur Lane)\nWallpaper © 2019, Zolaida\nWallpaper used under CC BY-NC-ND 3.0 license\nOriginal wallpaper from DeviantArt"),
        
        "pokemon": Theme(
            colorViewBackgrounds: [
                .init(baseColour: .black, linearGradients: [
                    .init(colours: [.black], angle: 0)
                ], overlayImage: nil)
            ],
            backgroundImage: UIImage(named: "pokemon"),
            backgroundCenter: CGPoint(x: 720, y: 720),
            backgroundOverlay: UIColor(white: 0, alpha: 0.3),
            enableBlur: true,
            copyrightString: "Misty (Pokemon)\nWallpaper © 2016, Zolaida\nWallpaper used under CC BY-NC-ND 3.0 license\nOriginal wallpaper from DeviantArt"),
        
        "overwatch": Theme(
            colorViewBackgrounds: [
                .init(baseColour: .black, linearGradients: [
                    .init(colours: [.black], angle: 0)
                ], overlayImage: nil)
            ],
            backgroundImage: UIImage(named: "overwatch"),
            backgroundCenter: CGPoint(x: 600, y: 480),
            backgroundOverlay: UIColor(white: 1, alpha: 0.1),
            enableBlur: true,
            copyrightString: "D.va n' Lucio (Overwatch)\nWallpaper © 2017, raikoart\nWallpaper used under CC BY-NC-ND 3.0 license\nOriginal wallpaper from DeviantArt"),
        
        "league": Theme(
            colorViewBackgrounds: [
                .init(baseColour: .black, linearGradients: [
                    .init(colours: [.black], angle: 0)
                ], overlayImage: nil)
            ],
            backgroundImage: UIImage(named: "league"),
            backgroundCenter: CGPoint(x: 750, y: 540),
            backgroundOverlay: UIColor(white: 0, alpha: 0.1),
            enableBlur: true,
            copyrightString: "Lux [Star Guardian] (League of Legends)\nWallpaper © 2016, Liang-Xing\nWallpaper used under CC BY-NC-ND 3.0 license\nOriginal wallpaper from DeviantArt"),
    
        "Sierra": Theme(
            colorViewBackgrounds: [
                .init(baseColour: .black, linearGradients: [
                    .init(colours: [.black], angle: 0)
                ], overlayImage: nil)
            ],
            backgroundImage: UIImage(named: "Sierra"),
            backgroundCenter: CGPoint(x: 1440, y: 900),
            backgroundOverlay: UIColor(white: 0, alpha: 0.1),
            enableBlur: true)
    ]
    
    public var currentTheme: Theme {
        let currentThemeName = UserDefaults.standard.string(forKey: "theme") ?? "default"
        return themes[currentThemeName] ?? themes["default"]!
    }
    
    init() {
        if UserDefaults.standard.string(forKey: "theme") == nil {
            UserDefaults.standard.set("default", forKey: "theme")
        }
    }
}
