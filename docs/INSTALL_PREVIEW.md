# SkellFan Windows VST3 private preview

This is a **private, unsigned Windows x64 VST3 preview**, not a public or commercial release. The ZIP includes the single **SkellFan.vst3** bundle, provenance, SHA256SUMS, this guide and the PowerShell 7 installer.

Preview first:

~~~powershell
pwsh -NoProfile -File .\Install-SkellFanPreview.ps1 -PackageRoot . -WhatIf
~~~

The default per-user destination is **%LOCALAPPDATA%\Programs\Common\VST3\SkellFan.vst3**.

Install only after reviewing the preview:

~~~powershell
pwsh -NoProfile -File .\Install-SkellFanPreview.ps1 -PackageRoot . -Approve
~~~

The installer validates product/version/vendor, VST3 class IDs, AMD64 PE32+, pluginval provenance and all five SHA-256 entries. It refuses unsafe paths, tampered packages, and existing installations it cannot prove it owns. Updates are staged and the previous verified bundle is backed up before replacement. Rollback validates the selected backup and preserves the current version.

Backups are stored under **%LOCALAPPDATA%\Skell Audio\SkellFan\Backups**. Restore one explicitly with the **-RollbackBackup** parameter and **-Approve**.

No administrator elevation, registry writes, global VST3 path changes, FL Studio project changes, preset migrations, or automation-ID changes are performed. FL Studio scan/load, real-audio A/B, old-project/state recall, target-hardware CPU/latency, licensing/attribution and support contact remain unverified.
