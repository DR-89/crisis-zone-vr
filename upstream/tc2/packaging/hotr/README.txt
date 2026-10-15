Gun recoil with Hook of the Reaper (or MAMEHooker)

Time Crisis 2 and Crisis Zone send their outputs the way MAME does: on tcp port 8000 of this computer,
announced as "timecrs2" / "crszone". Display > "Recoil outputs (Hook of the Reaper)" turns this on or off
(it is on by default; turn it off if you run MAME at the same time).

MAME itself does not pass these games' gun solenoid on, so no Hook of the Reaper game file for them maps
it. The two files here do: copy timecrs2.txt and crszone.txt into Hook of the Reaper's defaultLG folder
and set "Use Default Light Gun files first" in its Settings. Time Crisis 2: one recoil per shot the game
accepts (none when out of ammo or in cover). Crisis Zone has a machine gun: its signal stays on for the
whole burst, and its file uses Recoil_R2S, which repeats the recoil while you fire (the delay between kicks
is in your gun's .hor file in Hook of the Reaper's data folder).

Signals the games send:
  gun_recoil     1 while the gun's recoil is on (Time Crisis 2: a 3-frame pulse per shot; Crisis Zone: the whole burst)
  out0..out2     the I/O board's three output bits (out0 = the recoil; Crisis Zone: out1 = the gun is live in play)
  analog0        Crisis Zone only: the I/O board's analog output
  lamp0..lamp7   the main board's lamps, the names MAME uses -- only with Display > Lamp outputs ON (off by default:
                 they change all the time, and the extra traffic is not needed for recoil)
