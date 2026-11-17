// This is the source code of AyuGram for Desktop.
//
// We do not and cannot prevent the use of our code,
// but be respectful and credit the original author.
//
// Copyright @Radolyn, 2026
#include "ayu/features/browser/browser_widget.h"

#include "core/cached_webview_availability.h"
#include "lang_auto.h"
#include "styles/style_ayu_styles.h"
#include "ui/painter.h"
#include "ui/qt_object_factory.h"
#include "ui/widgets/buttons.h"
#include "ui/widgets/fields/input_field.h"
#include "ui/widgets/labels.h"
#include "webview/webview_common.h"
#include "webview/webview_embed.h"
#include "webview/webview_interface.h"

#include <QtCore/QUrl>
#include <QtCore/QPointer>
#include <QtGui/QResizeEvent>
#include <QtWidgets/QWidget>

#include <algorithm>

namespace AyuFeatures::Browser {
namespace {

constexpr auto kToolbarHeight = 44;
constexpr auto kToolbarSkip = 6;
constexpr auto kAddressMinWidth = 60;
constexpr auto kMaxAddressWidth = 260;

[[nodiscard]] QString WithScheme(QString url) {
	const auto trimmed = url.trimmed();
	if (trimmed.isEmpty()) {
		return QString();
	}
	const auto parsed = QUrl(trimmed);
	if (!parsed.scheme().isEmpty()) {
		return trimmed;
	}
	// Looks like a host: no spaces and a dot inside.
	if (!trimmed.contains(' ') && trimmed.contains('.')) {
		return u"https://"_q + trimmed;
	}
	// Otherwise treat it as a search query.
	return u"https://duckduckgo.com/?q="_q
		+ QString::fromUtf8(QUrl::toPercentEncoding(trimmed));
}

} // namespace

struct WebviewHolder {
	WebviewHolder(QWidget *parent, Webview::WindowConfig config)
	: window(parent, std::move(config)) {
	}

	Webview::Window window;
	rpl::lifetime lifetime;
};

bool Available() {
	return Core::CachedWebviewAvailability().error
		== Webview::Available::Error::None;
}

Widget::Widget(
	QWidget *parent,
	Webview::StorageId storageId,
	bool safe)
: RpWidget(parent)
, _storageId(std::move(storageId))
, _safe(safe) {
	setupToolbar();
	setupWebview();
	hide();
}

void Widget::setupToolbar() {
	_back = Ui::CreateChild<Ui::IconButton>(this, st::ayuBrowserBack);
	_back->setClickedCallback([=] { if (const auto w = QPointer<Widget>(this)) w->goBack(); });
	_back->setDisabled(true);
	_back->show();

	_forward = Ui::CreateChild<Ui::IconButton>(this, st::ayuBrowserForward);
	_forward->setClickedCallback([=] { if (const auto w = QPointer<Widget>(this)) w->goForward(); });
	_forward->setDisabled(true);
	_forward->show();

	_reload = Ui::CreateChild<Ui::IconButton>(this, st::ayuBrowserReload);
	_reload->setClickedCallback([=] { if (const auto w = QPointer<Widget>(this)) w->reload(); });
	_reload->show();

	_close = Ui::CreateChild<Ui::IconButton>(this, st::ayuBrowserClose);
	_close->setClickedCallback([=] { if (const auto w = QPointer<Widget>(this)) w->hidePanel(); });
	_close->show();

	_address = Ui::CreateChild<Ui::InputField>(
		this,
		st::ayuBrowserAddress,
		Ui::InputField::Mode::NoNewlines,
		rpl::single(QString(u"https://"_q)));
	_address->setPlaceholder(
		rpl::single(tr::ayu_BrowserAddressPlaceholder()));
	_address->submits(
	) | rpl::on_next([=](Qt::KeyboardModifiers) {
		navigate();
	}, _address->lifetime());
	_address->show();

	_error = Ui::CreateChild<Ui::FlatLabel>(this, st::ayuBrowserErrorLabel);
	_error->setText(
		u"WebView is unavailable on this platform."_q);
	_error->hide();
}

void Widget::setupWebview() {
	if (!_storageId || !Available()) {
		_error->show();
		return;
	}
	_webview = std::make_unique<WebviewHolder>(
		this,
		Webview::WindowConfig{
			.opaqueBg = st::windowBg->c,
			.storageId = _storageId,
			.safe = _safe,
		});
	const auto raw = &_webview->window;

	const auto view = raw->widget();
	if (!view) {
		// The backend failed to start (no WebView2 / WebKitGTK installed).
		_webview = nullptr;
		_error->show();
		return;
	}
	_webviewReady = true;
	view->hide();

	const auto weak = QPointer<Widget>(this);
	raw->setNavigationStartHandler([=](const QString &uri, bool newWindow) {
		Q_UNUSED(newWindow);
		if (uri != u"about:blank"_q) {
			_address->setText(uri);
		}
		return true;
	});
	raw->setNavigationDoneHandler([=](bool success) {
		Q_UNUSED(success);
		if (weak) {
			weak->updateHistoryButtons();
		}
	});
	raw->navigationHistoryState(
	) | rpl::on_next([=](Webview::NavigationHistoryState state) {
		_back->setDisabled(!state.canGoBack);
		_forward->setDisabled(!state.canGoForward);
		if (!state.url.empty()) {
			const auto url = QString::fromStdString(state.url);
			if (url != u"about:blank"_q) {
				_address->setText(url);
			}
		}
	}, _webview->lifetime);

	raw->init(R"()");
}

bool Widget::showFor(QString url) {
	if (!_webviewReady) {
		_error->raise();
		_error->show();
		show();
		return false;
	}
	show();
	raise();
	if (const auto resolved = resolveUrl(url); !resolved.isEmpty()) {
		_webview->window.navigate(resolved);
	}
	if (const auto view = _webview->window.widget()) {
		view->show();
		view->raise();
	}
	updateControlsGeometry();
	_webview->window.focus();
	return true;
}

void Widget::showDefault() {
	showFor(QString());
	_address->setFocus();
}

Widget::~Widget() {
	// The native WebView must be gone before the child widgets it overlaps.
	_webview = nullptr;
}

void Widget::hidePanel() {
	hide();
	if (_webviewReady && _webview) {
		if (const auto view = _webview->window.widget()) {
			view->hide();
		}
	}
}

QString Widget::resolveUrl(const QString &input) const {
	return WithScheme(input);
}

void Widget::navigate() {
	if (!_webviewReady || !_webview) {
		return;
	}
	if (const auto url = resolveUrl(_address->getLastText()); !url.isEmpty()) {
		_webview->window.navigate(url);
	}
}

void Widget::reload() {
	if (_webviewReady && _webview) {
		_webview->window.reload();
	}
}

void Widget::goBack() {
	if (_webviewReady && _webview) {
		_webview->window.eval("window.history.back();");
	}
}

void Widget::goForward() {
	if (_webviewReady && _webview) {
		_webview->window.eval("window.history.forward();");
	}
}

void Widget::updateHistoryButtons() {
	if (_webviewReady && _webview) {
		_webview->window.refreshNavigationHistoryState();
	}
}

void Widget::updateControlsGeometry() {
	const auto toolbarTop = kToolbarSkip;
	const auto toolbarLeft = kToolbarSkip;
	const auto toolbarWidth = std::max(
		width() - 2 * kToolbarSkip,
		st::ayuBrowserBack.width * 4);
	const auto toolbar = QRect(
		toolbarLeft,
		toolbarTop,
		toolbarWidth,
		kToolbarHeight);

	auto left = toolbar.x();
	const auto placeLeft = [&](not_null<Ui::RpWidget*> widget) {
		const auto size = widget->size();
		const auto top = toolbar.y() + (kToolbarHeight - size.height()) / 2;
		widget->move(left, top);
		left += size.width();
	};
	placeLeft(_back);
	placeLeft(_forward);
	placeLeft(_reload);

	auto right = toolbar.x() + toolbar.width();
	const auto closeWidth = _close->width();
	_close->move(
		right - closeWidth,
		toolbar.y() + (kToolbarHeight - _close->height()) / 2);
	right -= closeWidth;

	const auto addressLeft = left;
	const auto addressWidth = std::clamp(
		right - addressLeft - kToolbarSkip * 2,
		0,
		kMaxAddressWidth);
	_address->setGeometryToLeft(
		addressLeft,
		toolbar.y() + (kToolbarHeight - _address->height()) / 2,
		std::max(addressWidth, 0),
		_address->height());

	const auto contentTop = toolbar.y() + kToolbarHeight + kToolbarSkip;
	const auto contentWidth = toolbar.width();
	const auto contentHeight = std::max(height() - contentTop - kToolbarSkip, 0);

	if (_error) {
		_error->setGeometry(
			toolbar.x(),
			contentTop,
			contentWidth,
			_error->height());
	}
	if (_webviewReady && _webview) {
		if (const auto view = _webview->window.widget()) {
			view->setGeometry(
				toolbar.x(),
				contentTop,
				contentWidth,
				contentHeight);
		}
	}
}

void Widget::paintEvent(QPaintEvent *) {
	auto p = QPainter(this);
	p.fillRect(rect(), st::windowBg);
}

void Widget::resizeEvent(QResizeEvent *e) {
	RpWidget::resizeEvent(e);
	updateControlsGeometry();
}

} // namespace AyuFeatures::Browser
