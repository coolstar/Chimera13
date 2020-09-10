//
//  ColorPickerInnerViewController.swift
//  Alderis
//
//  Created by Adam Demasi on 12/3/20.
//  Copyright © 2020 HASHBANG Productions. All rights reserved.
//

import UIKit

extension ColorPickerTab {
	var tabClass: ColorPickerTabViewController.Type {
		switch self {
		case .swatch: return ColorPickerSwatchViewController.self
		case .map: return ColorPickerMapViewController.self
		case .sliders: return ColorPickerSlidersViewController.self
		}
	}

	var index: Int {
		Self.allCases.firstIndex(of: self)!
	}
}

class ColorPickerInnerViewController: UIViewController {

	weak var delegate: ColorPickerDelegate?
	var overrideSmartInvert: Bool
	var color: Color

	var tab: ColorPickerTab {
		get { ColorPickerTab.allCases[currentTab] }
		set { currentTab = newValue.index }
	}

	func setColor(_ color: Color, withSource source: ColorPickerTabViewControllerBase? = nil) {
		self.color = color
		colorDidChange(withSource: source)
	}

	private var colorPicker: ColorPickerViewController {
		parent as! ColorPickerViewController
	}

    init(delegate: ColorPickerDelegate?, overrideSmartInvert: Bool, color: Color) {
		self.delegate = delegate
		self.overrideSmartInvert = overrideSmartInvert
		self.color = color
		self.currentTab = 0
		super.init(nibName: nil, bundle: nil)
	}
	required init?(coder: NSCoder) {
		fatalError("init(coder:) has not been implemented")
	}

	private var currentTab: Int {
		didSet {
			tabDidChange(oldValue: oldValue)
		}
	}

	private var pageViewController: UIPageViewController!
	private var tabs = [ColorPickerTabViewController]()
	private var tabButtons = [UIButton]()
	private var cancelButton: UIButton!
	private var saveButton: UIButton!
	private var tabsBackgroundView: UIView!
	private var buttonsBackgroundView: UIView!
	private var heightConstraint: NSLayoutConstraint!
	private var backgroundView: UIView!

	override func viewDidLoad() {
		super.viewDidLoad()

		for tabType in ColorPickerTab.allCases {
			let tab = tabType.tabClass.init(tabDelegate: self, overrideSmartInvert: overrideSmartInvert, color: color)
			_ = tab.view
			tabs.append(tab)
		}

		backgroundView = UIView()
		backgroundView.translatesAutoresizingMaskIntoConstraints = false
		backgroundView.accessibilityIgnoresInvertColors = overrideSmartInvert
		view.addSubview(backgroundView)

		tabsBackgroundView = UIView()
		tabsBackgroundView.translatesAutoresizingMaskIntoConstraints = false
		tabsBackgroundView.accessibilityIgnoresInvertColors = overrideSmartInvert
		view.addSubview(tabsBackgroundView)

		let topSeparatorView = ColorPickerSeparatorView(direction: .horizontal)
		topSeparatorView.translatesAutoresizingMaskIntoConstraints = false
		tabsBackgroundView.addSubview(topSeparatorView)

		buttonsBackgroundView = UIView()
		buttonsBackgroundView.translatesAutoresizingMaskIntoConstraints = false
		buttonsBackgroundView.accessibilityIgnoresInvertColors = overrideSmartInvert
		view.addSubview(buttonsBackgroundView)

		let bottomSeparatorView = ColorPickerSeparatorView(direction: .horizontal)
		bottomSeparatorView.translatesAutoresizingMaskIntoConstraints = false
		buttonsBackgroundView.addSubview(bottomSeparatorView)

		for (i, tab) in tabs.enumerated() {
			let button = UIButton(type: .system)
			button.translatesAutoresizingMaskIntoConstraints = false
			button.accessibilityIgnoresInvertColors = overrideSmartInvert
			button.tag = i
			button.accessibilityLabel = tab.title
			button.setImage(type(of: tab).image, for: .normal)
			button.addTarget(self, action: #selector(tabButtonTapped(_:)), for: .touchUpInside)
			tabButtons.append(button)
		}

		let tabsView = UIStackView(arrangedSubviews: tabButtons)
		tabsView.translatesAutoresizingMaskIntoConstraints = false
		tabsView.axis = .horizontal
		tabsView.alignment = .fill
		tabsView.distribution = .fillEqually

		pageViewController = UIPageViewController(transitionStyle: .scroll, navigationOrientation: .horizontal, options: [:])
		pageViewController.view.translatesAutoresizingMaskIntoConstraints = false
		pageViewController.willMove(toParent: self)
		addChild(pageViewController)

		cancelButton = UIButton(type: .system)
		cancelButton.translatesAutoresizingMaskIntoConstraints = false
		cancelButton.accessibilityIgnoresInvertColors = overrideSmartInvert
		cancelButton.titleLabel!.font = .systemFont(ofSize: 17, weight: .regular)
		cancelButton.setTitle("Cancel", for: .normal)
		cancelButton.addTarget(self, action: #selector(cancelTapped), for: .touchUpInside)

		saveButton = UIButton(type: .system)
		saveButton.translatesAutoresizingMaskIntoConstraints = false
		saveButton.accessibilityIgnoresInvertColors = overrideSmartInvert
		saveButton.titleLabel!.font = UIFont.systemFont(ofSize: 17, weight: .semibold)
		saveButton.setTitle("Done", for: .normal)
		saveButton.addTarget(self, action: #selector(saveTapped), for: .touchUpInside)

		let buttonSeparatorView = ColorPickerSeparatorView(direction: .vertical)
		buttonSeparatorView.translatesAutoresizingMaskIntoConstraints = false

		let buttonsView = UIStackView(arrangedSubviews: [ cancelButton, buttonSeparatorView, saveButton ])
		buttonsView.translatesAutoresizingMaskIntoConstraints = false
		buttonsView.axis = .horizontal
		buttonsView.alignment = .fill

		let mainStackView = UIStackView(arrangedSubviews: [ tabsView, pageViewController.view, buttonsView ])
		mainStackView.translatesAutoresizingMaskIntoConstraints = false
		mainStackView.axis = .vertical
		mainStackView.alignment = .fill
		mainStackView.distribution = .fill
		view.addSubview(mainStackView)

		heightConstraint = pageViewController.view.heightAnchor.constraint(equalToConstant: 300)
		heightConstraint.priority = .defaultHigh

		let tabsHeight: CGFloat = 44
		NSLayoutConstraint.activate([
			backgroundView.topAnchor.constraint(equalTo: view.topAnchor),
			backgroundView.bottomAnchor.constraint(equalTo: view.bottomAnchor),
			backgroundView.leadingAnchor.constraint(equalTo: view.leadingAnchor),
			backgroundView.trailingAnchor.constraint(equalTo: view.trailingAnchor),
			mainStackView.topAnchor.constraint(equalTo: view.topAnchor),
			mainStackView.bottomAnchor.constraint(equalTo: view.bottomAnchor),
			mainStackView.leadingAnchor.constraint(equalTo: view.leadingAnchor),
			mainStackView.trailingAnchor.constraint(equalTo: view.trailingAnchor),
			tabsView.heightAnchor.constraint(equalToConstant: tabsHeight),
			buttonsView.heightAnchor.constraint(equalToConstant: tabsHeight),
			tabsBackgroundView.topAnchor.constraint(equalTo: view.topAnchor),
			tabsBackgroundView.leadingAnchor.constraint(equalTo: view.leadingAnchor),
			tabsBackgroundView.trailingAnchor.constraint(equalTo: view.trailingAnchor),
			tabsBackgroundView.heightAnchor.constraint(equalToConstant: tabsHeight),
			buttonsBackgroundView.topAnchor.constraint(equalTo: buttonsView.topAnchor),
			buttonsBackgroundView.bottomAnchor.constraint(equalTo: buttonsView.bottomAnchor),
			buttonsBackgroundView.leadingAnchor.constraint(equalTo: buttonsView.leadingAnchor),
			buttonsBackgroundView.trailingAnchor.constraint(equalTo: buttonsView.trailingAnchor),
			topSeparatorView.leadingAnchor.constraint(equalTo: tabsBackgroundView.leadingAnchor),
			topSeparatorView.trailingAnchor.constraint(equalTo: tabsBackgroundView.trailingAnchor),
			topSeparatorView.bottomAnchor.constraint(equalTo: tabsBackgroundView.bottomAnchor),
			bottomSeparatorView.leadingAnchor.constraint(equalTo: buttonsBackgroundView.leadingAnchor),
			bottomSeparatorView.trailingAnchor.constraint(equalTo: buttonsBackgroundView.trailingAnchor),
			bottomSeparatorView.topAnchor.constraint(equalTo: buttonsBackgroundView.topAnchor),
			heightConstraint,
			cancelButton.widthAnchor.constraint(equalTo: saveButton.widthAnchor)
		])

		colorDidChange()
		tabDidChange(oldValue: currentTab)
	}

	override func viewWillLayoutSubviews() {
		super.viewWillLayoutSubviews()
		updateHeightConstraint()
	}

	override func viewDidLayoutSubviews() {
		super.viewDidLayoutSubviews()
		updateHeightConstraint()
	}

	override func preferredContentSizeDidChange(forChildContentContainer container: UIContentContainer) {
		super.preferredContentSizeDidChange(forChildContentContainer: container)
		updateHeightConstraint()
	}

	private func updateHeightConstraint() {
		DispatchQueue.main.async {
			for tab in self.tabs {
				tab.view.layoutIfNeeded()
			}
			let preferredHeight = self.tabs[self.currentTab].preferredContentSize.height
			if preferredHeight > 0 {
				self.heightConstraint?.constant = preferredHeight
			}
		}
	}

	@objc private func tabButtonTapped(_ sender: UIButton) {
		UIView.animate(withDuration: 0.2) {
			self.currentTab = sender.tag
		}
	}

	@objc private func cancelTapped() {
		delegate?.colorPickerDidCancel?(colorPicker)
		dismiss(animated: true)
	}

	@objc private func saveTapped() {
		delegate?.colorPicker(colorPicker, didSelect: color.uiColor)
		dismiss(animated: true)
	}

	private func colorDidChange(withSource source: ColorPickerTabViewControllerBase? = nil) {
		let foregroundColor: UIColor = color.isDark ? .white : .black

		view.tintColor = color.uiColor
		tabsBackgroundView.backgroundColor = color.uiColor
		buttonsBackgroundView.backgroundColor = color.uiColor
		cancelButton.tintColor = foregroundColor
		saveButton.tintColor = foregroundColor

		for (i, button) in tabButtons.enumerated() {
			button.tintColor = i == currentTab ? foregroundColor : foregroundColor.withAlphaComponent(0.6)
		}

		// Even though `shouldBroadcast: false` avoids recursion if we call setColor on the callee tab,
		// doing so on ColorPickerSlidersViewController would reset `hexOptions`, leading to a buggy typing
		// experience in `hexTextField`
		for tab in tabs where tab != source {
			tab.setColor(color, shouldBroadcast: false)
		}

		backgroundView.backgroundColor = color.uiColor.withAlphaComponent(0.1)
	}

	private func tabDidChange(oldValue: Int) {
		let direction: UIPageViewController.NavigationDirection = currentTab < oldValue ? .reverse : .forward
		pageViewController.setViewControllers([ tabs[currentTab] ], direction: direction, animated: true)
		colorDidChange()

		UIView.animate(withDuration: 0.2) {
			self.view.layoutIfNeeded()
		}
	}

}

extension ColorPickerInnerViewController: ColorPickerTabDelegate {

	func colorPickerTab(_ tab: ColorPickerTabViewControllerBase, didSelect color: Color) {
		self.setColor(color, withSource: tab)
	}

}
