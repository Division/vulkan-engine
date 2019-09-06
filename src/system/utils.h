//
// Created by Sidorenko Nikita on 4/9/18.
//

#ifndef CPPWRAPPER_UTILS_H
#define CPPWRAPPER_UTILS_H

#include <climits>

template <typename T>
T swap_endian(T u) {
  static_assert (CHAR_BIT == 8, "CHAR_BIT != 8");

  union {
    T u;
    unsigned char u8[sizeof(T)];
  } source, dest;

  source.u = u;

  for (auto k = 0; k < sizeof(T); k++) {
    dest.u8[k] = source.u8[sizeof(T) - k - 1];
  }

  return dest.u;
}

#endif //CPPWRAPPER_UTILS_H
