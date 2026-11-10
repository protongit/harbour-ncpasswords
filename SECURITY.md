# Security policy

## Reporting a vulnerability

Please **do not** open a public issue for security problems.

Use the repository's **Security → Report a vulnerability** form so the report
and the discussion stay private. Include the affected version, Sailfish OS
version and a short description or proof of concept.

You can expect an acknowledgement within a few days. Once a fix is available,
the issue and the release notes will be published.

## Scope

The app stores credentials and talks to Nextcloud servers, so reports about
cryptography, secret storage, clipboard handling, logging of sensitive data and
network behaviour are especially welcome.

The following are known limitations rather than vulnerabilities:

- Fingerprint unlock is not available to sandboxed third-party apps on current
  Sailfish OS versions; the app uses a PIN instead.
- If the vault master password is lost it cannot be recovered.
- Data is protected by the app PIN and the operating system's sandbox, not by
  full-disk encryption alone.
