/*                        ,
 * Xsipp.c, written by Rene K. Mueller for SIPP 3.0, Nov 1992
 *
 * render_ximage(w,h,mode,over,rtype,dbuffer,tcolor)
 *
 *        w,h = size of window
 *       mode = LINE, FLAT, GOURAUD and PHONG
 *       over = oversampling (3 is good)
 *      rtype = render type: 0) normal view, 1) fast-preview with subdivide
 *    dbuffer = pseudo double-buffering: 0) off, 1) on
 *     tcolor = true-color forcing (216 color dithering)
 *              (doesn't look good yet)
 *
 * Desription:
 * -----------
 *    This function support monochrom monitor and 8bit color monitor. If
 *    you call render_ximage again, after a window is mapped, then it will
 *    render in the same window.
 *
 */

/* ------------------------------------------------------------------------- */

#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>
#include <stdint.h>

#include <X11/Xlib.h>
#include <X11/Xutil.h>

#define Object XObject     /* conflict between X11 and SIPP */

#include <sipp.h>
#include <sipp_pixmap.h>

/* ------------------------------------------------------------------------- */

static char *prgname = "Xsipp";
static int width = 512, height = 512;
static Display *disp;
static GC gc;
static Window win = 0;
static Colormap cmap = 0;
static XColor color_tab[216];
static Pixmap grey_tab[65*8], image = 0;
static Screen *scrn;
static int update = 0, truecolor = 0;

static int dither16x16[16][16];
static int divN[256], modN[256];
#define DMAP(v,x,y)  (modN[v]>dither16x16[x%16][y%16] ? divN[v] + 1 : divN[v])

/* ------------------------------------------------------------------------- */

static int
dither_value(int x, int y, int size)
{
   int d;

   x %= (1 << size);
   y %= (1 << size);
   d = 0;

   while (size-->0) {
       d = (d << 1 | (x & 1 ^ y & 1)) << 1 | y & 1;
       x >>= 1;
       y >>= 1;
   }

   return d;
}

/* ------------------------------------------------------------------------- */

static void
myplot(void *data, int x, int y, unsigned char r, unsigned char g, unsigned char b)
{
    int size = (int)(intptr_t)data;

    if (cmap) {
        if (truecolor) {
            XSetForeground(disp, gc, color_tab[DMAP(r,x,y) * 36
                                               + DMAP(g,x,y) * 6
                                               + DMAP(b,x,y)].pixel);
        } else {
            XSetForeground(disp, gc, 
                           color_tab[((int)(r * .299 + g * .587 + b * .114)
                                      / 2)].pixel);
        } 
    } else {
        int c = (((r * 32L) + (g * 64L) + (b * 16L)) * 64L) / 28560L;
        XSetTile(disp, gc, grey_tab[c]);
    }
    XFillRectangle(disp,image,gc,x*size,y*size,size,size);
    if(update && (x == width - 1)) {
        XClearWindow(disp, win);
    }
}


static void
myline(void *data, int x1, int y1, int x2, int y2)
{
    if (update) {
        XDrawLine(disp, win, gc, x1, y1, x2, y2);
    }
    XDrawLine(disp, image, gc, x1, y1, x2, y2);
}


static void
myrender(int w, int h, int mode, int over)
{
    int size = (mode == LINE) ? 1 : (cmap ? 1 : 2);

    width = w / size;
    height = h / size;
    render_image_func(width, height, 
                      (mode == LINE) ? (Pixel_func *)myline : myplot, 
                      (void *)(intptr_t)size, mode, over);
    if (!update) {
        XClearWindow(disp, win);
    }
    XFlush(disp);
}


static void
myrender2(int w, int h, int mode, int over)
{
    int size = width;

    truecolor = 0;
    if (mode == LINE) {
        mode = PHONG;
    }

    while(1) {
        width = w / size;
        height = h / size;
        render_image_func(width, height, myplot, (void *)(intptr_t)size,
                          mode, over);
        if (!cmap && (size / 2 == 1)) {
            break;
        }
        if (size == 1) {
            break;
        }
        if (!update) {
            XClearWindow(disp, win);
        }
        XFlush(disp);
        size = size / 2;
    }
}


static void
myrender3(int w, int h, int mode, int over)
{
    int          size = 16;
    int          i, j, n; 
    Sipp_pixmap *pm;

    truecolor = 0;
    if (mode == LINE) {
        mode = PHONG;
    }
    width = w / size;
    height = h / size;
    pm = sipp_pixmap_create(width, height);
    render_image_func(width, height, (Pixel_func *)sipp_pixmap_set_pixel,
                      pm, mode, over);
    for (i = 0; i < width; i++) {
        for(j = 0; j < height; j++);
    }
}


/* ------------------------------------------------------------------------- */

Pixmap 
render_ximage(int width, int height, int mode, int over, int rtype, int dbuffer, int tcolor)
{
    if (!win) {
        disp = XOpenDisplay("");
        scrn = ScreenOfDisplay(disp, 0);
   
        if (DefaultDepth(disp, DefaultScreen(disp)) > 1) {
            cmap = DefaultColormap(disp, DefaultScreen(disp));
        }
   
        gc = XCreateGC(disp, DefaultRootWindow(disp), 0L, 0L);
        XSetBackground(disp, gc, WhitePixel(disp, DefaultScreen(disp)));
        XSetForeground(disp, gc, BlackPixel(disp, DefaultScreen(disp)));
        XSetPlaneMask(disp, gc, cmap?-1L:1L);
   
        if (cmap) {
            int i; 
            double N = (255.0/5.);

            for (i = 0; i < 256; i++) {
                dither16x16[i / 16][i % 16] = dither_value(i / 16, i % 16, 4);
                divN[i] = i / N;
                modN[i] = i - (N * divN[i]);
            }
            modN[255] = 0;

        } else {
            int n, i, j;
            unsigned char grey[8];

            for (n = 0; n < 65; n++) {
                for (i = 0; i < 8; i++) {
                    grey[i] = 0;
                }
                for (i = 0; i < 8; i++) {
                    for (j = 0; j < 8; j++) {
                        grey[j] |= ((n <= dither_value(i, j, 3)) ? 1 : 0) << i;
                    }
                }
                grey_tab[n] 
                    = XCreateBitmapFromData(disp, DefaultRootWindow(disp), 
                                            grey, 8, 8);
            }
            XSetFillStyle(disp, gc, FillTiled);
        }
      
        if (cmap) {
            if(tcolor) { /* pseudo true-color with 216 well allocated colors */
                int r, g, b, n = 0;

                for (r = 0; r < 6; r++) {
                    for (g = 0; g < 6; g++) {
                        for (b = 0; b < 6; b++) {
                            color_tab[n].red = (r * 65535) / 5;
                            color_tab[n].green = (g * 65535) / 5;
                            color_tab[n].blue = (b * 65535) / 5;
                            if (XAllocColor(disp, cmap, &color_tab[n])==0) {
                                fprintf(stderr, "%s: color allocation failed, %d of 216 Colors (R=%d, G=%d, B=%d) failed", prgname, n, r*43, g*43, b*43);
                                exit(1);
                            }
                            n++;
                        }
                    }
                }
            } else {       /* greyscale (default) */
                int n; 
                double gamma = 1.4;

                for (n = 0; n < 128; n++) {
                    long g = pow(n / 127., 1. / gamma) * 65535;

                    color_tab[n].red = g;
                    color_tab[n].green = g;
                    color_tab[n].blue = g;
                    if (XAllocColor(disp, cmap, &color_tab[n])==0) {
                        fprintf(stderr, "%s: greyscale allocation failed, %d of 128 Colors (R=%d, G=%d, B=%d) failed", prgname, n, n, n, n);
                        exit(1);
                    }
                }
            }
        }
    }
   
    if (!image) {
        image = XCreatePixmap(disp, DefaultRootWindow(disp), 
                              width, height, cmap ? 8 : 1);
    }   

    if ((mode==LINE)||!win) {   
        XSetForeground(disp, gc, color_tab[0].pixel);
        if (!cmap) {
            XSetTile(disp, gc, grey_tab[0]);
        }
        XFillRectangle(disp, image, gc, 0, 0, width, height);
    }
    
    if (!win) {
        XSizeHints sh;
        
        win = XCreateSimpleWindow(disp, DefaultRootWindow(disp), 
                                  0, 0, width, height, 2,
                                  BlackPixel(disp, DefaultScreen(disp)), 
                                  WhitePixel(disp, DefaultScreen(disp)));
        
        sh.flags = PSize | PMinSize | PMaxSize;
        sh.width = sh.min_width = sh.max_width = width;
        sh.height = sh.min_height = sh.max_height = height;
        XSetStandardProperties(disp, win, "SIPP Rendering", "SIPP", 
                               None, NULL, 0, &sh);
        
        XSetWindowBackgroundPixmap(disp, win, image);
        XClearWindow(disp, win);
        XMapWindow(disp, win);
        XFlush(disp);
        XSync(disp, 0);
    }
    
    if (dbuffer) {
        update = 0;
    } else {
        update = 1;
        XClearWindow(disp, win);
        XFlush(disp);
        XSync(disp, 0);
    }
    
    if (tcolor) {
        truecolor = 1;
    }
    
    if (mode==LINE) {
        if (cmap) {
            XSetForeground(disp, gc, color_tab[215].pixel);
        } else {
            XSetTile(disp, gc, grey_tab[64]);
        }
    }
    
    if (rtype == 2) {
        myrender3(width, height, mode, over);
    } else if (rtype==1) {
        myrender2(width, height, mode, over);
    } else {
        myrender(width, height, mode, over);
    }

   return(image);
}


