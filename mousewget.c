/*
 * mousewget.c - mouse_wgetch support for cdialog 0.9a+
 *
 * Copyright 1995   demarco_p@abramo.it (Pasquale De Marco)
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 2 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program; if not, write to the Free Software
 *   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 ********/

#include "dialog.h"

#ifdef HAVE_LIBGPM
#include <gpm.h>
#endif

#include <unistd.h>        /* select() for mouse_wgetch() */
#include <sys/time.h>      /* timeval  for mouse_wgetch() */

int mouse_wgetch(WINDOW *win)
{
    int key;
    
#ifdef HAVE_LIBGPM

    fd_set selSet;
    int flag, result;
    int fd=STDIN_FILENO;
    static Gpm_Event ev;

    key = 0;

    if (!gpm_flag || gpm_fd==-1) return wgetch(win);
    if (gpm_morekeys) return (*gpm_handler)(&ev,gpm_data);

    gpm_hflag = 0;

  while (1) {

    if (gpm_visiblepointer) GPM_DRAWPOINTER(&ev);

    do {
        FD_ZERO(&selSet);
        FD_SET(fd,&selSet);
        FD_SET(gpm_fd,&selSet);
        gpm_timeout.tv_usec=WTIMEOUT_VAL*10000;
        gpm_timeout.tv_sec=0;
        flag=select(5,&selSet,(fd_set *)NULL,(fd_set *)NULL,&gpm_timeout);
        ctl_idlemsg(win);
        /* fprintf(stderr, "X"); */
    } while (!flag);

    if (FD_ISSET(fd,&selSet))
      return wgetch(win);
      
    if (flag == -1)
        continue;
    
    if (Gpm_GetEvent(&ev) && gpm_handler
        && (result=(*gpm_handler)(&ev,gpm_data)))
    {
      gpm_hflag=1;
      return result;
    }
  }

#else
  
  /* before calling this you have to put (only the first time)
     a "wtimeout(dialog, WTIMEOUT_VAL);" */
  
  do { 

    key = wgetch(win);

    if (key == ERR)
        ctl_idlemsg(win);

  } while (key == ERR);
  
#endif

    return key;
}
