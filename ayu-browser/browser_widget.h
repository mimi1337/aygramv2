// This is the source code of AyuGram for Desktop.
//
// We do not and cannot prevent the use of our code,
// but be respectful and credit the original author.
//
// Copyright @Radolyn, 2026
#pragma once

#include "ui/rp_widget.h"

#include <memory>

namespace Ui {
class IconButton;
class InputField;
class FlatLabel;
} // namespace Ui

namespace Webview {
struct StorageId;
} // namespace Webview

namespace AyuFeatures::Browser {

// A compact in-window browser panel: a small toolbar (back / forward / reload /
// address field / close) and a native WebView under it.
//
// The panel is a plain RpWidget, not a window, so it is attached *inside* an
// existing window and never spawns a separate OS window.
class Widget final : public Ui::RpWidget {
public:
	Widget(
		QWidget *parent,
		Webview::StorageId storageId,
		bool safe);
	~Widget() override;

	// Navigates to `url` (a scheme is guessed when it is missing) and shows
	// the panel. Returns false when the WebView backend is unavailable.
	bool showFor(QString url);

	// Shows the panel on the default page.
	void showDefault();

	void hidePanel();
	[[nodiscard]] bool shown() const {
		return !isHidden();
	}

protected:
	void paintEvent(QPaintEvent *e) override;
	void resizeEvent(QResizeEvent *e) override;

private:
	void setupToolbar();
	void setupWebview();
	void updateControlsGeometry();
	void updateHistoryButtons();
	[[nodiscard]] QString resolveUrl(const QString &input) const;

	void navigate();
	void reload();
	void goBack();
	void goForward();

	Webview::StorageId _storageId;
	const bool _safe = false;

	Ui::IconButton *_back = nullptr;
	Ui::IconButton *_forward = nullptr;
	Ui::IconButton *_reload = nullptr;
	Ui::IconButton *_close = nullptr;
	Ui::InputField *_address = nullptr;
	Ui::FlatLabel *_error = nullptr;

	std::unique_ptr<struct WebviewHolder> _webview;

	bool _webviewReady = false;

};

// Returns true when the current platform has a working WebView backend.
[[nodiscard]] bool Available();

} // namespace AyuFeatures::Browser
