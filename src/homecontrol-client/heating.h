#include <stdlib.h>
#include <string.h>
#include "slist.h"

#ifndef __heating_h
#define __heating_h

typedef struct _hc_heating hc_heating;

struct _hc_heating {
  char *id;
  char *name;
  char *set_temp;
  char *cur_temp;
  char *cur_humidity;
};

slist *heating_zones_get(void);
slist *update_heating_zones(void);

#endif
