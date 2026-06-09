
#ifndef TEMPERATURE
#define TEMPERATURE

#include <stdint.h>

struct info_temp{
  int16_t tempBLE;
  int converted_dec_temp;
  int converted_ent_temp;
};


struct info_temp read_temp(void);


#endif
