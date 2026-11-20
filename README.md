# FluidiaGBA

A pixel-art physics game for the Game Boy Advance, made for fun.
(Add one or two lines about what the player does and which physics concepts it teaches.)

Built with [Butano](https://github.com/GValiente/butano), a modern C++ engine for the GBA.

## Building

1. Install devkitPro with the `gba-dev` package (https://devkitpro.org/wiki/Getting_Started).
2. Set the environment variables:
   export DEVKITPRO=/opt/devkitpro
   export DEVKITARM=/opt/devkitpro/devkitARM
3. Build:
   cd FluidiaGBA
   make -j$(nproc)
4. Run `FluidiaGBA.gba` in an emulator such as mGBA.

## Repository layout

- `FluidiaGBA/` - the game (source, graphics, audio)
- `butano/` and `common/` - the Butano engine and its shared assets, needed to build
- `LICENSE`, `licenses/`, `credits/` - Butano's license and credits

## Credits

- `Engine:` Butano by Gustavo Valiente.
- `Pixel Art:` Made by Me
- `Music:` all found in Pixabay

## Disclaimer stuff
Im still new to developing actual stuff for actual things, so AI was used on few parts. but I am learning as I progress. I learned that C++ is absolutely hell to look at
