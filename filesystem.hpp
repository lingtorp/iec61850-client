#ifndef _FILESYSTEM_HPP_
#define _FILESYSTEM_HPP_

#include "measurement.hpp"

#ifdef __LINUX__
  #include <sys/types.h>
  #include <sys/stat.h>
  #include <fcntl.h>
  #include <errno.h>
  #include <stdio.h>
#elif
  #error OS not supported
#endif

namespace FS {
  template<typename T>
  bool save_data(std::vector<Measurement<T>> &values, std::string file_path) {
    #ifdef __LINUX__
      std::ifstream file(file_path);

      if (!file) { std::cerr << "Failure 1." << std::endl; }

      for (auto &value : values) {
        file << value.value << "," << value.timestamp << ";";
      }

      return true;
    #elif
      #error OS not supported
    #endif
  }
}

#endif // _FILESYSTEM_HPP_
