# Crisis Zone VR 0.1.5 — local test build

- Enabled palette-aware bilinear filtering for world textures on Quest and
  Windows. Neighbouring colours are resolved independently before interpolation,
  including packed palette modes, tile flips and alpha pens. This softens texture
  steps and shimmer; it is not mipmapping and cannot eliminate distant aliasing.
- Kept menu/HUD polygons, text, the ticker and stencilled cutouts on their original
  sampling path.
- Added Android launcher icons and a banner for launchers such as QGO. Both
  legacy density-specific PNGs and adaptive icons are included, with explicit
  application and activity references.
- Android version code 6. Existing package identity and signing key are retained
  for installation over 0.1.4 without uninstalling.

Validation: Android signing/alignment, ROM/model integrity, production GLES
captures and ROM-free palette/filter checks. Headset performance and QGO display
still need confirmation on the connected Quest. This build is not published.

Developer comparison: set `TC2_FILTER=0` before launching on Windows to restore
nearest sampling. The default is filtered. This is a diagnostic environment
override, not a new saved menu option.
