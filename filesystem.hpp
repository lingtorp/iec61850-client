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
      int file = open(file_path.c_str(), O_CREAT|O_WRONLY|O_TRUNC);
      if (file < 0) {
        std::cerr << strerror(errno) << std::endl;
        return false;
      }

      for (auto &value : values) {
        int stat = fprintf(file, "%f, %u;", value.value, value.timestamp);
        if (stat < 0) {
          std::cerr << strerror(errno) << std::endl;
          return false;
        }
      }

      return true;
    #elif
      #error OS not supported
    #endif
  }
}

#endif // _FILESYSTEM_HPP_
