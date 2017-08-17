#ifndef _MEASUREMENT_HPP_
#define _MEASUREMENT_HPP_

template<typename T>
struct Measurement {
  T value;
  uint64_t client_timestamp;
};

#endif // _MEASUREMENT_HPP_
