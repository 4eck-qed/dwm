#ifndef DWM_PATCH_WINICON_H
#define DWM_PATCH_WINICON_H

static Picture geticonprop(Window w, unsigned int *icw, unsigned int *ich);
static void freeicon(Client *c);
static void updateicon(Client *c);

#endif // DWM_PATCH_WINICON_H
