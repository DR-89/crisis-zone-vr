# Crisis Zone VR for Windows

**Render resolution:** open options with the **left menu button**, then use **left stick up/down** to find **GRAPHICS**. Move the **right stick left/right** to choose **Default**, **1280 x 1344**, **1680 x 1760**, **2550 x 2640**, **2940 x 3080** or **3360 x 3520** pixels per eye, matching the NFSU VR ports. Click the **left stick** to restore Default. Each change saves automatically; **fully close and restart the app** to apply it. The panel shows the active size and a restart reminder. Default preserves the previous resolution (runtime recommendation capped at 1440 pixels on the longer side on Quest, 2160 on PCVR). Unsupported presets fall back to Default. Higher resolutions require more GPU time and memory; the highest setting is not a performance recommendation. Texture filtering stays enabled by default and still switches immediately.

Keyboard: **Tab** changes the options page; on **GRAPHICS**, **Left/Right** selects resolution and **Home** restores Default. Resolution presets affect OpenXR VR rendering; desktop mode continues to follow the window size.

**Texture filtering:** texture filtering defaults to ON and is saved in `quest-options.cfg`. With options open, squeeze the **left grip**, or press **F** on the keyboard, to toggle it immediately. The left grip keeps its cover function during gameplay.

Extract the entire package before launching. Game content and weapon models are bundled.

Connect your OpenXR headset first, then run **Play VR.cmd**. The active runtime must support OpenGL. **Play SteamVR.cmd** selects an installed SteamVR runtime for this launch. Use **Play Desktop.cmd** to run on a monitor.

Press A for credits, then trigger to start. Either trigger selects the weapon hand. Hold trigger for automatic fire, hold either grip to leave cover, release grips to hide/reload. The left menu button opens English VR options; world size, gun angle, view distance, handedness, aiming guide and physical ducking match the earlier Time Crisis ports.

Desktop: mouse aim, left button fire, right button or Space pedal, 5 credits, Escape options.

Version 0.1.5 adds bilinear world-texture filtering to the blue TC2 arcade pistol and corrected zone-selection ticker from 0.1.4. Crisis Zone's automatic fire and reload mechanics still apply. See [VALIDATION.md](VALIDATION.md) for tested behavior and known limitations. Windows desktop mode has been tested; Windows headset mode still requires validation. Logs are written to `tc2.log` beside the executable. Keep the entire package together, including `extracted`, `models` and `openxr_loader.dll`.

## Support development

If you enjoy this project and would like to support further development of my VR ports, you can support my work on Patreon:

[https://www.patreon.com/c/OpenSourceDevDirk](https://www.patreon.com/c/OpenSourceDevDirk)
