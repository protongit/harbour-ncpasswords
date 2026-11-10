# Publishing to the Jolla Store (Harbour)

The package is ready for Harbour and passes the validator:

```sh
sfdk -c target=SailfishOS-5.1.0.11-aarch64 check -s harbour \
  RPMS/harbour-ncpasswords-1.0.1-1.aarch64.rpm
# Validation succeeded
```

## Harbour-specific packaging decisions

- `libsodium` is not part of the Sailfish OS base image and is not an allowed
  Harbour dependency. The library is bundled in the app-private directory
  `/usr/share/harbour-ncpasswords/lib/`, found through the binary RPATH, and
  excluded from the generated Requires/Provides.
- `libsailfishsecrets` and Qt Network are allowed as shared-library
  dependencies; the explicit package `Requires` were removed.
- The package does not install files under `/usr/share/doc` or
  `/usr/share/licenses`; the licence is shipped as
  `/usr/share/harbour-ncpasswords/LICENSE`.
- The unused `FingerprintPage.qml` was removed because
  `org.nemomobile.devicelock` is not an allowed QML import. Fingerprint unlock
  is not available to sandboxed third-party apps anyway.
- Permissions are limited to `Internet` and `Secrets`.
- The binary and the bundled library are stripped, so the validator reports no
  warnings.

## Submission

1. Create a free developer account at <https://harbour.jolla.com/>.
2. Add the application and upload the aarch64 RPM.
3. Fill in the store listing:
   - Name: **NC Passwords**
   - Categories: Utilities / Security
   - Description: see [README.md](../../README.md)
   - Icon: `icons/256x256/harbour-ncpasswords.png`
   - Privacy policy: <https://github.com/protongit/harbour-ncpasswords/blob/main/PRIVACY.md>
   - Screenshots: 2–4 images showing the vault, entry detail, TOTP and settings
4. Answer the QA questions about Sailfish Secrets: the app password is stored
   there so it does not have to be kept in plain text; the vault master
   password is never stored.
5. After approval, publish the release in the Store app.

If you also want to support older 32-bit devices, build an `armv7hl` RPM with
the corresponding SDK target and upload it too.
