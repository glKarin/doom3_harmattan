DOOM III on MeeGo Harmattan



Harmattan version:
* OpenGL implement is OpenGL ES1.1, so without GPU program for rendering. Like lighting, shadow and glass are not be rendering, base on bhree/dhewm3(github), Cvar is "hr_noLighting".
* Virtual buttons layer support.
* Enable auto-run on single-player game.
* Because Auto-generate texture coord function is not support in OpenGL ES1.1(Actually, OpenGL ES1.1 of powerVR is support cubemap texture coord auto generate by "GL_OES_texture_cube_map" extension.), it only render object that has texture coord explicitly, so like skybox, some PDA surface, glass and other objects that need to auto-generate texture coord will not render(So some PDAs show with noise.).
* Disable Alsa for sound hardware support. Suggest to disable sound for CPU(Set "s_noSound" to 1 when game booting.).
* Disable OpenGL cubemap.


Libraries, scripts and configs are installed in "/usr/lib/doom3".

Executable binarys installed in "/usr/bin".
"doom.arm" is client execution file.

Game data should be put in "/home/user/MyDocs/doom3".
Home path of game is in "/home/user/.doom3".

If you want to set configs for booting DOOM3, edit the shell script in "/home/user/.doom3/DOOM_III".
The file is copy from "/usr/lib/doom3/DOOM_III" when package is installed.
If this file is not executed, clicking desktop icon will execute "/usr/lib/doom3/DOOM_III.default".
Prefer configs, you can see in the shell script "/usr/lib/doom3/DOOM_III.default".

Harmattan CVar for DOOM III:
All CVars is begin with "h_", next is type character, "r" is for renderer, "s" is for sound, "g" is for gaming, "c" is for common, "t" is for testing in developing, unused.

[Rendering]
* h_rCompressTexture[integer, default is 0 - do not compresse texture]: Compressing texture when loading texture to GPU memory: 0 to disable, less than 0 to means compress level, others means compressed size.
* h_rUsingMipmap: Generate mipmap of texture[bool, default is 1 - enable mipmap]: If set false, it will not generate mipmap, so image_filter must be set GL_LINEAR or GL_NEAREST, otherwise OpenGL texture will be disabled. Disable mipmap can decrement GPU memory used.
* h_rUsingCubemapOES[bool, default is 0 - disable OpenGL cubemap texture]: Using cubemap-texture: 0 / 1
* h_rUsingMultitextureSTD[bool, default is 1 - enable OpenGL multitexture]: Using multi-texture: 0 / 1
* h_rNoLighting[bool, default is 1 - disable lighting]: Rendering without lighting: 0 / 1

[Sound]
* h_sUsingAlsa[bool, default is 1 - enable Alsa for sound]: Using Alsa sound: 0 / 1
* h_sNoSound[integer, default is 2 - not load and play sound]: Disable sound system: 0 not disable / 1 only load sound / 2 not load and play sound / 3 not init sound system, so not load and play sound" );

[Common]
* h_cUsingVKB[bool, default is 1 - enable virtual buttons layer]: Rendering virtual buttons layer and response events: 0 / 1
* h_cSwipeSens[float, default is 1.0 - not scale sensitivity]: Swipe sensitivity on virtual buttons layer: float
* h_cAlwaysRun[bool, default is 1 - always run]: Always run on every game mode: 0 / 1

[Gaming]

