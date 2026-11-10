pragma Singleton

import QtQuick 2.2

import "locales/en.js" as En
import "locales/es.js" as Es

// To add a language:
//   1. Copy qml/js/locales/en.js to qml/js/locales/<code>.js and translate it.
//   2. Import it above.
//   3. Add one entry to `catalog` below.
// The settings selector and the device-language fallback pick it up
// automatically.

QtObject {
    id: i18n

    // "system" follows the device language; any code from `catalog` forces it.
    property string language: typeof vault !== "undefined" ? vault.language : "system"

    readonly property var catalog: [
        { "code": "en", "name": En.languageName, "strings": En.strings,
          "duration": En.duration },
        { "code": "es", "name": Es.languageName, "strings": Es.strings,
          "duration": Es.duration }
    ]

    function stringsFor(code) {
        for (var i = 0; i < catalog.length; ++i) {
            if (catalog[i].code === code) {
                return catalog[i].strings
            }
        }
        return null
    }

    readonly property string deviceLanguage: {
        var code = Qt.locale().name.substring(0, 2).toLowerCase()
        return stringsFor(code) !== null ? code : "en"
    }

    readonly property string effectiveLanguage: {
        return stringsFor(language) !== null ? language : deviceLanguage
    }

    // A stable list so the settings menu items are not recreated (and the
    // ComboBox does not lose its selection) when the language changes.
    readonly property var languageCodes: {
        var codes = ["system"]
        for (var i = 0; i < catalog.length; ++i) {
            codes.push(catalog[i].code)
        }
        return codes
    }

    function languageName(code) {
        if (code === "system") {
            return t("language_system")
        }
        for (var i = 0; i < catalog.length; ++i) {
            if (catalog[i].code === code) {
                return catalog[i].name
            }
        }
        return code
    }

    function languageIndex(code) {
        var codes = languageCodes
        for (var i = 0; i < codes.length; ++i) {
            if (codes[i] === code) {
                return i
            }
        }
        return 0
    }

    function duration(seconds) {
        for (var i = 0; i < catalog.length; ++i) {
            if (catalog[i].code === effectiveLanguage) {
                return catalog[i].duration(seconds)
            }
        }
        return seconds + " " + t(seconds < 60 ? "seconds" : "minutes")
    }

    function t(key) {
        // Reading effectiveLanguage here is what makes QML bindings using
        // t() re-evaluate when the user changes the language.
        var strings = stringsFor(effectiveLanguage)
        if (strings !== null && strings[key] !== undefined) {
            return strings[key]
        }
        var fallback = stringsFor("en")
        if (fallback !== null && fallback[key] !== undefined) {
            return fallback[key]
        }
        return key
    }
}
