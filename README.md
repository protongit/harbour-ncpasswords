# NC Passwords for Sailfish OS

A native Sailfish OS client for the **Passwords** app for Nextcloud.
`harbour-ncpasswords` talks directly to your Nextcloud server, decrypts
client-side encrypted vaults on the device and never sends data anywhere else.

> This is an independent, unofficial client. It is not affiliated with
> Nextcloud GmbH or the author of the Nextcloud Passwords app.

## Features

- Passwords: list, search (press Enter), create, edit, delete and restore from
  the trash
- Folders and tags, with an optional folder-first start view
- Favourites, hidden entries, notes and custom fields
- URL and password copy actions, password reveal, and a generator with four
  strength presets
- TOTP codes (`otpauth://` and `client.ios.otp`), also on the cover with
  previous/next navigation
- Client-side encryption (`CSEv1r1`) with the vault's master password
- Nextcloud Login Flow v2, including accounts protected with two-factor
  authentication
- App PIN lock (Argon2id) and automatic locking, from 10 seconds to 30 minutes
- App password stored in **Sailfish Secrets**; the clipboard is cleared
  automatically
- English and Spanish UI, with a language selector in Settings
- No telemetry, no analytics and no third-party services

## Requirements

- Sailfish OS 5.x (tested on the Jolla Phone 2026, aarch64)
- A Nextcloud server with the
  [Passwords app](https://apps.nextcloud.com/apps/passwords)
- A Nextcloud **app password** (do not use your account password)
- `libsodium` and `libsailfishsecrets` at runtime

## Install

### SailfishOS:Chum

Once the package is published, install it from the Chum GUI or with:

```sh
devel-su pkcon install harbour-ncpasswords
```

### RPM

```sh
devel-su pkcon install-local -y harbour-ncpasswords-*.aarch64.rpm
```

Close the app before installing or updating it.

## Usage

1. Enter the Nextcloud address and an app password, or use **Connect with
   browser** to authorise the app through Nextcloud.
2. Unlock the vault with its master password. It is kept in memory only.
3. If the master password is lost there is no recovery: it never leaves the
   device and is not stored anywhere.
4. Pull down on the list to open the menu. The search field filters when you
   press Enter; clearing it restores the full list.

## Build

Install the Sailfish SDK and add the `libsodium-devel` package from Chum to the
build target:

```sh
sfdk -c target=SailfishOS-5.1.0.11-aarch64 tools target exec \
  SailfishOS-5.1.0.11-aarch64 bash -lc \
  'zypper -n --no-gpg-checks ar -f \
   https://repo.sailfishos.org/obs/sailfishos:/chum/5.1_aarch64/ chum && \
   zypper -n --no-gpg-checks ref chum && \
   zypper -n --no-gpg-checks in -y libsodium-devel'
```

Then build the RPM:

```sh
sfdk -c target=SailfishOS-5.1.0.11-aarch64 build
```

The package is written to `RPMS/harbour-ncpasswords-<version>-1.aarch64.rpm`.

## Development helpers

The binary includes a few modes that are useful without a compositor:

```sh
harbour-ncpasswords --selftest   # libsodium and wire-format test vectors
harbour-ncpasswords --qmlcheck   # load every QML page offscreen
harbour-ncpasswords --qmlrun     # load the app with the minimal platform
```

The `--cli` mode exposes the API for debugging:

```sh
harbour-ncpasswords --cli probe  <server> <user> <app-password>
harbour-ncpasswords --cli list   <server> <user> <app-password> <master-password>
harbour-ncpasswords --cli create <server> <user> <app-password> <master-password> <label> <username> <secret>
harbour-ncpasswords --cli update <server> <user> <app-password> <master-password> <id> <label> <username> <secret>
harbour-ncpasswords --cli delete <server> <user> <app-password> <master-password> <id>
```

## Translations

Each language lives in `qml/js/locales/<code>.js` and exports `languageName`,
`strings` and `duration(seconds)`. To add a language:

1. Copy `qml/js/locales/en.js` to `qml/js/locales/<code>.js` and translate the
   values.
2. Import it in `qml/js/I18n.qml` and add one entry to `catalog`.
3. The language selector and the device-language fallback pick it up
   automatically.

## Security and privacy

- The Nextcloud app password is stored in Sailfish Secrets when available and
  falls back to a `0600` file in the app's private data directory otherwise.
- The master password and the derived keychain are never written to disk.
- The clipboard is cleared after the configured timeout.
- The app talks only to the Nextcloud server you configure. There is no
  telemetry or crash reporting.
- Report security issues privately; see [SECURITY.md](SECURITY.md).

## Licence

MIT. See [LICENSE](LICENSE). Nextcloud is a trademark of Nextcloud GmbH.
