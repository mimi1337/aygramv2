// This is the source code of AyuGram for Desktop.
//
// We do not and cannot prevent the use of our code,
// but be respectful and credit the original author.
//
// Copyright @Radolyn, 2026
#include "ayu/features/browser/browser_window.h"

#include "ayu/features/browser/browser_widget.h"
#include "mainwidget.h"
#include "storage/storage_account.h"
#include "ui/qt_object_factory.h"
#include "window/window_session_controller.h"

#include <QtCore/QEvent>
#include <QtCore/QPointer>
#include <QtGui/QResizeEvent>
#include <QtWidgets/QWidget>

namespace AyuFeatures::Browser {
namespace {

// Panel width: a comfortable reading width, but never more
// than half of the window.
constexpr auto kPanelPreferredWidth = 440;
constexpr auto kPanelMinWidth = 320;
constexpr auto kPanelSkip = 0;

[[nodiscard]] QRect PanelGeometry(not_null<QWidget*> parent) {
	const auto available = parent->rect();
	const auto half = available.width() / 2;
	const auto width = std::clamp(
		kPanelPreferredWidth,
		std::min(kPanelMinWidth, available.width()),
		std::max(half, kPanelMinWidth));
	return QRect(
		available.x() + available.width() - width + kPanelSkip,
		available.y(),
		width,
		available.height());
}

// Keeps the panel in sync with the size of the window it is attached to.
class Host final : public QObject {
public:
	Host(
		not_null<Window::SessionController*> controller,
		not_null<QWidget*> parent);

	void show(QString url);
	void hide();
	[[nodiscard]] bool shown() const;

private:
	bool eventFilter(QObject *watched, QEvent *event) override;
	void updateGeometry();

	const not_null<Window::SessionController*> _controller;
	const not_null<QWidget*> _parent;
	Widget *_panel = nullptr;

};

Host::Host(
	not_null<Window::SessionController*> controller,
	not_null<QWidget*> parent)
: QObject(parent.get())
, _controller(controller)
, _parent(parent) {
	_parent->installEventFilter(this);
}

void Host::show(QString url) {
	if (!_panel) {
		_panel = Ui::CreateChild<Widget>(
			_parent,
			_controller->session().local().resolveStorageIdOther(),
			true);
	}
	updateGeometry();
	_panel->raise();
	if (url.isEmpty()) {
		_panel->showDefault();
	} else {
		_panel->showFor(std::move(url));
	}
}

void Host::hide() {
	if (_panel) {
		_panel->hidePanel();
	}
}

bool Host::shown() const {
	return _panel && !_panel->isHidden();
}

bool Host::eventFilter(QObject *watched, QEvent *event) {
	if (watched == _parent.get()
		&& event->type() == QEvent::Resize) {
		updateGeometry();
	}
	return QObject::eventFilter(watched, event);
}

void Host::updateGeometry() {
	if (!_panel) {
		return;
	}
	_panel->setGeometry(PanelGeometry(_parent));
}

[[nodiscard]] Host *FindHost(not_null<QWidget*> parent) {
	return parent->findChild<Host*>(
		QStringLiteral("ayuBrowserHost"),
		Qt::FindDirectChildrenOnly);
}

} // namespace

void Open(not_null<Window::SessionController*> controller, QString url) {
	const auto parent = controller->widget()
		? static_cast<QWidget*>(controller->widget())
		: nullptr;
	if (!parent) {
		return;
	}
	if (const auto existing = QPointer<Host>(FindHost(parent))) {
		existing->show(std::move(url));
		return;
	}
	const auto host = new Host(controller, parent);
	host->setObjectName(QStringLiteral("ayuBrowserHost"));
	host->show(std::move(url));
}

void Close(not_null<Window::SessionController*> controller) {
	const auto parent = controller->widget()
		? static_cast<QWidget*>(controller->widget())
		: nullptr;
	if (!parent) {
		return;
	}
	if (const auto existing = QPointer<Host>(FindHost(parent))) {
		existing->hide();
	}
}

} // namespace AyuFeatures::Browser
