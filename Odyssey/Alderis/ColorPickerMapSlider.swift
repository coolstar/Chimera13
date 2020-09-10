//
//  ColorPickerMapSlider.swift
//  Alderis
//
//  Created by Kabir Oberai on 23/03/20.
//  Copyright © 2020 HASHBANG Productions. All rights reserved.
//

import UIKit

class ColorPickerMapSlider: ColorPickerComponentSlider {

	init(minImageName: String, maxImageName: String, component: Color.Component, overrideSmartInvert: Bool) {
		super.init(component: component, overrideSmartInvert: overrideSmartInvert)

		stackView.alignment = .center
		stackView.spacing = 10

		let minImage: UIImage
		let maxImage: UIImage
		let imageTintColor: UIColor
		if #available(iOS 13, *) {
			minImage = UIImage(systemName: minImageName)!
			maxImage = UIImage(systemName: maxImageName)!
			imageTintColor = .secondaryLabel
		} else {
			let bundle = Bundle(for: Self.self)
			minImage = UIImage(named: minImageName, in: bundle, compatibleWith: nil)!
			maxImage = UIImage(named: maxImageName, in: bundle, compatibleWith: nil)!
			imageTintColor = UIColor(white: 60 / 255, alpha: 0.6)
		}

		let leftImageView = UIImageView(image: minImage)
		leftImageView.translatesAutoresizingMaskIntoConstraints = false
		leftImageView.contentMode = .scaleAspectFit
		leftImageView.tintColor = imageTintColor
		stackView.insertArrangedSubview(leftImageView, at: 0)

		let rightImageView = UIImageView(image: maxImage)
		rightImageView.translatesAutoresizingMaskIntoConstraints = false
		rightImageView.contentMode = .scaleAspectFit
		rightImageView.tintColor = imageTintColor
		stackView.addArrangedSubview(rightImageView)

		NSLayoutConstraint.activate([
			leftImageView.widthAnchor.constraint(equalToConstant: 22),
			leftImageView.widthAnchor.constraint(equalTo: rightImageView.widthAnchor),
			leftImageView.heightAnchor.constraint(equalTo: leftImageView.widthAnchor),
			rightImageView.heightAnchor.constraint(equalTo: rightImageView.widthAnchor)
		])
	}

	required init?(coder: NSCoder) {
		fatalError("init(coder:) has not been implemented")
	}

}
