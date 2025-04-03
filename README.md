## H4ptiX

The **H4ptiX** is a hardware/software solution designed to simulate physical interactions with **YubiKey** during automated testing. This device allows test frameworks to programmatically "touch" the YubiKey, enabling end-to-end (E2E) testing of multi-factor authentication (MFA) flows without manual intervention.

![](docs/irl.jpg)

#### Key Features
- **Physical YubiKey Interaction**: A capacitive touch mechanism triggers the YubiKey sensor as if pressed by a user.
- **Programmable Control**: Integrates with test frameworks to simulate button presses on demand.
- **Multi-Device Support**: Works with various YubiKey models, but I've used [Yubikey nano 5](https://www.yubico.com/th/product/yubikey-5-series/yubikey-5-nano/)
- **Test Automation Integration**: Compatible with CI/CD pipelines for unattended MFA/PIV/FIDO2 testing.

#### Use Cases
- Automated testing of **WebAuthn**, **U2F**, **TOTP**, and **OTP-based authentication**.
- CI/CD pipelines requiring **MFA testing** without human intervention.
- Load testing scenarios involving **YubiKey hardware authentication**.

#### Hardware Implementation

  - [Waveshare RP2040-Zero](https://www.waveshare.com/wiki/RP2040-Zero) (Raspberry Pi RP2040, USB-C, compact footprint)
  - [USB Hub: ORICO 4-Port USB 3.0](https://oricotechs.com/th/products/orico-4-port-usb-3-0-clamp-design-mountable-hub)
  - Custom PCB

#### Benefits
✔ **Eliminates manual MFA steps** in automated tests.
✔ **Improves test coverage** for security-critical flows.
✔ **Reduces flakiness** compared to software emulators.
✔ **Cost-effective** compared to commercial  automation tools.
