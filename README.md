# Crisis Zone VR

Native **Crisis Zone (arcade, World CSZO4 Ver. B)** for **Meta Quest 3** and **Windows OpenXR PCVR**. Built from the Crisis Zone 0.9.0 release of spacestate1/namco22-decompile and the VR host of DR-89/time-crisis-2-vr.

**[Download v0.1.7](https://github.com/DR-89/crisis-zone-vr/releases/tag/v0.1.7)** â€” Quest APK and Windows x64 package, with SHA-256 checksums. Game content is bundled in both packages. On Quest, the app installs the bundled files into its private storage before starting VR; no file picker or importer is required.

The player uses the blue Namco arcade pistol from the TC2 VR port. Its calibrated grip, muzzle, recoil and flash apply in either hand. Crisis Zone retains its automatic fire, ammunition and cover/reload mechanics.

Version **0.1.7** adds saved per-eye resolution presets on the **GRAPHICS** options page, a default-on texture-filtering toggle, and launcher icons/banners. See the [0.1.7 release notes](docs/RELEASE_NOTES_0.1.7.md).

Zone selection appears on one VR panel with a stationary title/countdown and a continuously scrolling ticker. Original aiming cursors and hit flashes project onto scene surfaces; gameplay uses stereo geometry. Updates retain saved options and cabinet data.

This is an unofficial single-player arcade port. See [release notes](docs/RELEASE_NOTES_0.1.7.md), [validation and limitations](docs/VALIDATION.md), and [weapon model details](docs/WEAPON_MODELS.md).

## Install

- Quest: sideload `CrisisZoneVR-v0.1.7-quest3.apk`, then launch **Crisis Zone VR (Test)** in Unknown sources. Package: `org.crisiszone.vr`; installs alongside both Time Crisis ports. The first launch extracts the bundled content.
- Windows: extract the entire Windows ZIP. Connect an OpenXR headset, then use **Play VR.cmd**. **Play SteamVR.cmd** selects an installed SteamVR runtime for this launch. **Play Desktop.cmd** starts a normal monitor window. An ADB USB connection does not provide PCVR.

The Windows runtime must support OpenGL. Quest Link/Virtual Desktop/SteamVR compatibility and headset performance must be checked with this build; results from the earlier ports are not Crisis Zone results.

### Sideload on Quest from a Windows PC

1. Enable **Developer Mode** for your headset in the Meta Horizon phone app under **Headset Settings**. First-time setup requires a verified Meta developer account/team; follow [Meta's setup guide](https://developers.meta.com/vr/documentation/native/android/mobile-device-setup/) for this and the Windows ADB driver.
2. Download `CrisisZoneVR-v0.1.7-quest3.apk` from the [release page](https://github.com/DR-89/crisis-zone-vr/releases/tag/v0.1.7).
3. Download and extract [Android SDK Platform-Tools for Windows](https://developer.android.com/tools/releases/platform-tools), then copy the APK into its `platform-tools` folder.
4. Connect the Quest with a **USB data cable**, put on the headset and accept **Allow USB debugging** for your PC.
5. Open PowerShell in the `platform-tools` folder and run:

   ```powershell
   .\adb.exe install -r .\CrisisZoneVR-v0.1.7-quest3.apk
   ```

6. After `Success`, open the headset's app library, select **Unknown sources**, and launch **Crisis Zone VR (Test)**. The first launch extracts the bundled game content.

For updates, install the new APK over the existing app with the same command. The `-r` option keeps your settings and game data; no uninstall is needed.

## Controls and options

**Render resolution:** open options with the **left menu button**, then use **left stick up/down** to find **GRAPHICS**. Move the **right stick left/right** to choose **Default**, **1280 x 1344**, **1680 x 1760**, **2550 x 2640**, **2940 x 3080** or **3360 x 3520** pixels per eye, matching the NFSU VR ports. Click the **left stick** to restore Default. Each change saves automatically; **fully close and restart the app** to apply it. The panel shows the active size and a restart reminder. Default preserves the previous resolution (runtime recommendation capped at 1440 pixels on the longer side on Quest, 2160 on PCVR). Unsupported presets fall back to Default. Higher resolutions require more GPU time and memory; the highest setting is not a performance recommendation. Texture filtering stays enabled by default and still switches immediately.

**Texture filtering:** open the VR options with the **left menu button**, then squeeze and release the **left grip** to switch **TEXTURE FILTERING: ON / OFF**. It defaults to **ON**, including when upgrading an older settings file, and saves automatically. Changes apply immediately; no restart is needed. This control uses the physical left controller in either handedness layout. During gameplay, grips still operate cover. Filtering smooths world textures; HUD and text retain their existing sampling.

The default hand controls the face-button mapping. Either trigger transfers the weapon to that hand. Hold the trigger for machine-gun fire; hold either grip to leave cover and release both to hide/reload. Press **A** (or **X** with left-handed layout) for credits and the trigger to start.

| Action | Default right hand | Default left hand |
| --- | --- | --- |
| Credits | A | X |
| LASER / OFF / TRACER | B | Y |
| Recenter / upright calibration | X | A |
| Open / close English options | Left menu | Left menu |
| Change default hand in options | Right stick click | Right stick click |
| Physical ducking / grip cover in options | Y | B |

Options retain the previous ports' **GUN ANGLE** (-60 to +60 degrees), **WORLD SIZE** (25â€“400%), **VIEW DISTANCE** (0â€“100 cm), physical ducking and gun grip offsets (Â±200 mm). Right stick changes scale/angle; left stick left/right changes distance and up/down cycles the world, gun-fit and graphics pages. On the gun-fit page the right stick adjusts up/back offsets and left stick click resets them. Settings save automatically.

Crisis Zone arcade uses the blue TC2 pistol model for its machine-gun player weapon. Its continuous recoil output drives repeated visual and haptic feedback. There is no TC2 player-route selector. The initial world scale is provisional and can be adjusted in options.

Desktop: mouse aim, left button fire, right button or Space for cover/pedal, **5** credits, **Escape** options.

## Development

The `upstream/` snapshot comes from the explicit 0.9.0 source release asset, **not** GitHub's automatically generated tag archive (that tag points to an older source revision). Archive SHA-256 and VR base commit are in `upstream/SOURCE.json`.

Build on Windows with Python 3 and CMake on `PATH`. The build scripts download pinned SDL, OpenXR, Ninja and LLVM/MinGW dependencies into `.tools/` as needed. For the Quest build, install a JDK and set `JAVA_HOME` (the default is Android Studio's bundled `jbr`), then install Android SDK platform 35, build-tools 35.0.0 and NDK 29.0.14206865 into `.tools/android-sdk`. With Android's `sdkmanager` available, run from the repository root:

```powershell
sdkmanager --sdk_root="$PWD/.tools/android-sdk" "platforms;android-35" "build-tools;35.0.0" "ndk;29.0.14206865"
```

Place your World CSZO4 Ver. B ROM set in Downloads or pass its path explicitly. Build either target:

```powershell
python -X utf8 tools/build.py --rom "$env:USERPROFILE\Downloads\crszone.zip" --jobs 2
python -X utf8 tools/build_pc.py --rom "$env:USERPROFILE\Downloads\crszone.zip" --jobs 2
```

Keep `namco_csz1.zip` beside `crszone.zip` when its I/O ROM is separate. Build preparation verifies all 23 chip sizes and CRCs and generates both H8 programs. SHA-256 manifests also verify the packaged content. Game files, keys and outputs are ignored by Git.

Quest logs: private `files/crisiszone-vr.log` and `files/crisiszone-vr-previous.log`. Windows logs: `tc2.log`; cabinet recordings/settings use the upstream `crszone` tag. Validation results are in `docs/VALIDATION.md`.

## Support development

If you enjoy this project and would like to support further development of my VR ports, you can support my work on Patreon:

[https://www.patreon.com/c/OpenSourceDevDirk](https://www.patreon.com/c/OpenSourceDevDirk)

## Credits

- spacestate1/namco22-decompile: native System 23 / Evolution 2 reconstruction.
- DR-89/time-crisis-2-vr and DR-89/time-crisis-vr: OpenXR, stereo renderer, settings and controller integration.
- Martoscar: blue Namco arcade gun (CC BY 4.0). FireWarden: fallback pistol (CC0). Full attribution and the preserved TMP model's provenance are in `NOTICE.md`.

Port code is MIT licensed. Original game programs, graphics, sound and trademarks retain their original rights. See `NOTICE.md` and bundled dependency licenses.
