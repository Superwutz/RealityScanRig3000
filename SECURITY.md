# Security Policy

## Supported Versions

Security fixes are provided for the latest release line only.

- Supported: latest `v0.x.y`
- Not supported: older tags and unreleased historical commits

## Reporting a Vulnerability

Please do not open public issues for security vulnerabilities.

Instead, contact the maintainer privately with:
- A short description of the issue
- Impact and affected versions
- Reproduction steps
- Any suggested mitigation

The project will acknowledge the report, validate it, and publish a fix in a new release as quickly as possible.

## Scope Notes

Common security-sensitive areas in this project:
- Wi-Fi credentials handling
- OTA update flow and manifest source
- Local AP fallback behavior
- Web UI endpoints and command interface
