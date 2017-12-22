/********************************************************* {COPYRIGHT-TOP} ***
* Copyright 2017 IBM Corporation
*
* All rights reserved. This program and the accompanying materials
* are made available under the terms of the MIT License
* which accompanies this distribution, and is available at
* http://opensource.org/licenses/MIT
********************************************************** {COPYRIGHT-END} **/


#include "StatsdStatsWriter.hpp" //! Product code
#include "UdpSocket.hpp"         //! Product code

#include <gmock/gmock.h> //! gtest/gmock support
using namespace ::testing;


#include <boost/asio/ip/host_name.hpp>
#include <boost/locale.hpp>
using boost::locale::conv::utf_to_utf;
using namespace boost::asio::ip; //! host_name()

//! 
//! Mock class to allow checking of UdpSocket calls via the StatsdStatsWriter
//! class. We're interested in checking that send() is called with the values
//! we expect, including correct numerical precision and unicode conversion.
//! 
class FakeUdpSocket: public UdpSocket {
public:
  //! testData is generated independently from the StatsdStatsWriter and used to
  //! check the information being passed in.
  FakeUdpSocket(const std::u16string& hostname, const std::u16string& port, std::string testData)
  : UdpSocket(hostname, port),
    iTestData(testData)
  {
  };
  virtual ~FakeUdpSocket() 
  {
  };

  //! Validate the data when called from StatsdStatsWriter
  virtual void fakeSend(const std::string&data)
  {
    std::string dataString = utf_to_utf<char>(data);
    EXPECT_EQ(iTestData, data) << "Failed to match data in string " << dataString;
  }

  //! Mock the relevant methods; send is forwarded to fakeSend as needed.
  MOCK_METHOD1(send, void(const std::string&));
  MOCK_METHOD0(flush, void());

  std::string iTestData;
};


//! Main test fixture class; responsible for creating basic test data structures.
class StatsdStatsWriter_UnitTest: public ::testing::Test 
{
public:

  StatsdStatsWriter_UnitTest()
  {
    memset((void *)&zeroDate, 0, sizeof(zeroDate));
    memset((void *)&zeroTime, 0, sizeof(zeroTime));
    memset((void *)&zeroGmtTime, 0, sizeof(zeroGmtTime));
    memset((void *)&iMFRecord, 0, sizeof(iMFRecord));
    memset((void *)&iRecord, 0, sizeof(iRecord));

    iMFRecord.brokerLabel = u"dummyBroker";
    iMFRecord.brokerUUID = u"a";
    iMFRecord.executionGroupName = u"b";
    iMFRecord.executionGroupUUID = u"c";
    iMFRecord.messageFlowName = u"d";
    iMFRecord.messageFlowUUID = u"e";
    iMFRecord.applicationName = u"f";
    iMFRecord.applicationUUID = u"g";
    iMFRecord.libraryName = u"h";
    iMFRecord.libraryUUID = u"i";
    iMFRecord.startDate = zeroDate;
    iMFRecord.startTime = zeroTime;
    iMFRecord.gmtStartTime = zeroGmtTime;
    iMFRecord.endDate = zeroDate;
    iMFRecord.endTime = zeroTime;
    iMFRecord.gmtEndTime = zeroGmtTime;
    iMFRecord.accountingOrigin = u"j";

    iRecord.version = CSI_STATS_RECORD_VERSION_1;
    iRecord.type = CSI_STATS_RECORD_TYPE_SNAPSHOT;
    iRecord.code = CSI_STATS_RECORD_CODE_SNAPSHOT;
    iRecord.messageFlow = iMFRecord;
  }

  struct CciDate zeroDate;
  struct CciTime zeroTime;
  struct CciTimestamp zeroGmtTime;

  CsiStatsRecordMessageFlow iMFRecord;
  CsiStatsRecord            iRecord;
};

/** 
 *  Test: Check statsd writer can be constructed
 *        
 */
TEST_F(StatsdStatsWriter_UnitTest, construction)
{
  StatsdStatsWriter *testStatsdStatsWriter = new StatsdStatsWriter();
  EXPECT_THAT(testStatsdStatsWriter, NotNull());
  delete testStatsdStatsWriter;
}

/** 
 *  Test: Check statsd writer can write unicode to mock UDP socket
 *        and have it formatted correctly.
 */
TEST_F(StatsdStatsWriter_UnitTest, udpSocketWriteWithUnicode)
{
  // Construct test data, and override the fixture class to get the right broker label
  char16_t simplifiedChineseWchars[] = { 0x4ED6 , 0x4EEC , 0x4E3A , 0x4EC0 , 0x4E48 , 0x4E0D , 0x8BF4 , 0x4E2D , 0x6587, 0x0000 };
  iRecord.messageFlow.brokerLabel = simplifiedChineseWchars;

  // Expect this to be sent via the UdpSocket
  char     simplifiedChineseUTF8[]   = { (char)0xE4 , (char)0xBB , (char)0x96 , (char)0xE4 , (char)0xBB , 
                                         (char)0xAC , (char)0xE4 , (char)0xB8 , (char)0xBA , (char)0xE4 , 
                                         (char)0xBB , (char)0x80 , (char)0xE4 , (char)0xB9 , (char)0x88 , 
                                         (char)0xE4 , (char)0xB8 , (char)0x8D , (char)0xE8 , (char)0xAF , 
                                         (char)0xB4 , (char)0xE4 , (char)0xB8 , (char)0xAD , (char)0xE6 ,
                                         (char)0x96 , (char)0x87};

  // Format the complete expected string for the first send call
  std::string    chineseName(simplifiedChineseUTF8, 27);
  std::string    hostname(host_name()); // Taken from StatsStatsWriter.cpp
  hostname = hostname.substr(0, hostname.find('.'));
  std::string    expectedData = hostname + "." + chineseName + ".b.f.h.d.minimumCPUTime:0.000000|g";

  // Set up the mock
  StrictMock<FakeUdpSocket> *fakeUdp = new StrictMock<FakeUdpSocket>(u"localhost", u"65535", expectedData);
  StatsdStatsWriter testStatsdStatsWriter(fakeUdp); // The stats writer will free the mock

  // Only validate the first record; the algorithm is the same for the other
  // send calls, and we're only checking float precision and unicode handling.
  EXPECT_CALL(*fakeUdp, send(_)).Times(7)
    .WillOnce(Invoke(fakeUdp, &FakeUdpSocket::fakeSend))
    .WillRepeatedly(Return());

  EXPECT_CALL(*fakeUdp, flush());

  testStatsdStatsWriter.write(&iRecord);

  // Mock will validate on exit - testStatsdStatsWriter deletes the mock on 
  // destruction, triggering validation of EXPECT_CALLs. 
  //
  // Data validation errors will have been flagged already in fakeSend().
}
