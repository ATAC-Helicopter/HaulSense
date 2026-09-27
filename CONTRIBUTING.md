# Contributing

Please describe the actual device, transport, kernel, game version and whether a result is from a fixture, rendered UI or real driving. Read the README build/test commands and the architecture before changing wire data.

Use focused pull requests. Keep the shared channel registry as the source of truth and change the wire version when the layout changes. Never add blocking I/O or unbounded queues to the feedback loop. Preserve user's settings and backwards migration. SDK coverage must distinguish unavailable data and synthetic cues.

Main is protected: pass the Build and test workflow, resolve review conversations and keep history linear. A maintainer may review without an artificial mandatory second reviewer for this solo project. Do not bypass failed checks. No force pushes or branch deletion on main.

UI changes should include honest screenshots and rendered verification where available. Hardware-sensitive changes need actual device evidence before they can be promoted as stable. Build output, generated DLLs and user logs do not belong in Git.
