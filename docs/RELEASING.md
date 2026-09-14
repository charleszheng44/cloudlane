# Publishing Cloudlane

The repository is being prepared for open source. It remains a private development repository until the owner chooses to publish it. No marketplace submission or approval has occurred.

## Release preparation

- Run the offline regression suite, native graphical test and `scripts/check-plugin.sh`. Validate the root manifest with `omarchy plugin validate .`; validate the desktop file and installation/removal in a temporary prefix.
- Review tracked files and Git history for private account data, signed URLs, local filesystem paths and screenshots. Only the fixture-based `preview.png` belongs in the public README. Do not distribute the build directory.
- Preserve the GPL-3.0-or-later license and all third-party notices. Distribute corresponding source for binary releases and document the system-library dependencies. The VCS PKGBUILD follows current repository HEAD and is not a reproducible release pin; release packaging must pin its source revision.
- Record tested features and remaining gaps in `IMPLEMENTATION.md`. Do not call this full NetEase parity. Native x86_64 has been exercised locally; other architectures and a clean package-manager installation need their own acceptance.
- Confirm a complete hosted CI run passes; the initial run could not allocate a runner and executed no build/test steps.
- Review the exact release diff, choose a version, and synchronize native app/manifest/package versions. A marketplace manifest requires a three-part version even while the app is a development preview.
- When ready, change GitHub visibility to public, enable private vulnerability reporting, confirm the README and image render, and record the exact commit for review. Do not publish account acceptance artifacts.

## Marketplace preparation

The [Omarchy plugin development guide](https://plugins.omarchy.org/develop.html) and [submission guide](https://github.com/omacom/omarchy-plugin-marketplace/blob/main/SUBMISSION.md) require a public GitHub root containing one plugin, a root manifest, README, license, installation/removal instructions and external dependency documentation.

Cloudlane's permanent plugin ID is `io.github.charleszheng44.cloudlane`. The native app is a separately built dependency; `plugin/BarWidget.qml` is the actual Quattro shell entry point. Plugin installation does not run build hooks. Ask reviewers to classify this as requiring **manual setup**. Before submission, search the catalog for the exact ID to confirm it is still available.

The marketplace's compatibility check and Automated Security Baseline operate on an exact commit. The local baseline preflight reported no deterministic findings and review-required installer, privilege, package-manager and remote-build capabilities (the README documents optional package installation). These capabilities require maintainer review. A local manifest validation or static preflight is not marketplace verification or a security audit. New listings require a fresh scan and an explicit maintainer `approved-and-verified` decision.

## Draft submission

Title: `[Plugin]: Cloudlane`

This is a draft, not a submitted issue. Leave the checklist unchecked until the owner has reviewed every statement and the repository is public. Obtain the owner's explicit approval of the completed issue before creating it.

### Repository URL

https://github.com/charleszheng44/cloudlane

### Category

Widgets

### Tags

bar, launcher, media

### Suggest a missing tag

_No response_

### Maintainer notes

Native Qt6/libmpv NetEase Cloud Music app with an optional Omarchy Quattro bar launcher and MPRIS controls. Manual setup: build/install the desktop app and system dependencies as documented before enabling the widget. GPL-3.0-or-later, with a preserved MIT consumer-crypto reference notice. Independent development preview; no official NetEase partnership or complete feature-parity claim. The preview uses fictional account/music data and original artwork. Install/build and package-manager capabilities are documented. Request review of the exact submitted commit.

### Submission checklist

- [ ] The repository is public and contains installation and removal instructions.
- [ ] I have documented the plugin license and any external dependencies.
- [ ] I confirm that I own or have permission to submit this plugin and its preview assets.
- [ ] The plugin does not overwrite user configuration without explicit consent.
- [ ] I understand that approval is for listing and is not a security review.
