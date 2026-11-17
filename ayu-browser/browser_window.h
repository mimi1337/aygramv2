// This is the source code of AyuGram for Desktop.
//
// We do not and cannot prevent the use of our code,
// but be respectful and credit the original author.
//
// Copyright @Radolyn, 2026
#pragma once

#include <QtCore/QString>

namespace Window {
class SessionController;
} // namespace Window

namespace AyuFeatures::Browser {

// Shows the in-app browser panel inside the main window of `controller`.
// The panel is created on first use and then reused (only shown / hidden),
// so the browsing session and cookies survive re-opening.
//
// An empty `url` opens the default page.
void Open(not_null<Window::SessionController*> controller, QString url = {});

// Hides the panel if it was created for that window.
void Close(not_null<Window::SessionController*> controller);

} // namespace AyuFeatures::Browser
