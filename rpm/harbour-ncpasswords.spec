Name:       harbour-ncpasswords
Summary:    Nextcloud Passwords client for Sailfish OS
Version:    1.0.0
Release:    1
License:    MIT
URL:        https://github.com/protongit/harbour-ncpasswords
Source0:    %{name}-%{version}.tar.bz2

Requires:   sailfishsilica-qt5 >= 0.10.9
Requires:   libsodium
Requires:   libsailfishsecrets
Requires:   qt5-qtnetwork

BuildRequires:  pkgconfig(sailfishapp) >= 1.0.2
BuildRequires:  pkgconfig(Qt5Core)
BuildRequires:  pkgconfig(Qt5Gui)
BuildRequires:  pkgconfig(Qt5Qml)
BuildRequires:  pkgconfig(Qt5Quick)
BuildRequires:  pkgconfig(Qt5Network)
BuildRequires:  pkgconfig(libsodium)
BuildRequires:  pkgconfig(sailfishsecrets)
BuildRequires:  desktop-file-utils

%description
A native Sailfish OS client for the Passwords app for Nextcloud.
It supports client-side encryption, passwords, folders, tags, favourites,
the trash, TOTP codes, a PIN lock and a password generator.

This is an unofficial client. Nextcloud is a trademark of the
Nextcloud GmbH.

%if 0%{?_chum}
Title: NC Passwords
Type: desktop-application
DeveloperName: protongit
Categories:
 - Utility
 - Security
AIRating: C
AINote: AI-assisted development, tested on device by the author.
Custom:
  Repo: https://github.com/protongit/harbour-ncpasswords
PackageIcon: https://raw.githubusercontent.com/protongit/harbour-ncpasswords/main/icons/256x256/harbour-ncpasswords.png
Links:
  Homepage: https://github.com/protongit/harbour-ncpasswords
  Bugtracker: https://github.com/protongit/harbour-ncpasswords/issues
%endif

%prep
%setup -q -n %{name}-%{version}

%build
%qmake5
%make_build

%install
%qmake5_install

desktop-file-install --delete-original       \
  --dir %{buildroot}%{_datadir}/applications  \
  %{buildroot}%{_datadir}/applications/*.desktop

%files
%defattr(-,root,root,-)
%license LICENSE
%doc README.md CHANGELOG.md
%{_bindir}/%{name}
%{_datadir}/%{name}
%{_datadir}/applications/%{name}.desktop
%{_datadir}/icons/hicolor/*/apps/%{name}.png

%changelog
* Tue Nov 10 2026 protongit <protongit@users.noreply.github.com> - 1.0.0-1
- First public release

* Tue Nov 10 2026 protongit <protongit@users.noreply.github.com> - 0.7.0-1
- Add a password strength setting for generated passwords
- Add reveal and copy icons next to generate in the password editor

* Tue Nov 10 2026 protongit <protongit@users.noreply.github.com> - 0.6.3-1
- Declare menu items in reverse so the pull-down order matches the design

* Tue Nov 10 2026 protongit <protongit@users.noreply.github.com> - 0.6.2-1
- Fix the translation singleton import so all texts resolve
- Fall back to the system language when none is stored

* Tue Nov 10 2026 protongit <protongit@users.noreply.github.com> - 0.6.1-1
- Move Refresh below New password in the pull-down menu

* Tue Nov 10 2026 protongit <protongit@users.noreply.github.com> - 0.6.0-1
- Add a per-language translation catalogue and a language selector
- Use a name placeholder in the password editor and an icon to generate
- Show tags as selectable chips
- Reorder the pull-down menu

* Tue Nov 10 2026 protongit <protongit@users.noreply.github.com> - 0.5.3-1
- Remove the unused controller search debounce

* Tue Nov 10 2026 protongit <protongit@users.noreply.github.com> - 0.5.2-1
- Restore the PullDownMenu layout with the header inside the list
- Handle Enter inside the search field component

* Tue Nov 10 2026 protongit <protongit@users.noreply.github.com> - 0.5.1-1
- Apply the search filter on Enter and clear it when the field is emptied

* Tue Nov 10 2026 protongit <protongit@users.noreply.github.com> - 0.5.0-1
- Debounce search in the controller
- Log search calls for diagnostics

* Tue Nov 10 2026 protongit <protongit@users.noreply.github.com> - 0.4.9-1
- Restore the standard Sailfish PullDownMenu
- Keep the search field text across model resets

* Tue Nov 10 2026 protongit <protongit@users.noreply.github.com> - 0.4.8-1
- Fix the header ContextMenu invocation

* Tue Nov 10 2026 protongit <protongit@users.noreply.github.com> - 0.4.7-1
- Open URLs from an icon next to the copy button

* Tue Nov 10 2026 protongit <protongit@users.noreply.github.com> - 0.4.6-1
- Replace the pull-down menu with a header menu button
- Keep the search field fixed and filtering again

* Tue Nov 10 2026 protongit <protongit@users.noreply.github.com> - 0.4.5-1
- Put headers back inside the lists so PullDownMenu aligns with the page

* Tue Nov 10 2026 protongit <protongit@users.noreply.github.com> - 0.4.4-1
- Replace the highlighted PageHeader with a plain title on list pages

* Tue Nov 10 2026 protongit <protongit@users.noreply.github.com> - 0.4.3-1
- Fix the highlighted PageHeader blue band

* Tue Nov 10 2026 protongit <protongit@users.noreply.github.com> - 0.4.2-1
- Add 10 s and 30 s auto-lock options
- Open folder contents on a separate page with normal back navigation

* Tue Nov 10 2026 protongit <protongit@users.noreply.github.com> - 0.4.1-1
- Debounce the search field and keep it outside the list
- Remove the entry-name copy button

* Tue Nov 10 2026 protongit <protongit@users.noreply.github.com> - 0.4.0-1
- Add PIN app lock
- Add configurable flat/folder view

* Tue Nov 10 2026 protongit <protongit@users.noreply.github.com> - 0.3.8-1
- Disable the unsupported fingerprint option

* Tue Nov 10 2026 protongit <protongit@users.noreply.github.com> - 0.3.7-1
- Use the native device-lock Authenticator for fingerprint unlock

* Tue Nov 10 2026 protongit <protongit@users.noreply.github.com> - 0.3.6-1
- Defer page navigation until the synchronous load finishes

* Tue Nov 10 2026 protongit <protongit@users.noreply.github.com> - 0.3.5-1
- Do not run the page transition while a synchronous load is in progress
- Stop the fingerprint auto-timer when the page is inactive

* Tue Nov 10 2026 protongit <protongit@users.noreply.github.com> - 0.3.4-1
- Drop ListView cacheBuffer and add crash backtraces

* Tue Nov 10 2026 protongit <protongit@users.noreply.github.com> - 0.3.3-1
- Add crash and load diagnostics

* Tue Nov 10 2026 protongit <protongit@users.noreply.github.com> - 0.3.2-1
- Start system authentication automatically on the fingerprint page
- Log authentication diagnostics

* Tue Nov 10 2026 protongit <protongit@users.noreply.github.com> - 0.3.1-1
- Persist settings inside the sandboxed app data directory
- Recover the app password from the protected secret if the flag was lost

* Tue Nov 10 2026 protongit <protongit@users.noreply.github.com> - 0.3.0-1
- Assign tags to passwords
- Fingerprint/device-lock app unlock through Sailfish Secrets

* Tue Nov 10 2026 protongit <protongit@users.noreply.github.com> - 0.2.4-1
- Report Sailfish Secrets as the active app-password storage

* Tue Nov 10 2026 protongit <protongit@users.noreply.github.com> - 0.2.3-1
- Use a fresh Sailfish Secrets collection name

* Tue Nov 10 2026 protongit <protongit@users.noreply.github.com> - 0.2.2-1
- Use a SQLCipher-compatible collection name for Sailfish Secrets

* Tue Nov 10 2026 protongit <protongit@users.noreply.github.com> - 0.2.1-1
- Diagnostics for Sailfish Secrets under sailjail

* Tue Nov 10 2026 protongit <protongit@users.noreply.github.com> - 0.2.0-1
- Store the app password in Sailfish Secrets
- Add TOTP support on the cover and entry page

* Tue Nov 10 2026 protongit <protongit@users.noreply.github.com> - 0.1.1-1
- Fix QML page loading and startup navigation
- Add Nextcloud Login Flow v2
- Add folders, tags, trash and custom fields

* Mon Nov 09 2026 protongit <protongit@users.noreply.github.com> - 0.1.0-1
- Initial release
