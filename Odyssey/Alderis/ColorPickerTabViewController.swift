//
//  ColorPickerTabViewController.swift
//  Alderis
//
//  Created by Kabir Oberai on 23/03/20.
//  Copyright © 2020 HASHBANG Productions. All rights reserved.
//

import UIKit

protocol ColorPickerTabDelegate: class {
	func colorPickerTab(_ tab: ColorPickerTabViewControllerBase, didSelect color: Color)
}

class ColorPickerTabViewControllerBase: UIViewController {

	unowned var tabDelegate: ColorPickerTabDelegate

	var overrideSmartInvert: Bool

	private(set) var color: Color {
		didSet {
			colorDidChange()
		}
	}

	func colorDidChange() {}

	func setColor(_ color: Color, shouldBroadcast: Bool = true) {
		self.color = color
		if shouldBroadcast {
			tabDelegate.colorPickerTab(self, didSelect: color)
		}
	}

	required init?(coder: NSCoder) {
		fatalError("init(coder:) has not been implemented")
	}

	required init(tabDelegate: ColorPickerTabDelegate, overrideSmartInvert: Bool, color: Color) {
		self.tabDelegate = tabDelegate
		self.overrideSmartInvert = overrideSmartInvert
		self.color = color
		super.init(nibName: nil, bundle: nil)
	}

}

protocol ColorPickerTabViewControllerProtocol: ColorPickerTabViewControllerBase {
	static var imageName: String { get }
	static var image: UIImage { get }
}
extension ColorPickerTabViewControllerProtocol {
	static var image: UIImage {
		if #available(iOS 13, *) {
			return UIImage(systemName: imageName)!
		} else {
			let bundle = Bundle(for: self)
			return UIImage(named: imageName, in: bundle, compatibleWith: nil)!
		}
	}
}

typealias ColorPickerTabViewController = ColorPickerTabViewControllerBase & ColorPickerTabViewControllerProtocol
