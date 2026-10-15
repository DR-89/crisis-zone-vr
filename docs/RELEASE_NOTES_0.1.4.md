# Crisis Zone VR v0.1.4

First public release of the native Crisis Zone arcade VR port for Meta Quest 3 and Windows OpenXR, based on spacestate1's namco22-decompile 0.9.0 reconstruction and the Time Crisis II VR host.

## Included

- Stereo VR gameplay with tracked controller aiming, either-hand weapon use, recoil and haptic feedback.
- Blue TC2 arcade pistol model, with Crisis Zone's automatic fire, ammunition and cover/reload behavior.
- English pause/options menu with handedness, gun angle, grip offsets, world size, view distance, laser/tracer and physical ducking controls.
- Corrected zone-selection title, countdown and scrolling ticker; original aiming cursors and hit flashes project onto the scene.
- Bundled game content and weapon models. No content importer is needed.
- Windows VR, SteamVR-runtime and desktop launchers.

## Downloads and installation

- **CrisisZoneVR-v0.1.4-quest3.apk**: sideload onto Quest, then open **Crisis Zone VR (Test)** from Unknown sources. Package: `org.crisiszone.vr`. Installing over an earlier build preserves settings and cabinet data.
- **CrisisZoneVR-v0.1.4-windows-x64.zip**: extract the entire archive. Run **Play VR.cmd** with an OpenXR headset, **Play SteamVR.cmd** to select an installed SteamVR runtime, or **Play Desktop.cmd** for a monitor window.
- **SHA256SUMS.txt**: checksums for both binaries.

On Quest, press **A** for credits and trigger to start (left-handed mapping: **X** for credits). Hold trigger to fire; hold either grip to leave cover and release both to hide/reload. The left menu button opens options. Desktop controls: mouse aim, left button fire, right button or Space for cover, **5** for credits and **Escape** for options.

## Validation and limits

The current port has been played and confirmed working on Quest 3. Native Android ARM64 and Windows x64 builds, packaged-content checks, APK signing/alignment, controller/scene tests and captured-frame rendering have been validated. Windows runtime checks used desktop mode; Windows headset compatibility and sustained performance still need broader testing. Wide views can expose scenery absent from the original arcade camera. This arcade version is single-player.

Full test results and known limitations: [VALIDATION.md](https://github.com/DR-89/crisis-zone-vr/blob/v0.1.4/docs/VALIDATION.md). Credits and license scope: [NOTICE.md](https://github.com/DR-89/crisis-zone-vr/blob/v0.1.4/NOTICE.md).
