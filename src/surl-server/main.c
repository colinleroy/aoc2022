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
#include <string.h>
#include <errno.h>
#include <curl/curl.h>
#include "simple_serial.h"
#include "extended_string.h"
#include "math.h"

#define BUFSIZE 255

typedef struct _curl_buffer curl_buffer;
struct _curl_buffer {
  char *buffer;
  int response_code;
  size_t size;

  char *headers;
  size_t headers_size;

  char *content_type;
};

static curl_buffer *curl_request(char *method, char *url, char **headers, int n_headers);
static void curl_buffer_free(curl_buffer *curlbuf);

int main(int argc, char **argv)
{
  char reqbuf[BUFSIZE];
  char *method = NULL, *url = NULL;
  char **headers = NULL;
  int i, n_headers = 0;
  size_t bufsize = 0, sent = 0;
  curl_buffer *response = NULL;

  if (argc < 2) {
    printf("Usage: %s [serial tty]\n", argv[0]);
    exit(1);
  }

  if (simple_serial_open(argv[1], B9600, 1) != 0) {
    exit(1);
  }

  atexit((void *)simple_serial_close);
  curl_global_init(CURL_GLOBAL_ALL);

  while(1) {
    free(method);
    free(url);
    for (i = 0; i < n_headers; i++) {
      free(headers[i]);
    }
    free(headers);

    method = NULL;
    url = NULL;
    n_headers = 0;
    headers = NULL;

    if (simple_serial_gets(reqbuf, BUFSIZE) != NULL) {
new_req:
      char **parts;
      int num_parts, i;

      curl_buffer_free(response);
      response = NULL;

      num_parts = strsplit(reqbuf, ' ', &parts);
      if (num_parts < 2) {
        printf("Could not parse request '%s'\n", reqbuf);
        continue;
      }
      method = trim(parts[0]);
      url = trim(parts[1]);
      
      for (i = 0; i < num_parts; i++) {
        free(parts[i]);
      }
      free(parts);
    } else {
      printf("Read error %s\n", strerror(errno));
      exit(1);
    }
    
    do {
      reqbuf[0] = '\0';
      if (simple_serial_gets(reqbuf, BUFSIZE) != NULL) {
        if (strcmp(reqbuf, "\n")) {
          headers = realloc(headers, (n_headers + 1) * sizeof(char *));
          headers[n_headers] = trim(reqbuf);
          n_headers++;
        }
      } else {
        printf("Read error %s\n", strerror(errno));
        exit(1);
      }
    } while (strcmp(reqbuf, "\n"));

    if (method == NULL || url == NULL) {
      printf("could not parse request '%s'\n", reqbuf);
      continue;
    }

    simple_serial_puts("WAIT\n");


    response = curl_request(method, url, headers, n_headers);
    
    simple_serial_printf("%d,%d,%s\n", response->response_code, response->size, response->content_type);
    sent = 0;
    while (sent < response->size) {
      size_t to_send;

      if (simple_serial_gets(reqbuf, BUFSIZE) != NULL) {
        if(!strncmp("SEND ", reqbuf, 5)) {
          bufsize = atoi(reqbuf + 5);
        } else {
          printf("Aborting send\n");
          goto new_req;
        }
      } else {
        printf("Aborting send\n");
        goto new_req;
      }

      to_send = min(bufsize, response->size - sent);
      sent += simple_serial_write(response->buffer + sent, sizeof(char), to_send);
      printf("sent %zu (total %zu/%zu)\n", to_send, sent, response->size);
    }

    printf("sent %d response to %s %s (%ld bytes)\n", response->response_code, method, url, response->size);

  }
}

static size_t curl_write_data_cb(void *contents, size_t size, size_t nmemb, void *data)
{
  size_t realsize = size * nmemb;
  curl_buffer *curlbuf = (curl_buffer *)data;

  char *ptr = realloc(curlbuf->buffer, curlbuf->size + realsize + 1);

  if(!ptr) {
    printf("not enough memory\n");
    return 0;
  }

  curlbuf->buffer = ptr;
  memcpy(&(curlbuf->buffer[curlbuf->size]), contents, realsize);
  curlbuf->size += realsize;
  curlbuf->buffer[curlbuf->size] = '\0';

  return realsize;
}

static size_t curl_write_header_cb(void *contents, size_t size, size_t nmemb, void *data)
{
  size_t realsize = size * nmemb;
  curl_buffer *curlbuf = (curl_buffer *)data;

  char *ptr = realloc(curlbuf->headers, curlbuf->headers_size + realsize + 1);

  if(!ptr) {
    printf("not enough memory\n");
    return 0;
  }

  curlbuf->headers = ptr;
  memcpy(&(curlbuf->headers[curlbuf->headers_size]), contents, realsize);
  curlbuf->headers_size += realsize;
  curlbuf->headers[curlbuf->headers_size] = '\0';

  return realsize;
}

static void curl_buffer_free(curl_buffer *curlbuf) {
  if (curlbuf == NULL) {
    return;
  }
  free(curlbuf->buffer);
  free(curlbuf->headers);
  free(curlbuf->content_type);
  free(curlbuf);
}

static void proxy_set_curl_opts(CURL *curl) {
  curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
  curl_easy_setopt(curl, CURLOPT_USERAGENT, "http-serial-proxy/1.0");

}

static curl_buffer *curl_request(char *method, char *url, char **headers, int n_headers) {
  CURL *curl;
  CURLcode res;
  int i;
  curl_buffer *curlbuf;
  int is_ftp = !strncmp("ftp", url, 3);
  int ftp_is_maybe_dir = (is_ftp && url[strlen(url)-1] != '/');
  int ftp_try_dir = (is_ftp && url[strlen(url)-1] == '/');

  if (ftp_is_maybe_dir) {
    /* try dir first */
    char *dir_url = malloc(strlen(url) + 2);
    memcpy(dir_url, url, strlen(url));
    dir_url[strlen(url)] = '/';
    dir_url[strlen(url) + 1] = '\0';
    curlbuf = curl_request(method, dir_url, headers, n_headers);
    if (curlbuf->response_code >= 200 && curlbuf->response_code < 300) {
      return curlbuf;
    } else {
      /* get as file */
      free(curlbuf);
    }
  }
  
  curlbuf = malloc(sizeof(curl_buffer));
  curlbuf->buffer = NULL;
  curlbuf->size = 0;
  curlbuf->response_code = 500;
  curlbuf->headers = NULL;
  curlbuf->headers_size = 0;

  if (strcmp(method, "GET")) {
    printf("Unsupported method %s\n", method);
    return curlbuf;
  }

  curl = curl_easy_init();
  proxy_set_curl_opts(curl);

  printf("%s %s\n", method, url);
  if (curl) {
    struct curl_slist *curl_headers = NULL;
    char *tmp;
    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, curl_write_data_cb);
    curl_easy_setopt(curl, CURLOPT_HEADERFUNCTION, curl_write_header_cb);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)curlbuf);
    curl_easy_setopt(curl, CURLOPT_HEADERDATA, (void *)curlbuf);
    if (ftp_try_dir) {
      curl_easy_setopt(curl, CURLOPT_DIRLISTONLY, 1L);
    }
    for (i = 0; i < n_headers; i++) {
      curl_headers = curl_slist_append(curl_headers, headers[i]);
      printf("%s\n", headers[i]);
    }
    
    printf("\n");

    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, curl_headers);
    res = curl_easy_perform(curl);
    curl_slist_free_all(curl_headers);

    if(res != CURLE_OK) {
      printf("curl error %s\n", curl_easy_strerror(res));
    }
    curl_easy_getinfo (curl, CURLINFO_RESPONSE_CODE, &(curlbuf->response_code));
    curl_easy_getinfo (curl, CURLINFO_CONTENT_TYPE, &tmp);
    if (tmp != NULL) {
      curlbuf->content_type = strdup(tmp);
    } else {
      if (ftp_try_dir) {
        curlbuf->content_type = strdup("directory");
      } else {
        curlbuf->content_type = strdup("application/octet-stream");
      }
    }
    printf("Content-Type: %s\n", curlbuf->content_type);
  }

  curl_easy_cleanup(curl);
  return curlbuf;
}
