About HivelyTracker, from IRIS & Up Rough!

Hively Tracker is a tracker program based upon the AHX format created in the mid '90s by Dexter and Pink of Abyss. The format was relatively popular, and many songs were created and used in scene productions and games. AHX was designed to create a very SID-like sound on the Amiga.

HivelyTracker can import and export modules and instruments in the AHX format, but it also improves on AHX in several ways and therefore has its own instrument and module formats.

HivelyTracker offers the following features over AHX:

- Multichannel (4 to 16 channels)
- Per-channel stereo panning
- Two commands per note instead of one
- Ring modulation
- A more feature rich editor


HivelyTracker was coded by Xeron/IRIS
GUI design and skins by Spot/Up Rough

Editing with the mouse and keyboard
==================================

In the pattern editor and the instrument performance editor:

- Click a cell to focus its row and field. Drag to select a rectangle; Shift-click or Shift+arrow keys extends the selection. Pattern selections can span channels.
- Ctrl+C copies the selection, Ctrl+X cuts it, and Ctrl+V pastes starting at the cursor. Without a selection, these commands use the current cell. Click the destination cell before pasting. Notes, instrument/waveform numbers, effect commands, and effect parameters are separate fields. Paste into the matching kind of field; either effect slot can receive the other slot's commands or parameters.
- Ctrl+Q / Ctrl+A transposes notes up/down one semitone. Add Shift for one octave (12 semitones). With a selection, only its note fields change; without one, the note on the current row changes. Empty notes stay empty.
- Ctrl+mouse wheel over the editor increases/decreases the selected fields, or the current field if there is no selection. Values stop at their valid limits.
- Delete clears selected fields. Escape clears the selection. Edits support Undo/Redo, including selections across channels.

Click a number in the instrument editor to type its value directly. The first digit replaces the value; click again or use Left/Right to position the cursor. Enter or clicking elsewhere applies the value, Escape cancels, and Tab / Shift+Tab moves between fields. Numbers use the displayed base: decimal normally, hexadecimal for wavelength. Enter 0 or 1 for Off/On. Values are limited to each field's allowed range.

The existing whole-track shortcuts (such as Shift+F4/F5) remain available.

Editor tests
============

With SDL 1.2, SDL_image and SDL_ttf development packages installed:

    make -C tests test

The tests run mouse and keyboard events through the real GUI using SDL's dummy video driver, and check clipboard edits, transposition, value limits and Undo/Redo. They do not need a display or audio device.

Compiling
=========

The makefile at the root of the repository is for Amiga OS only and will not work for other platforms.

Building for Amiga OS
---------------------

    make

Building for Linux
------------------

    cd sdl
    make -f Makefile.linux
    
Building for Haiku
------------------

    cd sdl
    make -f Makefile.haiku
    
Building for Windows
--------------------

    cd sdl
    make -f Makefile
    
Building for MacOS
------------------

An XCode project is provided in the SDL directory.
