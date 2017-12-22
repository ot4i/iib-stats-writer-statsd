/********************************************************* {COPYRIGHT-TOP} ***
* Copyright 2016 IBM Corporation
*
* All rights reserved. This program and the accompanying materials
* are made available under the terms of the MIT License
* which accompanies this distribution, and is available at
* http://opensource.org/licenses/MIT
********************************************************** {COPYRIGHT-END} **/

#ifndef Compat_hpp
#define Compat_hpp

#if defined(AVOID_CXX11)
# include <BipCsi.h>
# include <memory>
# include <string>

# include <stdint.h>

// Includes for to_string()
#include <sstream>
#include <iomanip>

// Need C++11 char16_t equivalent
typedef uint16_t char16_t;

namespace std
{
  // Need C++11 u16string equivalent
  typedef std::basic_string<uint16_t> u16string;

  // Need C++11 to_string equivalent
  template <class T> std::string to_string(T &val)
  {
    std::string retval;
    ostringstream formatting_buffer;

    // Fixed precision to match to_string() default for this code  
    formatting_buffer << fixed << setprecision(6) << val;
    retval = formatting_buffer.str();

    return retval;
  }
};
#endif

#endif // Compat_hpp
