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

#include "network.h"
#include "simple_serial.h"

http_response *get_url(const char *url) {
  const char *headers[1] = {"Accept: text/*"};
  http_response *resp;
  
  simple_serial_set_activity_indicator(1, 39, 0);
  resp = http_request("GET", url, headers, 1);
  simple_serial_set_activity_indicator(0, 0, 0);

  return resp;
}
