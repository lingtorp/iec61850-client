#ifndef _FILESYSTEM_HPP_
#define _FILESYSTEM_HPP_

#include "measurement.hpp"

namespace FS {
  void save_data(std::vector<Measurement>, std::string file_path, bool create = true) {
    #ifdef __LINUX__
      
    #elif
      #error OS not supported
    #endif
  }
}

#endif // _FILESYSTEM_HPP_
