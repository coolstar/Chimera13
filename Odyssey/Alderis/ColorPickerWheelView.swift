//
//  ColorPickerMapView.swift
//  Alderis
//
//  Created by Adam Demasi on 14/3/20.
//  Copyright © 2020 HASHBANG Productions. All rights reserved.
//

import UIKit

protocol ColorPickerWheelViewDelegate {
	func colorPickerWheelView(didSelectColor color: Color)
}

class ColorPickerWheelView: UIView {

	var delegate: ColorPickerWheelViewDelegate?

	var color: Color {
		didSet {
			updateColor()
		}
	}

	private var containerView: UIView!
	private var hueLayer: CAGradientLayer!
	private var saturationLayer: CALayer!
	private var saturationMask: CAGradientLayer!
	private var brightnessLayer: CALayer!
	private var selectionView: ColorPickerCircleView!
	private var selectionViewXConstraint: NSLayoutConstraint!
	private var selectionViewYConstraint: NSLayoutConstraint!
	private var selectionViewFingerDownConstraint: NSLayoutConstraint!

	private var isFingerDown = false
	private let touchDownFeedbackGenerator = UIImpactFeedbackGenerator(style: .medium)
	private let touchUpFeedbackGenerator = UIImpactFeedbackGenerator(style: .light)

	init(color: Color) {
		self.color = color
		super.init(frame: .zero)

		containerView = UIView()
		containerView.translatesAutoresizingMaskIntoConstraints = false
		containerView.clipsToBounds = true
		addSubview(containerView)

		containerView.addGestureRecognizer(UITapGestureRecognizer(target: self, action: #selector(gestureRecognizerFired(_:))))
		containerView.addGestureRecognizer(UILongPressGestureRecognizer(target: self, action: #selector(gestureRecognizerFired(_:))))
		let panGestureRecognizer = UIPanGestureRecognizer(target: self, action: #selector(gestureRecognizerFired(_:)))
		panGestureRecognizer.maximumNumberOfTouches = 1
		containerView.addGestureRecognizer(panGestureRecognizer)

		hueLayer = CAGradientLayer()
		if #available(iOS 12.0, *) {
			hueLayer.type = .conic
		} else {
			// TODO
		}
		let colors = [
			0, 20, 40, 60, 80, 100, 120, 140, 160, 180, 200, 220, 240, 260, 280, 300, 320, 340, 360
		].map { h in
			UIColor(hue: CGFloat(h) / 360, saturation: 1, brightness: 1, alpha: 1).cgColor
		}
		hueLayer.colors = colors
		hueLayer.startPoint = CGPoint(x: 0.5, y: 0.5)
		hueLayer.endPoint = CGPoint(x: 0.5, y: 0)
		hueLayer.transform = CATransform3DMakeRotation(0.5 * .pi, 0, 0, 1)
		hueLayer.allowsGroupOpacity = false
		containerView.layer.addSublayer(hueLayer)

		saturationLayer = CALayer()
		saturationLayer.backgroundColor = UIColor.white.cgColor
		saturationLayer.allowsGroupOpacity = false
		containerView.layer.addSublayer(saturationLayer)

		saturationMask = CAGradientLayer()
		saturationMask.type = .radial
		saturationMask.colors = [ UIColor.white.cgColor, UIColor.clear.cgColor ]
		saturationMask.locations = [ 0, 1 ]
		saturationMask.startPoint = CGPoint(x: 0.5, y: 0.5)
		saturationMask.endPoint = CGPoint(x: 1, y: 1)
		saturationMask.allowsGroupOpacity = false
		saturationLayer.mask = saturationMask

		brightnessLayer = CALayer()
		brightnessLayer.backgroundColor = UIColor.black.cgColor
		brightnessLayer.allowsGroupOpacity = false
		containerView.layer.addSublayer(brightnessLayer)

		selectionView = ColorPickerCircleView()
		selectionView.translatesAutoresizingMaskIntoConstraints = false
		containerView.addSubview(selectionView)

		selectionViewXConstraint = selectionView.leftAnchor.constraint(equalTo: containerView.leftAnchor)
		selectionViewYConstraint = selectionView.topAnchor.constraint(equalTo: containerView.topAnchor)
		// https://www.youtube.com/watch?v=Qs8kDiOwPBA
		selectionViewFingerDownConstraint = selectionView.widthAnchor.constraint(equalToConstant: 56)
		let selectionViewNormalConstraint = selectionView.widthAnchor.constraint(equalToConstant: 24)
		selectionViewNormalConstraint.priority = .defaultHigh

		NSLayoutConstraint.activate([
			containerView.centerXAnchor.constraint(equalTo: self.centerXAnchor),
			containerView.topAnchor.constraint(equalTo: self.topAnchor),
			containerView.bottomAnchor.constraint(equalTo: self.bottomAnchor),
			containerView.widthAnchor.constraint(equalTo: containerView.heightAnchor, constant: 30),
			selectionViewXConstraint,
			selectionViewYConstraint,
			selectionViewNormalConstraint,
			selectionView.heightAnchor.constraint(equalTo: selectionView.widthAnchor)
		])
	}

	required init?(coder: NSCoder) {
		fatalError("init(coder:) has not been implemented")
	}

	override func layoutSubviews() {
		super.layoutSubviews()

		let bounds = containerView.bounds.insetBy(dx: 30, dy: 15)
		let cornerRadius = bounds.size.height / 2
		hueLayer.frame = bounds
		saturationLayer.frame = bounds
		saturationMask.frame = saturationLayer.bounds
		brightnessLayer.frame = bounds
		hueLayer.cornerRadius = cornerRadius
		saturationLayer.cornerRadius = cornerRadius
		saturationMask.cornerRadius = cornerRadius
		brightnessLayer.cornerRadius = cornerRadius
		updateSelectionPoint()
	}

	private func updateColor() {
		brightnessLayer.opacity = Float(1 - color.brightness)
		selectionView.backgroundColor = color.uiColor
		updateSelectionPoint()
	}

	private func updateSelectionPoint() {
		let selectionPoint = pointForColor(color, in: hueLayer.frame.size)
		var fingerYOffset: CGFloat = 0
		if isFingerDown {
			fingerYOffset = selectionPoint.y < hueLayer.frame.size.height / 2 ? 40 : -40
		}
		selectionViewXConstraint.constant = hueLayer.frame.origin.x + selectionPoint.x - (selectionView.frame.size.width / 2)
		selectionViewYConstraint.constant = hueLayer.frame.origin.y + selectionPoint.y - (selectionView.frame.size.height / 2) + fingerYOffset
	}

	private func colorAt(position: CGPoint, in size: CGSize) -> Color {
		let x = (size.width / 2) - position.x
		let y = (size.height / 2) - position.y
		let h = 180 + round(atan2(y, x) * (180 / .pi))
		let handleRange = size.width / 2
		let handleDistance = min(sqrt(x * x + y * y), handleRange)
		let s = round(100 / handleRange * handleDistance)
		return Color(hue: h / 360, saturation: s / 100, brightness: color.brightness, alpha: 1)
	}

	private func pointForColor(_ color: Color, in size: CGSize) -> CGPoint {
		let handleRange = size.width / 2
		let handleAngle = (color.hue * 360) * (.pi / 180)
		let handleDistance = color.saturation * handleRange
		let x = (size.width / 2) + handleDistance * cos(handleAngle)
		let y = (size.height / 2) + handleDistance * sin(handleAngle)
		return CGPoint(x: x, y: y)
	}

	@objc private func gestureRecognizerFired(_ sender: UIGestureRecognizer) {
		switch sender.state {
		case .began, .changed, .ended:
			var location = sender.location(in: containerView)
			location.x -= hueLayer.frame.origin.x
			location.y -= hueLayer.frame.origin.y
			color = colorAt(position: location, in: hueLayer.frame.size)
			delegate?.colorPickerWheelView(didSelectColor: color)
		case .possible, .cancelled, .failed:
			break
		@unknown default:
			break
		}

		if sender is UITapGestureRecognizer {
			return
		}

		switch sender.state {
		case .began, .ended, .cancelled:
			isFingerDown = sender.state == .began
			selectionViewFingerDownConstraint.isActive = isFingerDown
			updateSelectionPoint()
			UIView.animate(withDuration: 0.2, animations: {
				self.containerView.layoutIfNeeded()
				self.updateSelectionPoint()
			}, completion: { _ in
				self.updateSelectionPoint()
			})
			if sender.state == .began {
				touchDownFeedbackGenerator.impactOccurred()
			} else {
				touchUpFeedbackGenerator.impactOccurred()
			}
		case .possible, .changed, .failed:
			break
		@unknown default:
			break
		}
	}

}
