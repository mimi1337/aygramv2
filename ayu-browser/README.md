# Браузер внутри AyuGram Desktop

Ветка с фичей: `feat/inapp-browser` (база — `dev` коммит `0f2a31b`).
Один коммит, 10 файлов, +669 строк, новых зависимостей нет.

## Что сделано

Компактная панель-браузер **внутри окна чата** (не отдельное окно ОС), на нативном
WebView, который уже есть в AyuGram (WebView2 на Windows, WebKit на macOS,
WebKitGTK на Linux — тот же движок, что в IV-редакторе и оплате).

- Тулбар: `←` `→` `⟳` + адресная строка + `✕`
- Адресная строка: `example.com` → https, фраза без точки → поиск в DuckDuckGo
- Панель живёт справа поверх области чата, ширина 440 px (но не больше половины окна)
- Одна панель на окно, при скрытии/показе сессия и cookies сохраняются
- Панель прячется кнопкой `✕`; состояние браузера не теряется

Точки входа (появляются только если WebView реально доступен на платформе):

1. Маленькая иконка-ссылка в топбаре чата (рядом с поиском/меню, справа)
2. Пункт **«Open browser»** в главном меню (гамбургер)

## Файлы

Новые:

```
Telegram/SourceFiles/ayu/features/browser/browser_widget.h    — панель (RpWidget + WebView)
Telegram/SourceFiles/ayu/features/browser/browser_widget.cpp
Telegram/SourceFiles/ayu/features/browser/browser_window.h    — Open()/Close(), хост на QObject
Telegram/SourceFiles/ayu/features/browser/browser_window.cpp
```

Правки:

```
Telegram/CMakeLists.txt                                     +4   регистрация файлов
Telegram/SourceFiles/ayu/ui/ayu_styles.style                +52  стили панели и кнопки
Telegram/SourceFiles/history/view/history_view_top_bar_widget.{h,cpp} +14 иконка в топбаре
Telegram/SourceFiles/window/window_main_menu.cpp            +10  пункт меню
Telegram/Resources/langs/lang.strings                       +3   строки локализации
```

## Как применить

### Вариант A — патч (просто)

```bash
git clone https://github.com/mimi1337/AyuGramDesktop.git
cd AyuGramDesktop
git submodule update --init --recursive
git checkout -b feat/inapp-browser
git am 0001-inapp-browser.patch
git push -u origin feat/inapp-browser
```

### Вариант B — вручную

Скопировать 4 файла из этой папки в
`Telegram/SourceFiles/ayu/features/browser/` (папку создать),
затем применить `0002-integration.diff`:

```bash
git apply 0002-integration.diff
```

## Сборка

По `AGENTS.md` проекта:

```bash
cmake --build out --config Debug --target Telegram
```

Windows: нужен Visual Studio 2022 + WebView2 Runtime (в Windows 11 уже есть).

## Что проверить при запуске

- [ ] Иконка в топбаре чата видна и открывает панель
- [ ] Пункт «Open browser» в главном меню работает
- [ ] Ввод `github.com` в адресную строку открывает сайт
- [ ] Ввод `котики` открывает поиск
- [ ] `←` / `→` включаются/выключаются по истории
- [ ] `✕` скрывает панель, повторное открытие сохраняет страницу
- [ ] Изменение размера окна — панель растягивается корректно

## Известные ограничения

- Панель пока не перетаскивается и не ресайзится вручную (только окно)
- Нет вкладок и поиска по странице
- На Linux нужен `libwebkit2gtk` — без него панель покажет
  «WebView is unavailable» и кнопки не появятся вовсе

## Не собрано

Собрать здесь невозможно: нужны Qt 6, MSVC/нативный тулчейн и ~5 GB
зависимостей (`Libraries/`, `ThirdParty/`), которых в песочнице нет.
Код написан строго по API, закреплённому сабмодулями форка
(`lib_webview` @ `596fb24`, `lib_ui` @ `master-ui`), но компиляцию
и отладку нужно сделать на ПК.
