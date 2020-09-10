//
//  ColorPickerDelegate.swift
//  Alderis
//
//  Created by Adam Demasi on 16/3/20.
//  Copyright © 2020 HASHBANG Productions. All rights reserved.
//

import UIKit

@objc(HBColorPickerDelegate)
public protocol ColorPickerDelegate: NSObjectProtocol {

	@objc(colorPicker:didSelectColor:)
	func colorPicker(_ colorPicker: ColorPickerViewController, didSelect color: UIColor)

	@objc(colorPickerDidCancel:)
	optional func colorPickerDidCancel(_ colorPicker: ColorPickerViewController)

}
