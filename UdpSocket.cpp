/********************************************************* {COPYRIGHT-TOP} ***
* Copyright 2016 IBM Corporation
*
* All rights reserved. This program and the accompanying materials
* are made available under the terms of the MIT License
* which accompanies this distribution, and is available at
* http://opensource.org/licenses/MIT
********************************************************** {COPYRIGHT-END} **/

#include "UdpSocket.hpp"

#include <boost/locale.hpp>

using boost::asio::ip::udp;
using boost::locale::conv::utf_to_utf;

namespace {

  /*
   * This is apparently the safest UDP packet size, suitable for transmission
   * across the internet. I suspect it's overkill and can be increased.
   */
  const size_t UDP_MAX_PACKET_SIZE = 508;
  
}

UdpSocket::UdpSocket(const std::u16string& hostname, const std::u16string& port)
 : iHostname(hostname),
   iPort(port),
   iSocket(iIOService) {
  udp::resolver resolver(iIOService);
  udp::resolver::query query(udp::v4(), utf_to_utf<char>(hostname), utf_to_utf<char>(port));
  iEndpoint = *resolver.resolve(query);
  iSocket.open(udp::v4());
}

UdpSocket::~UdpSocket() {

}

void UdpSocket::send(const std::u16string& data) {
  if ((iBuffer.length() + 1 + data.length()) > UDP_MAX_PACKET_SIZE) {
    flush();
  }
  if (!iBuffer.empty()) {
    iBuffer += u'\n';
  }
  iBuffer += data;
}

void UdpSocket::flush() {
  iSocket.send_to(boost::asio::buffer(utf_to_utf<char>(iBuffer)), iEndpoint);
  iBuffer.clear();
}
