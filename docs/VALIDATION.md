# Crisis Zone VR 0.1.4 validation

Release 0.1.4, 15 October 2026. These results describe Crisis Zone, not the earlier Time Crisis ports. The Quest user has confirmed that the current port plays well; the automated and device checks below record the scope of validation.

## 0.1.4 blue TC2 pistol

- Selected the existing blue Namco arcade pistol in both host startup and game-frame presentation. Its asset SHA-256 matches the Time Crisis II Arcade VR source asset exactly: `4a8231dcc6ba717d61ab5783156c3a0d0367572ddb54537b0b283415da12fab8`.
- The TMP remains preserved but is no longer loaded or selected. The game input/output path still supplies automatic firing, ammunition and cover/reload. Both controller hands use the selected pistol's calibrated muzzle; existing grip/angle options apply.
- The production GLES weapon renderer loaded the 13,149-vertex / 17,108-triangle pistol and rendered eight views with resting and recoil/flash poses without GL errors. The blue model was visually inspected; muzzle transforms passed for both hand positions and rotated controllers.
- Android ARM64 and Windows x64 builds succeeded. Both packages passed content/hash verification, including byte-identical blue pistol assets. APK signing and 16 KiB alignment passed.
- Installed 0.1.4 / code 5 on Quest 3. Saved options and cabinet-data hashes were unchanged by installation. The installed blue pistol asset matches the TC2 source hash. Startup reached focused OpenXR state with the Adreno renderer and selected blue pistol, then shut down normally at frame 171 with zero CPU traps / FP exceptions. This is a startup check; the model's in-headset comfort still requires user playtesting.

## 0.1.3 ticker correction

- Reproduced the user's split ticker in the captured zone-selection frames. The previous adapter's screen-y=448 boundary cut through the 32-pixel glyphs, moving only their lower halves. The ticker actually occupies source tile rows 27 and 28 (y=432..463).
- The adapter now applies scrolling to both complete tile rows in source coordinates. The title, timer and surrounding frame remain stationary. The menu detection and gameplay renderer are unchanged.
- A synthetic 32-pixel glyph test failed on the old adapter and passes after the fix, checking all 32 horizontal scroll phases and three vertical offsets (including wrap). All 22 captured menu frames match the original renderer across the full ticker band; 69 other frames retain byte-identical text and opacity maps.
- Eight captured frames were rendered through the production GLES code without GL errors, covering multiple ticker positions, zone previews and gameplay. The full-size corrected letters and contact sheet were visually inspected.
- Three menu snapshots were also rendered from each eye position (six VR views) without GL errors; both eyes' full-size ticker output was inspected.
- Android ARM64 and Windows x64 builds succeeded. Both packages passed bundled-content and weapon-hash verification; APK signing and 16 KiB alignment passed.
- Installed 0.1.3 / code 4 on Quest 3 over 0.1.2. Saved options and cabinet-data hashes were unchanged by installation. The native startup log confirms the TMP asset, Adreno renderer and focused OpenXR session. In-headset confirmation of the corrected ticker remains with the user; the visual checks above used captured frames on the desktop GLES renderer.

## 0.1.2 weapon model

- Inspected the four completed Tripo GLB exports: each has a valid scene, triangle geometry and embedded textures. The TMP's source SHA-256 is `21f0fc4754d81ee7b15a6879ecbb3ac40b4981e85b64998cfda0e9f297779915`.
- Replaced the arcade machine-gun asset with the image-guided Steyr TMP. The 90,181 source triangles were reduced to 6,000, with 4,778 vertices and a 1024-square texture. The source axis, muzzle cylinder and grip were inspected before conversion.
- Production `quest_gun.c` loaded the new asset and rendered eight views covering both sides, aiming perspective, recoil and muzzle flash on ANGLE GLES. All passed without GL errors. Muzzle transforms were checked for both hand positions and rotated controller poses. Rendered previews were visually inspected.
- The existing CSZO4 arcade player-weapon selection uses the TMP automatically. Extra generated models remain exported source assets because this arcade game has no handgun, shotgun or Gatling player-weapon states. No manual cosmetic selector was added.
- Android ARM64 and Windows x64 builds succeeded. Both packages passed ROM/content/hash verification, including byte-for-byte agreement between their weapon asset and the new source asset. APK signing and 16 KiB alignment passed. The production host's controller/handedness regression test also passed.
- Installed 0.1.2 / code 3 on Quest 3 over the previous version. Settings and cabinet-data hashes were unchanged by installation. The launched native process reached OpenXR focused state, initialized the Adreno renderer and logged the new 4,778-vertex / 6,000-triangle weapon. Its installed asset SHA-256 matched `cae05db2968d07cad6f1c347db54471c92e00539718f4074fc23c06f06a7e71f`. This is a startup check, not a gameplay comfort test.
- Physical grip comfort and appearance on the headset still need user playtesting. Earlier menu and aiming fixes remain in this version.

## 0.1.1 feedback fixes

- The user's Quest 3 session confirmed native VR gameplay. Its log shows the OpenXR session entering focused state, both controller hands used, and 120 Hz runtime presentation cadence. This does not establish sustained 120 FPS game simulation; the arcade remains 59.906 Hz.
- Replayed the actual recorded inputs on Windows and reproduced the zone-selection title/countdown movement. The shared 0.9.0 host applies the bottom ticker's residual 0..31-pixel scroll to the whole text layer. The initial menu adapter kept rows 0..447 fixed, but its boundary incorrectly split the ticker glyphs; the original row-comparison test did not catch this. Version 0.1.3 corrects the band and adds a whole-glyph regression check.
- Zone selection is drawn on one 2.5 m panel, including its cursor and building choices. Gameplay remains stereo geometry.
- Original cursors and rotating hit flashes use a synthetic layout depth of about 0.047 game units. They now project onto the corresponding scene surface, are excluded from aiming collision, and scale/move with that surface. Score popups 9112/9121 use the same surface projection. Unrelated world effects remain 3D.
- A controller ray and the original marker/hit-flash geometry agree at the target in regression cases for both hands, world scale 25/100/400% and view distance 0/40/100 cm. Real captured menu/game frames render without GLES errors; paused changes match fresh geometry uploads.
- The game retains its original input/shot latency and automatic-fire spread. Reprojection uses currently submitted world geometry, not an independent game collision model. Corrected in-headset visual alignment still needs user confirmation.
- Post-build device check: installed version 0.1.1 / code 2 over 0.1.0 on Quest 3; options and cabinet-data SHA-256 hashes were unchanged by installation. The new native session reached the corrected zone-selection panel and shut down normally at frame 1,749 with zero CPU traps / FP exceptions. A Windows replay of the supplied session completed 4,201 frames with zero CPU traps / FP exceptions. Device logs are saved locally under `build/device-feedback/`.

## Initial 0.1.0 baseline

## Build and packaging

- Android ARM64 native build succeeded with NDK 29.0.14206865. APK signing verified (v3), as did 16 KiB ZIP alignment.
- Windows x64 LLVM/MinGW native build and portable ZIP succeeded.
- Both packages contain all 23 required CSZO4 Ver. B game/I/O ROM chips. Sizes, CRC32 at preparation and packaged SHA-256 hashes match the supplied archives. Both ZIP containers passed full decompression/integrity checks.
- APK package `org.crisiszone.vr`, version 0.1.0 / code 1, launches MainActivity directly with OpenXR/Quest VR categories. No setup/import activity or network permission. All three ARM64 native libraries, DEX and weapon model hashes were checked.
- Game files are bundled in release packages as requested. They remain outside the Git source tree. Source and dependency notices are packaged.

## Automated behavior and rendering

- Actual host input code exercised against a mock OpenXR runtime: both hands, trigger handoff, tracking/focus loss, cover, recentering, aim alignment, saved angle/scale/distance and grip offsets. Crisis Zone gun port endpoints are 64..831 / 32..255. No TC2 alternate player route is selected.
- Settings persistence, malformed values, physical-cover hysteresis and 21 GLES options-panel variants passed; stereo views produced no GL errors. The English gun-fit page was visually inspected.
- Dedicated scene test checks that Crisis Zone ammunition, health, reload and pedal-prompt geometry stays at the same physical depth as the text HUD even when world size/distance changes. World effects with adjacent model IDs stay in the world. Timing preserves the arcade rate across 30–120 Hz presentation.
- Windows native desktop boot/attract sequence: **3,601 frames, zero CPU traps, zero FP exceptions**, normal shutdown. Captured original scene/text/texture output was visually inspected.
- Scripted Windows gameplay input: **10,801 frames, zero CPU traps, zero FP exceptions**, normal shutdown. Credits, game start, shield/cover, automatic firing, ammunition consumption, damage and scoring are visible in captured frames. This is a short first-area smoke test, not a full playthrough or aim-accuracy evaluation.
- Production GLES renderer exercised on ANGLE with captured Crisis Zone geometry: seven attract flat views, four attract scenes for each eye, nine gameplay flat views, four corrected gameplay stereo views, and paused world-size/distance changes. No GLES errors. Health/ammunition/reload prompts were checked after the Crisis Zone HUD fix. Native controller geometry/aim code is shared by Quest and PCVR.

## Limits still requiring playtesting

- The initial build was tested without a headset, then installed on Quest 3 and played by the user. Detailed controller comfort, audio quality, sustained performance and thermal testing remain outstanding. Windows headset mode is unverified; Windows runtime tests used desktop mode.
- World-size conversion starts with the previous System 23 calibration. Crisis Zone figure height and comfort need a headset check; users can adjust WORLD SIZE and VIEW DISTANCE.
- Wide tracked views can reveal the original game's missing scenery outside the arcade camera's submitted geometry. This is visible in some attract shots.
- The unchanged 0.9.0 shared C435 host reported **38,338 unknown commands** in the attract run and **636,550** in the gameplay run. The game continued rendering and had no CPU traps, but these results do not establish complete rendering accuracy. The VR layer does not suppress that counter.
- Only the captured HUD models are explicitly flattened. Later stages, other HUD/prompts, effects, full game completion, Quest 2/3S and additional PC runtimes need broader testing.
- Recoil pulses approximate a 10 Hz effect while the original arcade solenoid stays active; they are not separate game-engine shot events.

Detailed local logs and frame snapshots are under ignored `build/`; `tools/verify_release.py` repeats the package integrity audit. Original game snapshots and ROMs are deliberately excluded from source tracking.
