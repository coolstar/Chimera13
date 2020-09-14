//
//  ColorPickerViewController.swift
//  Alderis
//
//  Created by Adam Demasi on 12/3/20.
//  Copyright © 2020 HASHBANG Productions. All rights reserved.
//

import UIKit

@objc(HBColorPickerTab)
public enum ColorPickerTab: Int, CaseIterable {
	case swatch
	case map
	case sliders
}

@objc(HBColorPickerViewController)
open class ColorPickerViewController: UIViewController {

	@objc static let defaultColor = UIColor(white: 0.6, alpha: 1)

	@objc open weak var delegate: ColorPickerDelegate? {
		didSet {
			innerViewController?.delegate = delegate
		}
	}
	@objc open var overrideSmartInvert = true {
		didSet {
			innerViewController?.overrideSmartInvert = overrideSmartInvert
		}
	}
	@objc open var color = ColorPickerViewController.defaultColor {
		didSet {
			innerViewController?.color = Color(uiColor: color)
		}
	}

	// A width divisible by 12 (the number of items wide in the swatch).
	var finalWidth: CGFloat {
		floor(min(384, view.frame.size.width - 30) / 12) * 12
	}

	private var innerViewController: ColorPickerInnerViewController!
	private var backgroundView: UIVisualEffectView!
	private var widthLayoutConstraint: NSLayoutConstraint!
	private var bottomLayoutConstraint: NSLayoutConstraint!

	override init(nibName nibNameOrNil: String?, bundle nibBundleOrNil: Bundle?) {
		super.init(nibName: nil, bundle: nil)
		modalPresentationStyle = .overCurrentContext
	}

	required public init?(coder: NSCoder) {
		fatalError("init(coder:) has not been implemented")
	}

	override open func viewDidLoad() {
		super.viewDidLoad()

		navigationController?.isNavigationBarHidden = true
		view.backgroundColor = UIColor(white: 0, alpha: 0.2)

		let containerView = UIView()
		containerView.translatesAutoresizingMaskIntoConstraints = false
		view.addSubview(containerView)

		backgroundView = UIVisualEffectView(effect: UIBlurEffect(style: .light))
		backgroundView.translatesAutoresizingMaskIntoConstraints = false
		backgroundView.clipsToBounds = true
		if #available(iOS 13, *) {
			backgroundView.layer.cornerRadius = 13
			backgroundView.layer.cornerCurve = .continuous
		} else {
			backgroundView._continuousCornerRadius = 13
		}
		containerView.addSubview(backgroundView)
        
		let color = Color(uiColor: self.color)
        innerViewController = ColorPickerInnerViewController(delegate: delegate, overrideSmartInvert: overrideSmartInvert, color: color)
		innerViewController.willMove(toParent: self)
		addChild(innerViewController)
		innerViewController.view.translatesAutoresizingMaskIntoConstraints = false
		innerViewController.view.clipsToBounds = true
		if #available(iOS 13, *) {
			innerViewController.view.layer.cornerRadius = 13
			innerViewController.view.layer.cornerCurve = .continuous
		} else {
			innerViewController.view._continuousCornerRadius = 13
		}
		containerView.addSubview(innerViewController.view)

		let layoutGuide: LayoutGuide
		if #available(iOS 11, *) {
			layoutGuide = view.safeAreaLayoutGuide
		} else {
			layoutGuide = view
		}

		widthLayoutConstraint = containerView.widthAnchor.constraint(equalToConstant: finalWidth)
		bottomLayoutConstraint = layoutGuide.bottomAnchor.constraint(equalTo: containerView.bottomAnchor, constant: 15)

		NSLayoutConstraint.activate([
			containerView.centerXAnchor.constraint(equalTo: layoutGuide.centerXAnchor),
			widthLayoutConstraint,
			bottomLayoutConstraint,
			backgroundView.leadingAnchor.constraint(equalTo: containerView.leadingAnchor),
			backgroundView.trailingAnchor.constraint(equalTo: containerView.trailingAnchor),
			backgroundView.topAnchor.constraint(equalTo: containerView.topAnchor),
			backgroundView.bottomAnchor.constraint(equalTo: containerView.bottomAnchor),
			innerViewController.view.leadingAnchor.constraint(equalTo: containerView.leadingAnchor),
			innerViewController.view.trailingAnchor.constraint(equalTo: containerView.trailingAnchor),
			innerViewController.view.topAnchor.constraint(equalTo: containerView.topAnchor),
			innerViewController.view.bottomAnchor.constraint(equalTo: containerView.bottomAnchor),
		])
	}

	override open func viewWillLayoutSubviews() {
		super.viewWillLayoutSubviews()
		widthLayoutConstraint.constant = finalWidth
	}

	override open func viewWillAppear(_ animated: Bool) {
		super.viewWillAppear(animated)

		if animated {
			view.backgroundColor = view.backgroundColor!.withAlphaComponent(0)
			UIView.animate(withDuration: 0.3) {
				self.view.backgroundColor = self.view.backgroundColor!.withAlphaComponent(0.2)
			}
		}
	}

	private let keyboardNotificationNames = [
		UIResponder.keyboardWillShowNotification,
		UIResponder.keyboardWillHideNotification,
		UIResponder.keyboardWillChangeFrameNotification
	]

	override open func viewDidAppear(_ animated: Bool) {
		super.viewDidAppear(animated)

		keyboardNotificationNames.forEach {
			NotificationCenter.default.addObserver(self, selector: #selector(keyboardFrameWillChange(_:)), name: $0, object: nil)
		}
	}

	override open func viewWillDisappear(_ animated: Bool) {
		super.viewWillDisappear(animated)

		keyboardNotificationNames.forEach {
			NotificationCenter.default.removeObserver(self, name: $0, object: nil)
		}

		if animated {
			UIView.animate(withDuration: 0.3) {
				self.view.backgroundColor = self.view.backgroundColor!.withAlphaComponent(0)
			}
		}
	}

	@objc private func keyboardFrameWillChange(_ notification: Notification) {
		guard let userInfo = notification.userInfo,
			let keyboardEndFrame = userInfo[UIResponder.keyboardFrameEndUserInfoKey] as? CGRect,
			let duration = userInfo[UIResponder.keyboardAnimationDurationUserInfoKey] as? TimeInterval,
			let curve = userInfo[UIResponder.keyboardAnimationCurveUserInfoKey] as? UInt
			else {
				return
		}

		let isHiding = notification.name == UIResponder.keyboardWillHideNotification

		var options: UIView.AnimationOptions = .beginFromCurrentState
		options.insert(.init(rawValue: curve << 16))

		bottomLayoutConstraint.constant = max(isHiding ? 0 : keyboardEndFrame.size.height, 30)
		UIView.animate(withDuration: duration, delay: 0, options: options, animations: {
			self.view.layoutIfNeeded()
		})
	}

}
