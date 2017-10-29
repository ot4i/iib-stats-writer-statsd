/********************************************************* {COPYRIGHT-TOP} ***
* Copyright 2016 IBM Corporation
*
* All rights reserved. This program and the accompanying materials
* are made available under the terms of the MIT License
* which accompanies this distribution, and is available at
* http://opensource.org/licenses/MIT
********************************************************** {COPYRIGHT-END} **/

#include "StatsdStatsWriter.hpp"
#include "UdpSocket.hpp"

#include <algorithm>
#include <boost/asio/ip/host_name.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/locale.hpp>
#include <cmath>
#include <exception>

using namespace boost::asio::ip;
using boost::locale::conv::utf_to_utf;

extern "C" {

  /*
   * This function is called by the IBM Integration Bus runtime when it loads the LIL file.
   * It is expecting a CsiStatsWriter object to be created using csiCreateStatsWriter() and
   * returned. The CsiStatsWriter is registered after this function has been called.
   */
  CsiStatsWriter LilFactoryExportPrefix * LilFactoryExportSuffix bipGetStatsWriter() {
    try {
      StatsdStatsWriter* instance = new StatsdStatsWriter();
      return instance->writer();
    } catch (const std::exception& e) {
      return nullptr;
    }
  }

}

namespace {

  /*
   * This is the C callback for getAttributeName(); simply forward the call onto the C++ object.
   */
  CciSize getAttributeNameCallback(int* rc, int index, CciChar* buffer, CciSize bufferLength, void* context) {
    StatsdStatsWriter* writer = reinterpret_cast<StatsdStatsWriter*>(context);
    return writer->getAttributeName(rc, index, buffer, bufferLength);
  }

  /*
   * This is the C callback for getAttribute(); simply forward the call onto the C++ object.
   */
  CciSize getAttributeCallback(int* rc, const CciChar* name, CciChar* buffer, CciSize bufferLength, void* context) {
    StatsdStatsWriter* writer = reinterpret_cast<StatsdStatsWriter*>(context);
    return writer->getAttribute(rc, name, buffer, bufferLength);
  }

  /*
   * This is the C callback for setAttribute(); simply forward the call onto the C++ object.
   */
  void setAttributeCallback(int* rc, const CciChar* name, const CciChar* value, void* context) {
    StatsdStatsWriter* writer = reinterpret_cast<StatsdStatsWriter*>(context);
    writer->setAttribute(rc, name, value);
  }

  /*
   * This is the C callback for write(); simply forward the call onto the C++ object.
   */
  void writeCallback(const CsiStatsRecord* record, void* context) {
    StatsdStatsWriter* writer = reinterpret_cast<StatsdStatsWriter*>(context);
    writer->write(record);
  }

  /*
   * This is the name of the statistics writer resource. This is the name that should
   * be passed to the mqsichangeproperties or mqsireportproperties commands to change
   * or report on properties of this statistics writer, for example:
   *
   * mqsireportproperties NODE -e SERVER -o GettingStartedStatsWriter -r
   * mqsichangeproperties NODE -e SERVER -o GettingStartedStatsWriter -n property1,property2 -v value1,value2
   */
  const std::u16string RESOURCE_NAME(u"StatsdStatsWriter");

  /*
   * This is the name of the "format" written by this statistics writer. This is the
   * name that should be passed to mqsichangeflowstats command to enable writing
   * statistics using this statistics writer for a specific message flow or set of
   * message flows, for example:
   *
   * mqsichangeflowstats NODE -s -e SERVER -f FLOW -c active -o gettingstarted
   */
  const std::u16string FORMAT_NAME(u"statsd");

  /*
   * This is the name of a property of this statistics writer. The value of the
   * property can be accessed using the mqsichangeproperties or mqsireportproperties
   * commands (see above).
   */
  const std::u16string HOSTNAME_NAME(u"hostname");

  /*
   * This is the name of a property of this statistics writer. The value of the
   * property can be accessed using the mqsichangeproperties or mqsireportproperties
   * commands (see above).
   */
  const std::u16string PORT_NAME(u"port");

}

/*
 * Constructor.
 */
StatsdStatsWriter::StatsdStatsWriter(UdpSocket *socket)
 : iWriter(nullptr) 
{
  /*
   * Set the socket initially to the passed-in socket if it has
   * been set; this is normally used for unit testing with a mock
   * socket.
   */
  if ( socket != NULL )
    iSocket.reset(socket);

  /*
   * Create the virtual function table for the statistics writer.
   * These functions will be called by the IBM Integration Bus
   * runtime when it needs to interact with the statistics writer.
   */
  CsiStatsWriterVft vft = { CSI_STATS_WRITER_VFT_DEFAULT };
  vft.getAttributeName = &getAttributeNameCallback;
  vft.getAttribute = &getAttributeCallback;
  vft.setAttribute = &setAttributeCallback;
  vft.write = &writeCallback;

  /*
   * Create the statistics writer, passing it the virtual function
   * table, resource name, and format name that are defined above.
   * If this function returns an error, then throw an exception so
   * that we will return nullptr as the result of the bipGetStatsWriter()
   * call.
   */
  int returnCode = CCI_FAILURE;
  iWriter = csiCreateStatsWriter(
    &returnCode,
    RESOURCE_NAME.c_str(),
    FORMAT_NAME.c_str(),
    &vft,
    this
  );
  if (returnCode != CCI_SUCCESS) {
    const char16_t* traceText = u"Call to csiCreateStatsWriter failed";
    const char16_t* inserts[] = { traceText, RESOURCE_NAME.c_str(), FORMAT_NAME.c_str() };
    cciLogWithInsertsW(nullptr, CCI_LOG_ERROR,__FILE__, __LINE__, __func__, u"BIPmsgs", 2113, traceText, inserts, sizeof(inserts) / sizeof(inserts[0]));
    throw std::exception();
  }

}

/*
 * Destructor.
 */
StatsdStatsWriter::~StatsdStatsWriter() {

}

/*
 * Called by the IBM Integration Bus runtime to get the name of the property at the
 * specified index. If no property exists at the specified index, then this function
 * should return CCI_ATTRIBUTE_UNKNOWN. If the specified buffer is too small for the
 * name of the property, then this function should return CCI_BUFFER_TOO_SMALL and
 * the required size of the buffer.
 */
CciSize StatsdStatsWriter::getAttributeName(int* rc, int index, CciChar* buffer, CciSize bufferLength) const {
  switch (index) {
  case 0:
    if (bufferLength < HOSTNAME_NAME.length()) {
      if (rc) *rc = CCI_BUFFER_TOO_SMALL;
      return HOSTNAME_NAME.length();
    }
    if (rc) *rc = CCI_SUCCESS;
    return HOSTNAME_NAME.copy(buffer, HOSTNAME_NAME.length());
  case 1:
    if (bufferLength < PORT_NAME.length()) {
      if (rc) *rc = CCI_BUFFER_TOO_SMALL;
      return PORT_NAME.length();
    }
    if (rc) *rc = CCI_SUCCESS;
    return PORT_NAME.copy(buffer, PORT_NAME.length());
  default:
    *rc = CCI_ATTRIBUTE_UNKNOWN;
    return 0;
  }
}

/*
 * Called by the IBM Integration Bus runtime to get the value of the property with the
 * specified name. If no property exists with the specified name, then this function
 * should return CCI_ATTRIBUTE_UNKNOWN. If the specified buffer is too small for the
 * value of the property, then this function should return CCI_BUFFER_TOO_SMALL and
 * the required size of the buffer.
 */
CciSize StatsdStatsWriter::getAttribute(int* rc, const CciChar* name, CciChar* buffer, CciSize bufferLength) const {
  if (HOSTNAME_NAME == name) {
    if (bufferLength < iHostname.length()) {
      if (rc) *rc = CCI_BUFFER_TOO_SMALL;
      return iHostname.length();
    }
    if (rc) *rc = CCI_SUCCESS;
    return iHostname.copy(buffer, iHostname.length());
  } else if (PORT_NAME == name) {
    if (bufferLength < iPort.length()) {
      if (rc) *rc = CCI_BUFFER_TOO_SMALL;
      return iPort.length();
    }
    if (rc) *rc = CCI_SUCCESS;
    return iPort.copy(buffer, iPort.length());
  } else {
    if (rc) *rc = CCI_ATTRIBUTE_UNKNOWN;
    return 0;
  }
}

/*
 * Called by the IBM Integration Bus runtime to set the value of the property with the
 * specified name. If no property exists with the specified name, then this function
 * should return CCI_ATTRIBUTE_UNKNOWN.
 */
void StatsdStatsWriter::setAttribute(int* rc, const CciChar* name, const CciChar* value) {
  if (HOSTNAME_NAME == name) {
    iHostname.assign(value);
    if (rc) *rc = CCI_SUCCESS;
  } else if (PORT_NAME == name) {
    iPort.assign(value);
    if (rc) *rc = CCI_SUCCESS;
  } else {
    if (rc) *rc = CCI_ATTRIBUTE_UNKNOWN;
  }
  if (!iHostname.empty() && !iPort.empty()) {
    iSocket.reset(new UdpSocket(iHostname, iPort));
  } else {
    iSocket.reset();
  }
}

/*
 * Called by the IBM Integration Bus runtime to write the specified statistics record.
 * The statistics record contains data for one message flow with statistics enabled, so
 * this function will be called once per message flow.
 */
void StatsdStatsWriter::write(const CsiStatsRecord* record) {

  /*
   * If not connected, or we haven't been configured, then bail out early.
   */
#if defined(AVOID_CXX11)
  if (iSocket.get() == NULL) { return; }
#else
    if (!iSocket) { return; }
#endif

  /*
   * Calculate the base name for all of the metrics. This is as follows:
   * hostname.nodename.servername.uniqueflowname
   */
  std::u16string hostname(utf_to_utf<char16_t>(host_name()));
  hostname = hostname.substr(0, hostname.find(u'.'));
  std::u16string nodename(record->messageFlow.brokerLabel);
  std::replace(nodename.begin(), nodename.end(), u'.', u'_');
  std::u16string servername(record->messageFlow.executionGroupName);
  std::replace(servername.begin(), servername.end(), u'.', u'_');
  std::u16string uniqueservername;
  uniqueservername += hostname + u'.';
  if (!nodename.empty()) {
    uniqueservername += nodename + u'.';
  }
  if (!servername.empty()) {
    uniqueservername += servername + u'.';
  }

  /*
   * The unique flow name is built from the application, library, and
   * message flow names.
   */
  std::u16string application(record->messageFlow.applicationName);
  std::replace(application.begin(), application.end(), u'.', u'_');
  std::u16string library(record->messageFlow.libraryName);
  std::replace(library.begin(), library.end(), u'.', u'_');
  std::u16string messageflow(record->messageFlow.messageFlowName);
  std::replace(messageflow.begin(), messageflow.end(), u'.', u'_');
  std::u16string uniqueflowname;
  if (!application.empty()) {
    uniqueflowname += application + u'.';
  }
  if (!library.empty()) {
    uniqueflowname += library + u'.';
  }
  uniqueflowname += messageflow;
  std::u16string metricbase = uniqueservername + uniqueflowname + u'.';

  /*
   * Calculate the time interval for this record.
   */
  uint64_t startMillis = calculateMillis(record->messageFlow.startDate, record->messageFlow.startTime);
  uint64_t endMillis = calculateMillis(record->messageFlow.endDate, record->messageFlow.endTime);
  uint64_t duration = endMillis - startMillis;

  /*
   * Generate and send all of the metrics.
   */
  writeMessageFlowMetrics(metricbase, record, duration);

  /*
   * Ensure that all data is written to the socket. The UdpSocket class will
   * package the metrics into as few UDP packets as possible.
   */
  iSocket->flush();

}

/*
 * Calculate the time in milliseconds since the epoch from the specified date and time.
 */
uint64_t StatsdStatsWriter::calculateMillis(const CciDate& date, const CciTime& time) {
  struct tm temp = { 0 };
  temp.tm_year = date.year;
  temp.tm_mon = date.month;
  temp.tm_mday = date.day;
  temp.tm_hour = time.hour;
  temp.tm_min = time.minute;
  double seconds;
  uint64_t millis = static_cast<uint64_t>(modf(time.second, &seconds) * 1000);
  temp.tm_sec = static_cast<int>(seconds);
  time_t epochtime = mktime(&temp);
  return (epochtime * 1000) + millis;
}

/*
 * Write all the message flow specific metrics from the specified statistics record.
 */
void StatsdStatsWriter::writeMessageFlowMetrics(const std::u16string& metricbase, const CsiStatsRecord* record, uint64_t duration) {

  /*
   * Minimum and maximum CPU time and elapsed time in seconds.
   */
  writeMetric(metricbase, u"minimumCPUTime", record->messageFlow.minimumCPUTime / 1000.0f);
  writeMetric(metricbase, u"maximumCPUTime", record->messageFlow.maximumCPUTime / 1000.0f);
  writeMetric(metricbase, u"minimumElapsedTime", record->messageFlow.minimumElapsedTime / 1000.0f);
  writeMetric(metricbase, u"maximumElapsedTime", record->messageFlow.maximumElapsedTime / 1000.0f);

  /*
   * Average message rate in messages/second.
   */
  double averageMessageRate = 0;
  if (record->messageFlow.totalInputMessages > 0) {
    averageMessageRate = record->messageFlow.totalInputMessages / (duration / 1000.0f);
  }
  writeMetric(metricbase, u"averageMessageRate", averageMessageRate);

  /*
   * Average CPU time per message in seconds.
   */
  double averageCPUTimePerMessage = 0;
  if (record->messageFlow.totalInputMessages > 0) {
    averageCPUTimePerMessage = (record->messageFlow.totalCPUTime / static_cast<double>(record->messageFlow.totalInputMessages)) / 1000.0f;
  }
  writeMetric(metricbase, u"averageCPUTimePerMessage", averageCPUTimePerMessage);

  /*
   * Average elapsed time per message in seconds.
   */
  double averageElapsedTimePerMessage = 0;
  if (record->messageFlow.totalInputMessages > 0) {
    averageElapsedTimePerMessage = (record->messageFlow.totalElapsedTime / static_cast<double>(record->messageFlow.totalInputMessages)) / 1000.0f;
  }
  writeMetric(metricbase, u"averageElapsedTimePerMessage", averageElapsedTimePerMessage);

}

/*
 * Write a single metric.
 */
template <class T>
void StatsdStatsWriter::writeMetric(const std::u16string& metricbase, const std::u16string& metricname, T value) {
  std::string metric(utf_to_utf<char>(metricbase) + utf_to_utf<char>(metricname));
  metric += ':';
  metric += std::to_string(value);
  metric += "|g";
  iSocket->send(metric);
}
