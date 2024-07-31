/*==============================================================================
  Copyright (c) 2012-2014,2017,2020 Qualcomm Technologies, Inc.
  All rights reserved. Qualcomm Proprietary and Confidential.
==============================================================================*/

#include "AEEStdErr.h"
#include "microarch.h"
#include "microarch_test.h"
#include "rpcmem.h"
#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include "remote.h"
#include "os_defines.h"
#include <string.h>
#include "dsp_capabilities_utils.h"
#include "pd_status_notification.h"

#define STATUS_CONTEXT 0x12345678

int pd_status_notifier_callback(void *context, int domain, int session, remote_rpc_status_flags_t status){
    int nErr = AEE_SUCCESS;
    switch(status){
        case  FASTRPC_USER_PD_UP:
                printf( "PD is up\n");
                break;
        case  FASTRPC_USER_PD_EXIT:
                printf("PD closed\n");
                break;
        case  FASTRPC_USER_PD_FORCE_KILL:
                printf("PD force kill\n");
                break;
        case  FASTRPC_USER_PD_EXCEPTION:
                printf("PD exception\n");
                break;
        case  FASTRPC_DSP_SSR:
               printf("DSP SSR\n");
               break;
        default :
               nErr =  AEE_EBADITEM;
               break;
    }
    return nErr;
}

int local_microarch_sum(const int* vec, int vecLen, int64* res) {
  int ii = 0;
  *res = 0;
  for (ii = 0; ii < vecLen; ++ii)
    *res = *res + vec[ii];
  printf( "===============     DSP: local sum result %lld ===============\n", *res);
  return 0;
}

int local_microarch_max(const int* vec, int vecLen, int *res) {
  int ii;
  int max = 0;
  for (ii = 0; ii < vecLen; ii++)
     max = vec[ii] > max ? vec[ii] : max;
  *res = max;
  printf( "===============     DSP: local max result %d ===============\n", *res);
  return 0;
}

int microarch_test(int runLocal, int domain_id, int num, bool is_unsignedpd_enabled) {
  int nErr = AEE_SUCCESS;
  int* test = NULL;
  int ii, len = 0, resultMax = 0;
  int microarch_URI_domain_len = strlen(microarch_URI) + MAX_DOMAIN_URI_SIZE;
  int retry = 10;
  int64 result = 0;
  // remote_handle64 handleMax = -1;
  remote_handle64 handleSum = -1;
  char *microarch_URI_domain = NULL;
  domain *my_domain = NULL;

  rpcmem_init();

  len = sizeof(*test) * num;
  printf("\nAllocate %d bytes from ION heap\n", len);

  int heapid = RPCMEM_HEAP_ID_SYSTEM;
#if defined(SLPI) || defined(MDSP)
  heapid = RPCMEM_HEAP_ID_CONTIG;
#endif

  if (0 == (test = (int*)rpcmem_alloc(heapid, RPCMEM_DEFAULT_FLAGS, len))) {
    nErr = AEE_ENORPCMEMORY;
    printf("ERROR 0x%x: memory alloc failed\n", nErr);
    goto bail;
  }

  printf("Creating sequence of numbers from 0 to %d\n", num - 1);
  for (ii = 0; ii < num; ++ii)
    test[ii] = ii;

  if (runLocal) {
    printf("Compute sum locally\n");
    if (0 != local_microarch_sum(test, num, &result)) {
      nErr = AEE_EFAILED;
      printf("ERROR 0x%x: local compute sum failed\n", nErr);
      goto bail;
    }
    printf("Find max locally\n");
    if (0 != local_microarch_max(test, num, &resultMax)) {
      nErr = AEE_EFAILED;
      printf("ERROR 0x%x: local find max failed\n", nErr);
      goto bail;
    }
  } else {

    my_domain = get_domain(domain_id);
    if (my_domain == NULL) {
      nErr = AEE_EBADPARM;
      printf("\nERROR 0x%x: unable to get domain struct %d\n", nErr, domain_id);
      goto bail;
    }

    printf("Compute sum on domain %d\n", domain_id);

    if(is_unsignedpd_enabled) {
      if(remote_session_control) {
        struct remote_rpc_control_unsigned_module data;
        data.domain = domain_id;
        data.enable = 1;
        if (AEE_SUCCESS != (nErr = remote_session_control(DSPRPC_CONTROL_UNSIGNED_MODULE, (void*)&data, sizeof(data)))) {
          printf("ERROR 0x%x: remote_session_control failed\n", nErr);
          goto bail;
        }
      }
      else {
        nErr = AEE_EUNSUPPORTED;
        printf("ERROR 0x%x: remote_session_control interface is not supported on this device\n", nErr);
        goto bail;
      }
    }

    if ((microarch_URI_domain = (char *)malloc(microarch_URI_domain_len)) == NULL) {
        nErr = AEE_ENOMEMORY;
        printf("unable to allocate memory for microarch_URI_domain of size: %d", microarch_URI_domain_len);
        goto bail;
    }

    nErr = snprintf(microarch_URI_domain, microarch_URI_domain_len, "%s%s", microarch_URI, my_domain->uri);
    if (nErr < 0) {
        printf("ERROR 0x%x returned from snprintf\n", nErr);
        nErr = AEE_EFAILED;
        goto bail;
    }

    if(AEE_SUCCESS != (nErr = request_status_notifications_enable(domain_id, (void*)STATUS_CONTEXT, pd_status_notifier_callback))) {
        if(nErr != AEE_EUNSUPPORTEDAPI) {
           printf("ERROR 0x%x: request_status_notifications_enable failed\n", nErr);
        }
    }

    do {
      if (AEE_SUCCESS == (nErr = microarch_open(microarch_URI_domain, &handleSum))) {
        printf("\nCall microarch_spectest on the DSP\n");
        nErr = microarch_spectest(handleSum, test, num, &result);
      }

      if (!nErr) {
        printf("Sum pointer = 0x%llx\n", result);
        break;
      } else {
        if (nErr == AEE_ECONNRESET && errno == ECONNRESET) {
          /* In case of a Sub-system restart (SSR), AEE_ECONNRESET is returned by FastRPC
          and errno is set to ECONNRESET by the kernel.*/
          retry--;
          SLEEP(5); /* Sleep for x number of seconds */
        } else if (nErr == AEE_ENOSUCH || (nErr == (AEE_EBADSTATE + DSP_OFFSET))) {
          /* AEE_ENOSUCH is returned when Protection domain restart (PDR) happens and
          AEE_EBADSTATE is returned from DSP when PD is exiting or crashing.*/
          /* Refer to AEEStdErr.h for more info on error codes*/
          retry -= 2;
        } else {
          break;
        }
      }

      /* Close the handle and retry handle open */
      if (handleSum != -1) {
        if (AEE_SUCCESS != (nErr = microarch_close(handleSum))) {
          printf("ERROR 0x%x: Failed to close handle\n", nErr);
        }
      }
    } while(retry);

    if (nErr) {
      printf("Retry attempt unsuccessful. Timing out....\n");
      printf("ERROR 0x%x: Failed to compute sum on domain %d\n", nErr, domain_id);
    }

    // if (AEE_SUCCESS == (nErr = microarch_open(microarch_URI_domain, &handleMax))) {
    //   printf("\nCall microarch_max on the DSP\n");
    //   if (AEE_SUCCESS == (nErr = microarch_max(handleMax, test, num, &resultMax))) {
    //     printf("Max pointer = 0x%x\n", resultMax);
    //   }
    // }

    // if (nErr) {
    //   printf("ERROR 0x%x: Failed to find max on domain %d\n", nErr, domain_id);
    // }

    if (handleSum != -1) {
      if (AEE_SUCCESS != (nErr = microarch_close(handleSum))) {
        printf("ERROR 0x%x: Failed to close handleSum\n", nErr);
      }
    }

    // if (handleMax != -1) {
    //   if (AEE_SUCCESS != (nErr = microarch_close(handleMax))) {
    //     printf("ERROR 0x%x: Failed to close handleMax\n", nErr);
    //   }
    // }

  }
bail:
  if (microarch_URI_domain) {
    free(microarch_URI_domain);
  }
  if (test) {
    rpcmem_free(test);
  }
  rpcmem_deinit();
  return nErr;
}

int microarch_multisession_test(int domain_id, bool is_unsignedpd_enabled) {
    //Example of using multiple sessions
    int nErr = AEE_SUCCESS;
    int ii = 0;
    #define SESSION_ID_0 "&_session=0"
    #define FASTRPC_URI_BUF_LEN (sizeof(CDSP_DOMAIN) + sizeof(SESSION_ID_0) + 2)    //The 2 is reserved for the \n and potential double digit allocation for the session uri
    remote_handle64 handle_0, handle_1;
    int64 result0 = 0;
    int64 result1 = 0;
    int test_array_0[10] = {0};
    int test_array_1[20] = {0};
    domain *my_domain = NULL;
    my_domain = get_domain(domain_id);
    if (my_domain == NULL) {
        nErr = AEE_EBADPARM;
        printf("\nERROR 0x%x: unable to get domain struct %d\n", nErr, domain_id);
        goto bail;
    }
    printf("\nRunning Multisession Tests:\n");
    if(!remote_session_control) {
        nErr = AEE_EUNSUPPORTED;
        printf("ERROR 0x%x: remote_session_control interface is not supported on this device\n", nErr);
        goto bail;
    }
    struct remote_rpc_reserve_new_session reserve_session_0, reserve_session_1;
    struct remote_rpc_effective_domain_id data_effective_dom_id_0, data_effective_dom_id_1;
    struct remote_rpc_get_uri session_uri_0, session_uri_1;
    reserve_session_0.domain_name_len = reserve_session_1.domain_name_len = session_uri_0.domain_name_len = session_uri_1.domain_name_len = strlen(CDSP_DOMAIN_NAME);
    data_effective_dom_id_0.domain_name_len = data_effective_dom_id_1.domain_name_len= strlen(CDSP_DOMAIN_NAME);
    reserve_session_0.session_name_len = reserve_session_1.session_name_len = strlen("multi_session_test_0");
    reserve_session_0.domain_name = reserve_session_1.domain_name = session_uri_0.domain_name = session_uri_1.domain_name = data_effective_dom_id_0.domain_name = data_effective_dom_id_1.domain_name = CDSP_DOMAIN_NAME;
    reserve_session_0.session_name = "multi_session_test_0";
    reserve_session_1.session_name = "multi_session_test_1";

    session_uri_0.module_uri_len = session_uri_1.module_uri_len = strlen(microarch_URI);
    session_uri_0.uri_len = session_uri_1.uri_len = (session_uri_0.module_uri_len + FASTRPC_URI_BUF_LEN);

    if ((session_uri_0.module_uri = (char *)malloc(session_uri_0.module_uri_len + 1)) == NULL) {
        nErr = AEE_ENOMEMORY;
        printf("unable to allocate memory for uri of size: %u", (unsigned int)session_uri_0.module_uri_len);
        goto bail;
    }
    nErr = snprintf(session_uri_0.module_uri, session_uri_0.module_uri_len, "%s", microarch_URI);
    if (nErr < 0) {
        printf("ERROR 0x%x Failed to create URI for Session 0\n", nErr);
        nErr = AEE_EFAILED;
        goto bail;
    }
    //Allocate space and reserve session for session_0
    if ((session_uri_0.uri = (char *)malloc(session_uri_0.uri_len + 1)) == NULL) {
        nErr = AEE_ENOMEMORY;
        printf("unable to allocate memory for uri of size: %u", (unsigned int)session_uri_0.uri_len);
        goto bail;
    }
    if (AEE_SUCCESS != (nErr = remote_session_control(FASTRPC_RESERVE_NEW_SESSION, (void*)&reserve_session_0, sizeof(reserve_session_0)))) {
        printf("ERROR 0x%x: remote_session_control failed to reserve new session 0\n", nErr);
        goto bail;
    }
    session_uri_0.session_id = reserve_session_0.session_id;
    data_effective_dom_id_0.session_id = reserve_session_0.session_id;

    if (AEE_SUCCESS != (nErr = remote_session_control(FASTRPC_GET_EFFECTIVE_DOMAIN_ID, (void*)&data_effective_dom_id_0, sizeof(data_effective_dom_id_0)))) {
        printf("ERROR 0x%x: remote_session_control failed to reserve new session 0\n", nErr);
        goto bail;
    }
    if (AEE_SUCCESS != (nErr = remote_session_control(FASTRPC_GET_URI, (void *)&session_uri_0, sizeof(session_uri_0))) ) {
        printf("ERROR 0x%x: remote_session_control failed to get URI for session 0\n", nErr);
        goto bail;
    }
    if(is_unsignedpd_enabled) {
    printf("Setting up Unsigned PD for session 0\n");
    struct remote_rpc_control_unsigned_module data_0;
        data_0.domain = data_effective_dom_id_0.effective_domain_id;
        data_0.enable = 1;
        if (AEE_SUCCESS != (nErr = remote_session_control(DSPRPC_CONTROL_UNSIGNED_MODULE, (void*)&data_0, sizeof(data_0)))) {
            printf("ERROR 0x%x: remote_session_control failed\n", nErr);
            goto bail;
        }
    }

    //Open session 0
    printf("Creating sequence of numbers from 0 to 10\n");
    for (ii = 0; ii < 10; ii++)
        test_array_0[ii] = ii;
    if (AEE_SUCCESS == (nErr = microarch_open(session_uri_0.uri, &handle_0))){
        printf("Call microarch_spectest on the handle_0\n");
        nErr = microarch_spectest(handle_0, test_array_0, 10, &result0);
    }
    else {
        printf("\nERROR %d: failed to open handle\n", nErr);
        goto bail;
    }
    if (!nErr) {
        printf("Sum for array 0 = %lld\n\n", result0);
    } else {
        printf("ERROR is: %d", nErr);
    }

    //Allocate space and reserve session for session_1
    if ((session_uri_1.module_uri = (char *)malloc(session_uri_1.module_uri_len + 1)) == NULL) {
        nErr = AEE_ENOMEMORY;
        printf("unable to allocate memory for uri of size: %u", (unsigned int)session_uri_1.module_uri_len);
        goto bail;
    }
    nErr = snprintf(session_uri_1.module_uri, session_uri_1.module_uri_len, "%s", microarch_URI);
    if (nErr < 0) {
        printf("ERROR 0x%x Failed to create URI for Session 0\n", nErr);
        nErr = AEE_EFAILED;
        goto bail;
    }
    if ((session_uri_1.uri = (char *)malloc(session_uri_1.uri_len + 1)) == NULL) {
        nErr = AEE_ENOMEMORY;
        printf("unable to allocate memory for uri of size: %u", (unsigned int)session_uri_1.uri_len);
        goto bail;
    }
    if (AEE_SUCCESS != (nErr = remote_session_control(FASTRPC_RESERVE_NEW_SESSION, (void*)&reserve_session_1, sizeof(reserve_session_1)))) {
        printf("ERROR 0x%x: remote_session_control failed to reserve new session 0\n", nErr);
        goto bail;
    }
    session_uri_1.session_id = reserve_session_1.session_id;
    data_effective_dom_id_1.session_id = reserve_session_1.session_id;

    if (AEE_SUCCESS != (nErr = remote_session_control(FASTRPC_GET_EFFECTIVE_DOMAIN_ID, (void*)&data_effective_dom_id_1, sizeof(data_effective_dom_id_1)))) {
        printf("ERROR 0x%x: remote_session_control failed to reserve new session 0\n", nErr);
        goto bail;
    }
    if (AEE_SUCCESS != (nErr = remote_session_control(FASTRPC_GET_URI, (void *)&session_uri_1, sizeof(session_uri_1))) ) {
        printf("ERROR 0x%x: remote_session_control failed to get URI for session 1\n", nErr);
        goto bail;
    }
    if(is_unsignedpd_enabled) {
        printf("Setting up Unsigned PD for session 0\n");
        struct remote_rpc_control_unsigned_module data_1;
        data_1.domain = data_effective_dom_id_1.effective_domain_id;
        data_1.enable = 1;
        if (AEE_SUCCESS != (nErr = remote_session_control(DSPRPC_CONTROL_UNSIGNED_MODULE, (void*)&data_1, sizeof(data_1)))) {
            printf("ERROR 0x%x: remote_session_control failed\n", nErr);
            goto bail;
        }
    }
    printf("Creating sequence of numbers from 0 to 20\n");
    for (ii = 0; ii < 20; ii++)
        test_array_1[ii] = ii;

    //Open session 1
    if (AEE_SUCCESS == (nErr = microarch_open(session_uri_1.uri, &handle_1))){
        printf("Call microarch_spectest on the handle_1\n");
        nErr = microarch_spectest(handle_1, test_array_1, 20, &result1);
    }
    else {
        printf("\nERROR %d: failed to open handle\n", nErr);
        goto bail;
    }
    if (!nErr) {
        printf("Sum for array 1 = %lld\n\n", result1);
    } else {
        printf("ERROR is: %d", nErr);
    }

    if (handle_0 != -1) {
      if (AEE_SUCCESS != (nErr = microarch_close(handle_0))) {
        printf("ERROR 0x%x: Failed to close handle_0\n", nErr);
      }
    }
    if (handle_1 != -1) {
      if (AEE_SUCCESS != (nErr = microarch_close(handle_1))) {
        printf("ERROR 0x%x: Failed to close handle_1\n", nErr);
      }
    }

bail:
  if (session_uri_0.module_uri) {
    free(session_uri_0.module_uri);
  }
  if (session_uri_0.uri) {
    free(session_uri_0.uri);
  }

  if (session_uri_1.module_uri) {
    free(session_uri_1.module_uri);
  }
  if (session_uri_1.uri) {
    free(session_uri_1.uri);
  }
  return nErr;
}
