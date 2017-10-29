/********************************************************* {COPYRIGHT-TOP} ***
* Copyright 2017 IBM Corporation
*
* All rights reserved. This program and the accompanying materials
* are made available under the terms of the MIT License
* which accompanies this distribution, and is available at
* http://opensource.org/licenses/MIT
********************************************************** {COPYRIGHT-END} **/

#include "BipCsi.h"      //! Typedefs for stub functions
#include <gmock/gmock.h> //! gtest support


/* From BipCci.h and BipCsi.h - we need to implement these as stubs to avoid linking */
/* failures; if we didn't use stubs we would have to create an executable with every */
/* IIB shared library link in, and that brings a lot of static initialisers that are */
/* complicated to keep happy.                                                        */
void ImportExportPrefix ImportExportSuffix cciLogWithInsertsW(
  int*               returnCode,
  CCI_LOG_TYPE       type,
  const char*        file,
  int                line,
  const char*        function,
  const CciChar*     messageSource,
  int                messageNumber,
  const CciChar*     traceText,
  const CciChar**    inserts,
  CciSize            numInserts)
{
  // Ignore this call
}
CsiStatsWriter ImportExportPrefix * ImportExportSuffix csiCreateStatsWriter(
  int* returnCode,
  const CciChar* resourceName,
  const CciChar* formatName,
  const CsiStatsWriterVft* vft,
  void* context)
{
  // Ignore this call
  *returnCode = CCI_SUCCESS;
  return NULL;
}

//! Run the tests in the other files and report back
int main(int argc, char *argv[])
{
  ::testing::InitGoogleTest(&argc, argv);
  int rc = RUN_ALL_TESTS();

  return rc;
}

