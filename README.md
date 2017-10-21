# iib-stats-writer-statsd

## Introduction
In IBM Integration Bus, message flow statistics and accounting data can be collected
to record performance and operating details of one or more message flows. Message flow statistics and accounting data captures dynamic information about the runtime behavior
of a message flow. For example, it indicates how many messages are processed and how
large those messages are, as well as processor usage and elapsed processing times.

The IBM Integration Bus runtime has built-in support for publishing the message flow
statistics and accounting data in XML or JSON format on MQ or MQTT topics, writing
SMF records on z/OS, or recording to user trace.

IBM Integration Bus v10.0.0.5 introduces a new C plugin API that can be used to handle
message flow statistics and accounting data directly from the IBM Integration Bus
runtime.

This repository contains a small sample C plugin written using this new API that writes the message flow statistics and accounting data to StatsD (https://github.com/etsy/statsd). StatsD can forward the data on to monitoring tools such as Graphite (http://graphite.wikidot.com) and Grafana (http://grafana.org).

## Installation
There are no releases yet. In order to try this sample, you must build the code yourself.

### Build dependencies

#### AIX only

Current build process uses Makefile.aix rather than conan; see Makefile.aix for customisation. 
Boost 1.6.0 must be downloaded manually, and either GNU GCC 6.3.0 or xlC 13 must be installed.

#### C++ compiler
You need a recent C++ compiler that supports C++11. For example, GNU GCC 4.8+ (Linux),
Microsoft Visual Studio 2015 (Windows), or Xcode 7.3 (Mac OS X). Any other compiler
that supports C++11 should be sufficient, but no other compilers have been tested.

#### Conan
This sample uses the Conan C/C++ package manager. Conan manages the projects dependencies on libraries such as Boost. Conan can be downloaded from: https://www.conan.io

#### CMake
This sample uses the excellent CMake build system. CMake can generate a variety of
project files for use with your favourite IDE or make system.
CMake can be downloaded from: https://cmake.org

#### IBM Integration Bus
You need an installation of IBM Integration Bus v10.0.0.5 or later. The most recent
open beta build can be downloaded from: https://ibm.biz/iibopenbeta

### Build instructions

1. Extract the source from GitHub:

  `git clone https://github.com/ot4i/iib-stats-writer-statsd`

2. Change into the source directory:

  `cd iib-stats-writer-statsd`

3. Run Conan to download the project dependencies:

  `conan install`

4. Run CMake to generate project files or makefiles:

  `cmake -DIIB_INSTALL_DIR=/opt/ibm/iib-10.0.0.5`

5. Run CMake to perform the build:

  `cmake --build .`

6. If the build completes successfully, then a file named **statsdsw.lil**
   will have been created in the current directory. Check that this file exists.

### Installation instructions

1. Stop all integration nodes that are using the installation:

  `mqsistop NODE`

2. Copy the built file into the installation directory:

  `sudo cp -f statsdsw.lil /opt/ibm/iib-10.0.0.5/server/lil/` (Linux and Mac OS X)  
  `copy /y statsdsw.lil "C:\Program Files\IBM\IIB\10.0.0.5\server\bin\"` (Windows)

3. Start all integration nodes again:

  `mqsistart NODE`

## Testing

The sample plugin writes portions of the statistic record as metrics to StatsD server specified by the *hostname* and *port* properties. Currently, the following metrics are written:

- hostname.nodename.servername.uniqueflowname.minimumCPUTime
- hostname.nodename.servername.uniqueflowname.maximumCPUTime
- hostname.nodename.servername.uniqueflowname.minimumElapsedTime
- hostname.nodename.servername.uniqueflowname.maximumElapsedTime
- hostname.nodename.servername.uniqueflowname.averageMessageRate
- hostname.nodename.servername.uniqueflowname.averageCPUTimePerMessage
- hostname.nodename.servername.uniqueflowname.averageElapsedTimePerMessage

In order to test this sample plugin, you will need at the very least a StatsD server. If you want to generate graphs of the data, then you will need Graphite and Grafana as well. The following Docker image contains the entire stack and is very handy for test purposes: https://github.com/kamon-io/docker-grafana-graphite

To configure and enable the sample plugin once it has been installed, follow these steps:

1. Deploy at least one message flow that can be configured for collection of message flow
   statistics and accounting data.

   *Don't have a message flow? Check out the tutorials gallery in the toolkit!*

2. Review the configured values of the installed plugin:

  `mqsireportproperties NODE -e SERVER -o StatsdStatsWriter -r`

       StatsdStatsWriter
        uuid='StatsdStatsWriter'
        userTraceLevel='none'
        traceLevel='none'
        userTraceFilter='none'
        traceFilter='debugTrace'
        hostname=''
        port=''

       BIP8071I: Successful command completion.

3. Modify the configured values of the installed plugin:

  `mqsichangeproperties NODE -e SERVER -o StatsdStatsWriter -n hostname,port -v localhost,8125`

       BIP8071I: Successful command completion.

  `mqsireportproperties NODE -e SERVER -o StatsdStatsWriter -r`

       StatsdStatsWriter
        uuid='StatsdStatsWriter'
        userTraceLevel='none'
        traceLevel='none'
        userTraceFilter='none'
        traceFilter='debugTrace'
        hostname='localhost'
        port='8125'

       BIP8071I: Successful command completion.

4. Enable message statistics and the installed plugin:

  `mqsichangeflowstats IB10NODE -s -e default -j -c active -t basic -n advanced -o statsd`

       BIP8071I: Successful command completion.

5. Wait for at least 20 seconds (the default period in-between snapshot statistic
   messages), and check Graphite or Grafana for the published values.

6. Disable message flow statistics and the installed plugin:

  `mqsichangeflowstats IB10NODE -s -e default -j -c inactive -o usertrace`

       BIP8071I: Successful command completion.
