/*
 * Copyright (C) 2022 Colin Leroy-Mira <colin@colino.net>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include <stdio.h>
#include <stdlib.h>
#include "extended_conio.h"

static unsigned char scrw = 255, scrh = 255;

#ifndef __CC65__
#include <termios.h>
#include <unistd.h>
#include <sys/ioctl.h>

void clrscr(void) {
  fprintf(stdout, "%c[2J", CH_ESC);
  fprintf(stdout, "%c[%d;%dH", CH_ESC, 0, 0);
}
void gotoxy(int x, int y) {
  fprintf(stdout, "%c[%d;%dH", CH_ESC, x, y);
}
void gotox(int x) {
  fprintf(stdout, "%c[%d;%dH", CH_ESC, x, 0); /* fixme not 0 y */
}
void gotoy(int y) {
  fprintf(stdout, "%c[%d;%dH", CH_ESC, 0, y); /* fixme not 0 x */
}

char cgetc(void) {
  char c;
  static int unset_canon = 0;
  static struct termios ttyf;

  if (!unset_canon) {
    tcgetattr( STDIN_FILENO, &ttyf);
    ttyf.c_lflag &= ~(ICANON);
    tcsetattr( STDIN_FILENO, TCSANOW, &ttyf);
    unset_canon = 1;
  }
  fread(&c, 1, 1, stdin);
  return c;
}
int kbhit(void) {
  int n;
  
  return (ioctl(fileno(stdin), FIONREAD, &n) == 0 && n > 0);
}

static void cputsxy(int x, int y, char *buf) {
  char *prefix;
  int i;

  if (scrw == 255 && scrh == 255) {
    screensize(&scrw, &scrh);
  }

  prefix = malloc(scrw);
  for (i = 0; i < x && i < scrw - 1; i++) {
    prefix[i] = ' ';
  }
  prefix[i] = '\0';
  if (strlen(buf) > scrw - 1 - x) {
    buf[scrw - 1 - x] = '\0';
  }
  printf("%s%s", prefix, buf);
  free(prefix);
}
#endif

#ifndef __CC65__
void screensize(unsigned char *w, unsigned char *h) {
  *w = 40;
  *h = 24;
}

void chline(int len) {
  int i = 0;
  for (i = 0; i < len; i++) {
    printf("_");
  }
}
#endif

char * __fastcall__ cgets(char *buf, size_t size) {
#ifdef __CC65__
  char c;
  size_t i = 0, max_i = 0;
  int cur_x, cur_y;
  int prev_cursor = 0;
  
  if (scrw == 255 && scrh == 255) {
    screensize(&scrw, &scrh);
  }

  memset(buf, '\0', size - 1);
  prev_cursor = cursor(1);
  
  while (i < size - 1) {
    cur_x = wherex();
    cur_y = wherey();

    c = cgetc();

    if (c == CH_ENTER) {
      break;
    } else if (c == CH_CURS_LEFT) {
      if (i > 0) {
        i--;
        cur_x--;
      }
    } else if (c == CH_CURS_RIGHT) {
      if (i < max_i) {
        i++;
        cur_x++;
      }
    } else {
      cputc(c);
      buf[i] = c;
      i++;
      cur_x++;
    }
    if (i > max_i) {
      max_i = i;
    }

    if (cur_x > scrw - 1) {
      cur_x = 0;
      cur_y++;
      if (cur_y > scrh - 1) {
        gotoxy(scrw - 1, scrh - 1);
        printf("\n");
        cputcxy(scrw - 1, scrh - 2, c);
        cur_y--;
      }
    } else if (cur_x < 0) {
      cur_x = scrw - 1;
      cur_y--;
    }
    gotoxy(cur_x, cur_y);
    
  }
  cursor(prev_cursor);
  printf("\n");
  buf[i] = '\0';

  return buf;
#else
  return fgets(buf, size, stdin);
#endif
}

static char *clearbuf = NULL;
void __fastcall__ clrzone(char xs, char ys, char xe, char ye) {
  int l = xe - xs + 1;
  
  if (scrw == 255 && scrh == 255) {
    screensize(&scrw, &scrh);
  }

  if (clearbuf == NULL) {
    clearbuf = malloc(scrw + 2);
    memset(clearbuf, ' ', scrw + 2);
  }

  memset(clearbuf, ' ', l);
  clearbuf[l] = '\0';

  while (ys < ye + 1) {
    cputsxy(xs, ys, clearbuf);
    ys++;
  }
}

void printxcentered(int y, char *buf) {
  int len = strlen(buf);
  int startx;

  if (scrw == 255 && scrh == 255) {
    screensize(&scrw, &scrh);
  }

  startx = (scrw - len) / 2;
  gotoxy(startx, y);
  cputs(buf);
}

void printxcenteredbox(int width, int height) {
  int startx, starty, i;
  char *line = malloc(width + 1);

  if (scrw == 255 && scrh == 255) {
    screensize(&scrw, &scrh);
  }

  startx = (scrw - width) / 2;
  starty = (scrh - height) / 2;
  
  clrzone(startx, starty, startx + width - 1, starty + height);
  
  for (i = 0; i < width; i++) {
    line[i] = '-';
  }
  line[0] = '+';
  line[i - 1] = '+';
  line[i] = '\0';
  
  gotoxy(startx, starty);
  puts(line);
  for (i = 1; i < height + 1; i++) {
    cputsxy(startx, starty + i,  "!");
    cputsxy(startx + width - 1, starty + i, "!");
  }
  gotoxy(startx, starty + height + 1);
  puts(line);
  
  free(line);
}

void get_scrollwindow(unsigned char *top, unsigned char *bottom){
#ifdef __CC65__
  char t, b;

  __asm__("lda $22"); /* get WNDTOP */
  __asm__("sta %v", t);
  __asm__("lda $23"); /* get WNDBTM */
  __asm__("sta %v", b);
  
  *top = t;
  *bottom = b;
#else
  *top = 0;
  *bottom = 24;
#endif

}

void set_scrollwindow(unsigned char top, unsigned char bottom) {
#ifdef __CC65__
  char t = top, b = bottom;

  if (top >= bottom || bottom > 24)
    return;

  __asm__("lda %v", t);
  __asm__("sta $22"); /* store WNDTOP */
  __asm__("lda %v", b);
  __asm__("sta $23"); /* store WNDBTM */
#endif
}
