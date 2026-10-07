# ETS2LA memory provider

The standalone build includes the upstream MIT Windows DLL for ATS/ETS2 **1.61.x**, fetched at build time and verified by SHA-256. Its own MIT license is distributed beside the DLL. The HaulSense renderer is original code.

Binary: `ETS2LA/ETS2LA`, commit `41945bc41e36191fbdf277c7fa6f7dd6f101d679`, `Assets/SDKs/1.61/Windows/ets2la_plugin.dll`.

SHA-256: `0e1893719f84f28079857451b82a22a263401d98b8fc299cd561eb7695bfa88f`.

License/source: [dariowouters/ets2la_plugin](https://github.com/dariowouters/ets2la_plugin/tree/c925bd965a96c58b73eaea9ca033fe0f0dc623a7).

The app installs the DLL automatically only when an embedded map verifies the installed game content and identifies a supported 1.61.x version. An existing different DLL is backed up. The game must restart after a new installation. No running-game memory writes or driving controls are performed by HaulSense.
