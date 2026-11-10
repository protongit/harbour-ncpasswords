# Privacy policy

NC Passwords is a local client for the Nextcloud Passwords app. It does not
collect, transmit or sell personal data.

## Data the app handles

- **Server address, username and app password**: used only to authenticate
  against the Nextcloud server that you enter. The app password is stored in
  Sailfish Secrets when available, otherwise in a `0600` file inside the app's
  private data directory.
- **Vault data** (passwords, folders, tags, notes and TOTP secrets): downloaded
  from your Nextcloud server and decrypted on the device. It is never sent to
  the developer or to any third party.
- **Master password**: used in memory to derive the encryption key. It is never
  written to disk.
- **Clipboard**: the app copies only what you explicitly copy, and clears the
  clipboard after the timeout configured in Settings.

## Network connections

The only network connections are made to the Nextcloud server you configure.
There is no telemetry, analytics, crash reporting, advertising or update check.

## Permissions

- `Internet`: communicate with your Nextcloud server.
- `Secrets`: store the Nextcloud app password in Sailfish Secrets.

The app does not request access to contacts, location, camera, microphone or
files.

## Data removal

Removing the account in Settings clears the stored credentials and local
settings. Uninstalling the app removes its private data directory.

## Contact

Questions or reports: <https://github.com/protongit/harbour-ncpasswords/issues>
