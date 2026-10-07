# Security policy

Report privately through [GitHub Security advisories](https://github.com/FGLabs-dev/HaulSense/security/advisories/new). Private reporting is enabled. Do not publish exploitable details in a public issue before coordination.

The maintained line is **1.0.0-rc.1**; it remains a release candidate. Update the full app/service/plugin bundle together, especially after Electron or game-provider security fixes. Weekly dependency updates, dependency review, CodeQL, secret scanning and push protection support the repository; they do not constitute an independent security audit.

## Trust boundary

HaulSense runs as your unprivileged local user. Its HTTP/UDP endpoints bind to loopback. Do not expose or reverse-proxy them onto a LAN/Internet. HTTP checks Host, write Origin/custom header, duplicate security fields and transfer encoding; it grants no CORS access. Executable inline scripts are absent and CSP restricts scripts/workers to the local origin. A malicious process already running as your user can still modify local files/send telemetry; that boundary is not treated as authenticated game provenance.

The standalone renderer has sandbox and context isolation enabled, no Node/preload/IPC filesystem bridge, denied permissions and an allowlist of local protocol paths. Navigation/popups are restricted to the cockpit/HUD; exports require a native save selection. Runtime code comes from a packaged ASAR. Unneeded RunAsNode, NODE_OPTIONS and Node inspection fuses are disabled. No `--no-sandbox`, unsafe remote scripts or auto-update from arbitrary URLs is used.

Map imports are local and bounded/validated before use. Provider files require the current UID, regular-file type, no symlink, exact ABI size and stable reads. Unavailable/stale states are cleared. The optional ETS2LA DLL is downloaded from an immutable upstream revision with a pinned SHA-256; it depends on game internals and must match the game version. HaulSense reads its feeds and never writes driving overrides.

## Local data

The journal stores driving routes, cargo, cities and delivery/cost information under the user's data directory with private permissions. It keeps at most 50 completed reports and 10,000 downsampled route points per job. Map/preferences persist in the desktop/browser profile. There is no telemetry upload or cloud account. Exports disclose the selected report only when the user chooses a file.

Sanitize reports before sharing: remove account/config identifiers, paths and unnecessary route/job details. Never attach Steam credentials, tokens, full profiles, private journal exports or signing material. Local tests and scanners cover stated cases; no blanket guarantee of absence of vulnerabilities is made.
