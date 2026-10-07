# Release process

`VERSION` is the native/package version source; desktop package metadata and rendered footer must match. `1.0.0-rc.1` is a candidate, not stable 1.0. Run `python3 scripts/check-release.py` before packaging.

Work on a feature branch and submit a PR. The existing main ruleset requires linear history, resolved review threads and **Build and test**. Do not bypass failing checks or overwrite release tags. CodeQL/dependency review findings must be assessed even though only Build and test was historically required by the ruleset.

Local release gates: native Release/UBSan tests, exact v4/v5/v6/v7 and HTTP regressions, job journal resume/delivery tests, signal/route provider freshness tests, desktop isolation/background/cache tests, browser layout and report/export checks, pinned Windows DLL build, npm audit, secret scan and archive inspection. Keep fixture, rendered, actual driving and physical-controller evidence distinct.

Build the native binary, Windows DLL and standalone package, then run `ats-dualsense/scripts/package-release.sh`. The archive contains the standalone app, native/source/plugin/installer files, third-party licenses and documentation. Exclude maps, extraction output, local job reports, user config, IDE files, npm caches and signing secrets. Retain SHA-256 checksums. Electron packages disable RunAsNode, NODE_OPTIONS, Node CLI inspection and directory fallback through fuses; Linux ASAR signing/integrity is not claimed.

A draft/prerelease can be prepared from the reviewed candidate branch. Publish stable 1.0 only after passing remote gates and recording live v7 delivery/cancellation, actual signal/GPS alignment after restart, a long-haul background/performance run and the remaining device/transport qualification. The release notes must list any surviving limits. Never promote fixture-only traffic or job evidence to a real driving claim.
