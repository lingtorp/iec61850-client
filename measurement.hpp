#ifndef _MEASUREMENT_HPP_
#define _MEASUREMENT_HPP_

#include <vector>

template<typename T>
struct Measurement {
  std::vector<T> values;
  uint64_t client_timestamp;
};

#endif // _MEASUREMENT_HPP_
