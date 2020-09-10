//
//  LayoutGuide.swift
//  Alderis
//
//  Created by Adam Demasi on 12/3/20.
//  Copyright © 2020 HASHBANG Productions. All rights reserved.
//

import UIKit

protocol LayoutGuide {
	var leadingAnchor: NSLayoutXAxisAnchor { get }
	var trailingAnchor: NSLayoutXAxisAnchor { get }
	var leftAnchor: NSLayoutXAxisAnchor { get }
	var rightAnchor: NSLayoutXAxisAnchor { get }
	var topAnchor: NSLayoutYAxisAnchor { get }
	var bottomAnchor: NSLayoutYAxisAnchor { get }
	var widthAnchor: NSLayoutDimension { get }
	var heightAnchor: NSLayoutDimension { get }
	var centerXAnchor: NSLayoutXAxisAnchor { get }
	var centerYAnchor: NSLayoutYAxisAnchor { get }
}

extension UIView: LayoutGuide {}
extension UILayoutGuide: LayoutGuide {}
