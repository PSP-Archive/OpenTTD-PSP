OpenTTD is a clone of the Microprose game "Transport Tycoon Deluxe", a popular 
game originally written by Chris Sawyer. It attempts to mimic the original game
as closely as possible while extending it with new features.

OpenTTD is originaly developed by the openttd team http://www.openttd.org and
has been ported to the PSP by Jaime Peñalba <jpenalbae at gmail dot com>. This 
code is licensed under the GNU General Public License version 2.0. For more 
information, see the file 'COPYING' included with every release and source 
download of the game.


INSTALLATION
============

To run OpenTTD it is *REQUIRED* that you install certain files needed from the
original retail Transport Tycoon Deluxe, whether for DOS or Windows.

For more detailed installation instructions and supported firmwares info, visit
site at http://openttd.pc-workshop.da.ru/faq.php (sometimes outdated)

Since 0.5.3 version of this port, 1.50 kernel is no longer supported, now the
code is 3.XX firmwares and Slim compatbile. 1.50 kernel addon is no longer
required. For older versions supporting 1.50 kernel download any of the archive
release. Currently it has been only tested under 3.71 M33 and 3.80 M33


1.- Download latest OpenTTD for PSP release.

2.- Uncompress rar and copy the "ottd/" folder to "PSP/GAME/"

3.- Now you must install some data files from the retail Transport Tycoon Deluxe
    To install them you must copy the following files:
      * TRG1R.GRF     * TRGCR.GRF
      * TRGHR.GRF     * TRGIR.GRF
      * TRGTR.GRF     * SAMPLE.CAT
    To "/PSP/GAME/ottd/data/" folder.


OPTIONAL MIDI MUSIC
-------------------

4.- To be able to play midi sound tracks during game, you must copy the game 
midi tracks to "/PSP/GAME/ottd/gm/" folder.

 gm_tt00.gm  gm_tt03.gm  gm_tt06.gm  gm_tt09.gm  gm_tt12.gm  gm_tt15.gm  
 gm_tt18.gm  gm_tt01.gm  gm_tt04.gm  gm_tt07.gm  gm_tt10.gm  gm_tt13.gm
 gm_tt16.gm  gm_tt19.gm  gm_tt02.gm  gm_tt05.gm  gm_tt08.gm  gm_tt11.gm  
 gm_tt14.gm  gm_tt17.gm  gm_tt20.gm  gm_tt21.gm

    Those are the tracks the game will play, you can even rename your own midis
to play them while building :)



CONTROLS
========

NOTE: To use multiplayer feature, you must start the game with psp 
wireless switch on.


- Digital pad: 1px movement
- Analog stick: Variable speed movement

- Cross: mouse left click
- Circle: mouse right click, hold it and press pad to move arround the map.
- Square: hold it (over a window) and press pad or analog, to move a window.
- Triangle: Closes the window under mouse cursor.

- Cross + Square: ctrl + click, used for presignals

- L trigger: Zoom out
- R trigger: Zoom in

- L trigger + R trigger: Delete all non vital windows

- Start + L trigger: Change to lower system frequency
- Start + R trigger: Change to upper system frequency

- Select + pad Up: Shows the cheats window.
- Select + pad Down: Takes a screenshot.
- Select + pad Left: Show OSK keyboard. (press Start/Select to close it)
- Select + pad Right: Show network chat toolbar.


STATUS
======

See release Status and ChangeLog files, for information about the development 
status.


SOURCE & UPDATES
================

Source is now a bit more clean, but it can be always improved, also is pending 
to be reviewed if it can be added to the official openttd svn.

To compile source, the following dependencies are needed:
 - Working PSP development environment
 - SDL
 - Zlib
 - Libpng
 - Libjpeg
 - LibFreetype
 - Libtimidity (timidity.h uint* types comment or ifdef required to build)

Just unpack source, and type make.

Source code, updates and more detailed info can be found at:
http://openttd.pc-workshop.da.ru


AUTHORS
=======

OpenTTD has been developed by the openttd team under the GPL license,
you can find their official site at http://www.openttd.org

PSP porting has been done by Jaime Penalba <jpenalbae at gmail dot com>,
psp port site is available at http://openttd.pc-workshop.da.ru

PSP port code/graphics contributors:
 - Silentdragon
 - RacerII
 - Sharkus 
 - Daydreamer

Port also includes code from:
 - Danzeff OSK, thanks to Jeff Chen
 - Libtimidity psp port from psp doom

At last thanks to all ps2dev.org people who develops such nice SDK 
which make all this possible.



BUGS & CONTACT
==============

Any kind of patch, sugestion or contribution is welcome, contact me at
Jaime Peñalba <jpenalbae at gmail dot com> - http://openttd.pc-workshop.da.ru

