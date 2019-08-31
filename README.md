# corner_sensitivity: Adjust joystick corner sensitivity for th17
th17 has a problem where the game is too sensitive on what it considers a corner on a joystick. Older games were much more lenient

Copy corner_sensitivity.ini to your th17 folder and edit it to change configuration. To apply the patch you need to manually inject this DLL and detour XInputGetState. Instructions may never appear because this is supposed to use the currently unimplemented patch level DLL loading in thcrap
