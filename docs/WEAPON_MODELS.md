# Weapon models

## Current player model

Version 0.1.4 selects the blue Namco arcade pistol from Time Crisis II Arcade VR at startup and every game frame, as requested by the user. The existing `namco-gun.tcgun` is byte-identical to that port's asset (SHA-256 `4a8231dcc6ba717d61ab5783156c3a0d0367572ddb54537b0b283415da12fab8`). It has 13,149 vertices, 17,108 triangles and a 1024-square color atlas; its calibrated muzzle is `(0, 0.054069425, -0.173751)` metres relative to the controller aim pose. Existing grip/angle settings, left/right handoff, whole-model recoil and muzzle flash apply. Crisis Zone's game-controlled automatic fire and cover/reload mechanics remain intact. Attribution and conversion details are in `NOTICE.md` and `quest/assets/models/namco-gun.json`.

## Generated models

Four exterior game props were generated in the user's existing Chrome / Tripo Studio session on 15 October 2026. Textures were generated in Tripo, and the user downloaded the completed GLB exports. Source files are preserved under the ignored `artifacts/weapon-models/` directory, with source checksums and project IDs in `SOURCES.json`.

| Model | Tripo project | Use in this port |
| --- | --- | --- |
| Steyr TMP | `b1d7e6bf-abdc-4c08-9573-2a4a52f9c05f` | Used in 0.1.2–0.1.3; preserved but not selected in 0.1.4 |
| Handgun | `d3d9198d-1a5a-4bed-9206-df6f31d0a637` | Textured GLB source prepared; no corresponding arcade weapon state |
| Shotgun | `1264c97c-93c1-43f4-b476-48d09d111140` | Textured GLB source prepared; no corresponding arcade weapon state |
| M134 / Gatling gun | `55324cae-0485-4049-a8a6-3d24acd1fab0` | Textured GLB source prepared; no corresponding arcade weapon state |

This build runs the original CSZO4 arcade program. It has one player weapon. The extra playable weapons and their switching system belong to the PS2 version; the arcade port cannot automatically select those states. No manual cosmetic selector, invented weapon states or altered firing mechanics have been added.

The TMP's first text-only generation was rejected because its silhouette did not resemble a TMP. The final model used [Spectrums' public-domain photo](https://commons.wikimedia.org/wiki/File:Steyr_TMP_9mmPara_001.jpg) as the visual reference. The generated models are visual approximations, not exact replicas.

## Runtime conversion

The preserved TMP uses the existing `machine-gun.tcgun` slot and is not loaded by the current host. Its muzzle is calibrated to `(0, 0, -0.159018)` metres in controller aim space, with +Y up and -Z forward. Visual length is 0.282 m; geometry has 4,778 vertices and 6,000 triangles with one 1024-square base-color atlas; full source textures remain in the GLB.

```powershell
python tools/import_machine_gun.py artifacts/weapon-models/CrisisZone-Steyr-TMP.glb quest/assets/models/machine-gun.tcgun --calibration tools/calibration/steyr-tmp.json --triangles 6000 --atlas-size 1024
python tests/render_weapon.py quest/assets/models/machine-gun.tcgun --output artifacts/weapon-models/tmp-preview
```

These offline tools require NumPy, Pillow and the pinned meshoptimizer dependency. The game runtime gains no new parser or dependencies. Calibration is tied to the exact downloaded GLB SHA-256 to reject a differently oriented export.
