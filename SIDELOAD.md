# Sideloading SM64CoopDX iOS

This document is not an exact step-by-step of how to sideload. It briefly shows what the usual process looks like, and how to get updated instructions for your exact setup.

## Frequently asked questions

- **Do I need an Apple PC to Sideload?** - Using an Apple PC is the simplest option, but using Windows and Linux is also possible. Sideloading without a PC is in theory possible with [Feather](https://feather-ios.com/) but I have never tried it.
- **Is Jailbreaking needed? Is it a risky process?** - No jailbreaking or any risky process needs to be performed, this is fully supported by Apple.
- **Do I need the ROM to Sideload?** - Not for the sideloading process itself, once the app is installed it will automatically ask you to select the ROM from your iPhone/iPad Files.

## Typical Sideloading process

### Option 1: add the source in AltStore / SideStore

If you use [AltStore](https://altstore.io/) or [SideStore](https://sidestore.io/), the easiest way to install and get update notifications automatically is to add this source:

```
https://raw.githubusercontent.com/LeoManrique/sm64coopdx-ios/main/apps.json
```

In the app, go to **Sources → +**, paste that URL, then install SM64CoopDX from the source. New releases will show up as updates without re-downloading anything manually.

*SideStore* apparently has the option to automatically re-install the app, so it doesn't expire every 7 day. I haven't tested this myself yet.

### Option 2: Install .ipa directly

1. Download the `.ipa` from [Releases](https://github.com/LeoManrique/sm64coopdx-ios/releases/latest).
2. Use a sideloading tool  ([AltStore](https://altstore.io/), [Sideloadly](https://sideloadly.io/), [SideStore](https://sidestore.io/), or similar) to install it on your device, signed with your Apple ID.
3. Trust the certificate on the device, re-sign when needed.

_Note: Using an Apple PC is the simplest option, but using Windows and Linux is also possible._

## Get updated instructions

Either do a web search including your setup details, or paste this prompt into your favorite LLM with web search turned on. Change the content between brackets to match your setup.

```
I want to sideload an iOS app distributed as a .ipa file onto my [iPhone / iPad] running iOS [version].
My computer runs [macOS / Windows / Linux].
I have [a free Apple ID / a paid Apple Developer account / no Apple ID yet].
I live in [country].

Look up updated information about how to do this as of today.

Provide a detailed step by step of what to do.

Include:
- Which tools are currently considered best for my setup
- What I need to install on my computer (if anything)
- How long the signature lasts and how I refresh it

The app I'm sideloading is https://github.com/LeoManrique/sm64coopdx-ios/releases/latest.
Nothing jailbreak-related.
```

## Common after-install issue

You might get "Untrusted Developer" error on first launch. To fix it go to Settings > General > VPN & Device Management. Find the certificate matching the Apple ID you signed with and tap Trust.

## Other issues

Sideloading isn't always a seamless process. Look up your errors or update your LLM session with what you've tried so far and ask for help when stuck.
