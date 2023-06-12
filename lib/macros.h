
#pragma once

#if defined CONCAT || defined _CONCAT_IMPL
#error
#endif
#define CONCAT(x, y) _CONCAT_IMPL(x, y)
#define _CONCAT_IMPL(x, y) x##y
