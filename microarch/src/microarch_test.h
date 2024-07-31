#ifndef CALCULATOR_TEST_H
#define CALCULATOR_TEST_H
/*==============================================================================
  Copyright (c) 2012-2014, 2020, 2023 Qualcomm Technologies, Inc.
  All rights reserved. Qualcomm Proprietary and Confidential.
==============================================================================*/

#include "AEEStdDef.h"
#include <stdbool.h>
#include "remote.h"

#ifdef __cplusplus
extern "C" {
#endif

int microarch_test(int runMode, int domain, int num, bool isUnsignedPD_Enabled);

/**
* Method to test multiple sessions on a single domain.
* @param[in] domain_id                  Domain id to run test on
* @param[in] is_unsignedpd_enabled      Run on the Signed/Unsigned PD
* @returns                              0 on Success
*/
int microarch_multisession_test(int domain_id, bool is_unsignedpd_enabled);

#ifdef __cplusplus
}
#endif

#endif // CALCULATOR_TEST_H

