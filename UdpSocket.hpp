/********************************************************* {COPYRIGHT-TOP} ***
* Copyright 2016 IBM Corporation
*
* All rights reserved. This program and the accompanying materials
* are made available under the terms of the MIT License
* which accompanies this distribution, and is available at
* http://opensource.org/licenses/MIT
********************************************************** {COPYRIGHT-END} **/

#ifndef UdpSocket_hpp
#define UdpSocket_hpp

#include <boost/asio.hpp>
#include <string>

#if defined(AVOID_CXX11)
# include "Compat.hpp"
#endif

class UdpSocket {

public:

  UdpSocket(const std::u16string& hostname, const std::u16string& port);
  virtual ~UdpSocket();

  virtual void send(const std::string& data);
  virtual void flush();

protected:

  std::u16string iHostname;
  std::u16string iPort;
  std::string iBuffer;

  boost::asio::io_service iIOService;
  boost::asio::ip::udp::endpoint iEndpoint;
  boost::asio::ip::udp::socket iSocket;

};

#endif // UdpSocket_hpp
