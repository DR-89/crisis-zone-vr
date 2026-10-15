# Crisis Zone VR for Windows

Extract the entire package before launching. Game content and weapon models are bundled.

Connect your OpenXR headset first, then run **Play VR.cmd**. The active runtime must support OpenGL. **Play SteamVR.cmd** selects an installed SteamVR runtime for this launch. Use **Play Desktop.cmd** to run on a monitor.

Press A for credits, then trigger to start. Either trigger selects the weapon hand. Hold trigger for automatic fire, hold either grip to leave cover, release grips to hide/reload. The left menu button opens English VR options; world size, gun angle, view distance, handedness, aiming guide and physical ducking match the earlier Time Crisis ports.

Desktop: mouse aim, left button fire, right button or Space pedal, 5 credits, Escape options.

Version 0.1.4 uses the blue TC2 arcade pistol model and corrected zone-selection ticker. Crisis Zone's automatic fire and reload mechanics still apply. See [VALIDATION.md](VALIDATION.md) for tested behavior and known limitations. Windows desktop mode has been tested; Windows headset mode still requires validation. Logs are written to `tc2.log` beside the executable. Keep the entire package together, including `extracted`, `models` and `openxr_loader.dll`.
