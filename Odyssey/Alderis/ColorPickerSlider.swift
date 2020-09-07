//
//  ColorPickerSlider.swift
//  Alderis
//
//  Created by Kabir Oberai on 28/03/20.
//  Copyright © 2020 HASHBANG Productions. All rights reserved.
//

import UIKit

class ColorPickerSliderBase: UIControl {

	var overrideSmartInvert: Bool {
		didSet {
			slider.accessibilityIgnoresInvertColors = overrideSmartInvert
		}
	}

	let stackView: UIStackView
	let slider: UISlider

	var value: CGFloat {
		get {
			CGFloat(slider.value)
		}
		set {
			slider.value = Float(newValue)
		}
	}

	init(overrideSmartInvert: Bool) {
		self.overrideSmartInvert = overrideSmartInvert

		slider = UISlider()
		slider.translatesAutoresizingMaskIntoConstraints = false
		slider.accessibilityIgnoresInvertColors = overrideSmartInvert

		stackView = UIStackView(arrangedSubviews: [ slider ])
		stackView.translatesAutoresizingMaskIntoConstraints = false
		stackView.axis = .horizontal
		stackView.distribution = .fill

		super.init(frame: .zero)

		slider.addTarget(self, action: #selector(sliderChanged), for: .valueChanged)
		addSubview(stackView)

		NSLayoutConstraint.activate([
			stackView.leadingAnchor.constraint(equalTo: self.leadingAnchor),
			stackView.trailingAnchor.constraint(equalTo: self.trailingAnchor),
			stackView.topAnchor.constraint(equalTo: self.topAnchor),
			stackView.bottomAnchor.constraint(equalTo: self.bottomAnchor)
		])
	}

	required init?(coder: NSCoder) {
		fatalError("init(coder:) has not been implemented")
	}

	@objc func sliderChanged() {
		sendActions(for: .valueChanged)
	}

}

protocol ColorPickerSliderProtocol: ColorPickerSliderBase {
	func setColor(_ color: Color)
	func apply(to color: inout Color)
}

typealias ColorPickerSlider = ColorPickerSliderBase & ColorPickerSliderProtocol

class ColorPickerComponentSlider: ColorPickerSlider {

	let component: Color.Component

	init(component: Color.Component, overrideSmartInvert: Bool) {
		self.component = component
		super.init(overrideSmartInvert: overrideSmartInvert)
	}

	required init?(coder: NSCoder) {
		fatalError("init(coder:) has not been implemented")
	}

	func setColor(_ color: Color) {
		value = color[keyPath: component.keyPath]
		slider.tintColor = component.sliderTintColor(for: color).uiColor
	}

	func apply(to color: inout Color) {
		color[keyPath: component.keyPath] = value
	}

}
