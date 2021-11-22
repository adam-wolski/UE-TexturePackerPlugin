# Texture Packer

Yet another texture packing plugin for unreal.
- Packing channels from multiple textures into single one
- Reordering channels in single texture

This one differs mostly by that it is an C++ editor plugin which means:
- It doesn't need any scene loaded.
- It works losslessly on source image data. You can pack same texture as much as you want without losing quality.

Has simple UI and is fast to work with.

## Usage

Select any number of textures in content browser, right click and select Pack Textures.
