
#pragma once

#include "lib/macros.h"

// https://stackoverflow.com/questions/2419650/c-c-macro-template-blackmagic-to-generate-unique-name
#define MODULE_INIT(code)  _MODULE_INIT_IMPL(code, __COUNTER__)
#define _MODULE_INIT_IMPL(code, unique_id)  \
  inline void CONCAT(_module_init_, unique_id)() __attribute__ ((constructor));  \
  inline void CONCAT(_module_init_, unique_id)() {  \
    code  \
  }
