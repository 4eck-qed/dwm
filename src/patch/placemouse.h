#ifndef DWM_PATCH_PLACEMOUSE_H
#define DWM_PATCH_PLACEMOUSE_H

static void moveorplace(const Arg *arg);
static void placemouse(const Arg *arg);
static Client *recttoclient(int x, int y, int w, int h);

#endif // DWM_PATCH_PLACEMOUSE_H
