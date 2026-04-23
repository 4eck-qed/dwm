### dwm

DWM with many patches you can easily toggle on/off.\
Current DWM version: 6.5
Available patches:

- [alpha](https://dwm.suckless.org/patches/alpha/)
- [alternativetags](https://dwm.suckless.org/patches/alternativetags/)
- [barpadding](https://dwm.suckless.org/patches/barpadding/)
- [cool autostart](https://dwm.suckless.org/patches/cool_autostart/)
- [cursorwarp](https://dwm.suckless.org/patches/cursorwarp/)
- [dwmblocks](https://github.com/torrinfail/dwmblocks)
- [fakefullscreen](https://dwm.suckless.org/patches/fakefullscreen/)
- [fullscreen](https://dwm.suckless.org/patches/fullscreen/)
- [fancybar](https://dwm.suckless.org/patches/fancybar/)
- [fibonacci](https://dwm.suckless.org/patches/fibonacci/)
- [float border color](https://dwm.suckless.org/patches/float_border_color/)
- [hide vacant tags](https://dwm.suckless.org/patches/hide_vacant_tags/)
- [layoutmenu](https://dwm.suckless.org/patches/layoutmenu/)
- [pertag](https://dwm.suckless.org/patches/pertag/)
- placemouse
- [resizecorners](https://dwm.suckless.org/patches/resizecorners/)
- [resizehere](https://dwm.suckless.org/patches/resizehere/)
- [status2d](https://dwm.suckless.org/patches/status2d/)
- [statuscmd](https://dwm.suckless.org/patches/statuscmd/)
- [statuspadding](https://dwm.suckless.org/patches/statuspadding/)
- [swallow](https://dwm.suckless.org/patches/swallow/)
- [systray](https://dwm.suckless.org/patches/systray/)
- [taglabels](https://dwm.suckless.org/patches/taglabels/)
- [vanitygaps](https://dwm.suckless.org/patches/vanitygaps/)
- [viewontag](https://dwm.suckless.org/patches/viewontag/)
- [winicon](https://dwm.suckless.org/patches/winicon/)

#### Usage

Configure which patches shall be applied in `patch.def.h`.

#### Build

Just use the `deploy.sh` script and it will build and install dwm..
..or use `deploy-and-relog.sh` which will additionally terminate the session so you can quickly relog to the newly deployed dwm.

I also added an extra runnable script (`dwm_d`) as well as an xsession entry (`DWM DEBUG`)\
which starts dwm and pipes all the output into a log file (`$HOME/log/dwm.log`) for debugging purposes.
