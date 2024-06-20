# comboBurst

Combo Burst from osu! now implemented in Geometry Dash!  

# Custom Sprite Packs

If you want to add your own sprites, create a folder in the mod's save folder named "custom-sprite" and add your sprites there.
The mods save folder is located in `Settings -> Open Folder`  

On Android, the sprite folder is in `Geode/save/geode/mods/crewly.comboburst/custom-sprite/`  

### Filename Syntax

Each character needs to have two files:
1. `comboburst-1.png` - The sprite that will be displayed. (required)
2. `comboburst-1.wav` - The sound that will be played. (optional; but recommended)  
(Supported Audio Files: .ogg, .wav, .mp3, .m4a, .flic)  
(.ogg audio files are highly recommended for the best audio quality)  

All sprites with a running number starting from 1 will be loaded.  
(Example: `comboburst-1.png`, `comboburst-2.png`, `comboburst-3.png`, etc.)  

### Additional Sprite Pack Info

Any sprites that do not have their own audio file will use the mod's default sprite sound effect.  
Any audio file named `comboburst-0` will replace the mod's default sprite sound effect.

A game restart is not needed for the custom sprite pack to be effective.  
Note that the more sprites you add, the longer the level may have to load.  

You may find a sprite pack example under `./resources/custom-sprite/`

# Closing remarks

Thank you for using Combo Burst!

This is my first time working with C++, so feel free to make any pull requests regarding bugfixes, optimizations, etc.

If you find any bugs, you may post it in the issues tab.
