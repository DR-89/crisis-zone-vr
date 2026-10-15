# Attribution and license scope

Crisis Zone VR is an unofficial port for Meta Quest 3 and Windows. It is not
affiliated with or endorsed by Namco, Bandai Namco, or Meta.

- **VR port code:** DR-89 and contributors, MIT; see [LICENSE](LICENSE).
- **Underlying game reconstruction and shared engine:**
  [spacestate1/namco22-decompile](https://github.com/spacestate1/namco22-decompile),
  using its 0.9.0 release source archive: `crszone/` and the `tc2/` shared host.
  The exact source archive hash is recorded in `upstream/SOURCE.json`.
  Its MIT notice credits **Copyright (c) 2026 cmcrann** and is preserved in
  the upstream snapshot and APK's `assets/licenses/namco22-LICENSE.txt`.
- **SDL 2.30.11:** zlib license; original notice included in the APK.
- **Khronos OpenXR loader 1.1.43:** its original license notice is included in the APK.
- **Windows OpenGL loader:** generated with [glad 2.0.8](https://github.com/Dav1dde/glad);
  generated code and Khronos declarations retain their notices in `pc/vendor/glad`.
- **Windows dependencies:** zlib 1.3.1, SDL 2.30.11, Khronos OpenXR 1.1.43,
  LLVM/MinGW runtime and winpthreads. Their notices are included under `licenses`
  in the portable Windows package.
- **Namco Arcade Gun model:** [Namco Arcade Gun by Martoscar](https://sketchfab.com/3d-models/namco-arcade-gun-15fbd5b9add94a34b5c21746e3dd32be),
  licensed under [Creative Commons Attribution 4.0 International](https://creativecommons.org/licenses/by/4.0/).
  Supplied by the user as `namco-arcade-gun.zip`. Converted for this port:
  axes and grip/muzzle calibration, reconstructed missing normals, baked base
  colors into a 1024-square atlas, omitted normal/roughness maps, combined mesh
  parts and added whole-model recoil. Original geometry is retained (17,108
  triangles). Source checksum and conversion details are recorded in
  `quest/assets/models/namco-gun.json`. This independently created model is not
  extracted from Time Crisis and is not covered by the port's MIT license.
- **Fallback player weapon model:** [Lowpoly Pistol by FireWarden](https://opengameart.org/content/lowpoly-pistol),
  released under CC0. Imported from the user-supplied QuestRetroDepth header, with
  separate body, trigger and slide parts. Provenance, geometry and checksums are
  recorded in `quest/assets/models/player-gun.json`. The model is not extracted
  from Time Crisis.
- **Steyr TMP player model:** generated in the user's Tripo Studio workspace
  on 15 October 2026, project `b1d7e6bf-abdc-4c08-9573-2a4a52f9c05f`, and
  exported as `CrisisZone-Steyr-TMP.glb`. Visual reference:
  [Steyr TMP 9mmPara 001 by Spectrums, revision by Luk](https://commons.wikimedia.org/wiki/File:Steyr_TMP_9mmPara_001.jpg),
  released into the public domain by its author. The 90,181-triangle source is
  reduced to 6,000 triangles with a padded 1024-square albedo atlas and calibrated
  grip, scale and muzzle axis. Source and output checksums are in
  `quest/assets/models/machine-gun.json`; reproducible calibration is in
  `tools/calibration/steyr-tmp.json`. This generated model is not an original
  arcade asset and is not covered by the port code's MIT license.
- **OpenXR host and controller features:** adapted from
  [DR-89/time-crisis-vr v0.8.4](https://github.com/DR-89/time-crisis-vr/tree/v0.8.4), MIT.
- **Crisis Zone, original game program, graphics, sound, ROM contents and trademarks:**
  belong to their respective original rights holders. They are not covered by this
  project's MIT license. The APK and Windows release packages include game files;
  inclusion does not place these assets under the port's MIT license.

The repository contains port and build tools plus the upstream source snapshot.
ROM archives, extracted ROMs, signing keys, generated build output and APKs are
excluded from Git. Release packages are created locally; no remote release is
published by the build scripts.

The immediate VR base is DR-89/time-crisis-2-vr; its commit is recorded in
upstream/SOURCE.json. The options and controller handling are adapted from that port.
