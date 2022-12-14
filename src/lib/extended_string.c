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

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "extended_string.h"

#ifdef __CC65__
#pragma static-locals(push, on)
#endif

static int __fastcall__ _strsplit_int(char in_place, char *in, char split, char ***out) {
  int i;
  int n_tokens;
  char **result = NULL;
  char **tmp;

  n_tokens = 0;
  if (in == NULL)
    goto err_out;

  for (i = 0; in[i] != '\0'; i++) {
    if (in[i] == split) {
      tmp = realloc(result, (n_tokens + 1) * sizeof(void *));
      if (tmp) {
        result = tmp;
      } else {
        free(result);
        n_tokens = 0;
        goto err_out;
      }
      in[i] = '\0';
      result[n_tokens] = in_place ? in : strdup(in);
      in = in + i + 1; i = 0;
      n_tokens++;
      if (*in == '\0')
        break;
    }
  }
  /* last token */
  if (in && *in) {
    tmp = realloc(result, (n_tokens + 1) * sizeof(void *));
    if (tmp) {
      result = tmp;
    } else {
      free(result);
      n_tokens = 0;
      goto err_out;
    }
    result[n_tokens] = in_place ? in : strdup(in);
    n_tokens++;
  }

err_out:  
  *out = result;
  return n_tokens;
}

int __fastcall__ strsplit(char *in, char split, char ***out) {
  return _strsplit_int(0, in, split, out);
}

int __fastcall__ strsplit_in_place(char *in, char split, char ***out) {
  return _strsplit_int(1, in, split, out);
}

char * __fastcall__ trim(const char *in) {
  int i = 0, o = 0, len = strlen(in);
  int trimmed_start = 0, last_non_sep = 0;
  char *out = strdup(in);
  
  for (i = 0; i < len; i++) {
    int is_whitespc = strchr(" \r\n\t", in[i]) != NULL;
    if (!trimmed_start && is_whitespc)
      continue;
    trimmed_start = 1;
    if (trimmed_start) {
      if (last_non_sep == 0 && is_whitespc)
        last_non_sep = o;
      else if (last_non_sep > 0 && !is_whitespc)
        last_non_sep = 0;
    }

    out[o] = in[i];
    o++;
  }
  if (last_non_sep > 0) {
    out[last_non_sep]='\0';
  }

  out[o] = '\0';
  return out;
}

char *strndup_ellipsis(char *in, int len) {
  char *out;
  if (strlen(in) < len) {
    return strdup(in);
  }
  out = malloc(len + 1);
  strncpy(out, in, len - 1);
  out[len-3]='.';
  out[len-2]='.';
  out[len-1]='.';
  out[len]='\0';

  return out;
}

char *ellipsis(char *in, int len) {
  if (strlen(in) < len) {
    return in;
  }
  in[len-3]='.';
  in[len-2]='.';
  in[len-1]='.';
  in[len]='\0';

  return in;
}

#ifdef __CC65__
#pragma static-locals(pop)
#endif
