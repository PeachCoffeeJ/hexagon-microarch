#include "microarch_test.h"
#include "rpcmem.h"
#include "remote.h"
#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "dsp_capabilities_utils.h"     // available under <HEXAGON_SDK_ROOT>/utils/examples
#include "os_defines.h"

static void print_usage()
{
  printf( "Usage:\n"
    "    calculator [-d domain] [-U unsigned_PD] [-r run_locally] [-q multisession_test] -n array_size\n\n"
    "Options:\n"
    "-d domain: Run on a specific domain.\n"
    "    0: Run the example on ADSP\n"
    "    3: Run the example on CDSP\n"
    "    1: Run the example on MDSP\n"
    "    2: Run the example on SDSP\n"
    "        Default Value: 3(CDSP) for targets having CDSP and 0(ADSP) for targets not having CDSP like Agatti.\n"
    "-U unsigned_PD: Run on signed or unsigned PD.\n"
    "    0: Run on signed PD.\n"
    "    1: Run on unsigned PD.\n"
    "        Default Value: 1\n"
    "-r run_locally:\n"
    "    1: Run locally on APPS.\n"
    "    0: Run on DSP.\n"
    "        Default Value: 1\n"
    "-q multisession_test: Test to create multiple sessions parallely calculating sums for predetermined arrays\n"
    "    0: Does not run the multisession tests\n"
    "    1: Run the multisession tests\n"
    "        Default Value: 0\n"
    "-n array_size: Natural number up to which sum is calculated from 0 to (n-1)\n"
    "        Default Value: 1\n"
    );
}

int main(int argc, char* argv[])
{
  int nErr = 0;
  int runLocal = 0;
  int num = 1;
  int domain_id = -1;
  int unsignedpd_flag = 1;
  bool is_unsignedpd_enabled = false;
  bool multisession_test = false;
  int option = 0;

  while ((option = getopt(argc, argv,"d:U:q:r:n:")) != -1) {
    switch (option) {
      case 'd' : domain_id = atoi(optarg);
        break;
      case 'U' : unsignedpd_flag = atoi(optarg);
        break;
      case 'q' : multisession_test = atoi(optarg);
        break;
      case 'r' : runLocal = atoi(optarg);
        break;
      case 'n' : num = atoi(optarg);
        break;
      default:
        print_usage();
      return -1;
    }
  }

  if(runLocal < 0 || runLocal > 1) {
    printf("Please provide a valid run local value (0/1).\n");
    nErr = -1;
    print_usage();
    goto bail;
  }

  if(runLocal == 1){
    goto starttest;
  }

  if (domain_id == -1) {
    printf("\nDSP domain is not provided. Retrieving DSP information using Remote APIs.\n");
    nErr = get_dsp_support(&domain_id);
    if (nErr != AEE_SUCCESS) {
      printf("ERROR in get_dsp_support: 0x%x, defaulting to CDSP domain\n", nErr);
    }
  }

  if (!is_valid_domain_id(domain_id, 0)) {
    nErr = AEE_EBADPARM;
    printf("\nERROR 0x%x: Invalid domain %d\n", nErr, domain_id);
    print_usage();
    goto bail;
  }

  if (unsignedpd_flag < 0 || unsignedpd_flag > 1) {
    nErr = AEE_EBADPARM;
    printf("\nERROR 0x%x: Invalid unsigned PD flag %d\n", nErr, unsignedpd_flag);
    print_usage();
    goto bail;
  }

  if(unsignedpd_flag == 1) {
    is_unsignedpd_enabled = is_unsignedpd_supported(domain_id);
    if (!is_unsignedpd_enabled) {
      printf("Overriding user request for unsigned PD. Only signed offload is allowed on domain %d.\n", domain_id);
      unsignedpd_flag = 0;
    }
  }

//No need to check for domain and unsigned_PD, if runlocal is used 
starttest:
  if(num <= 0) {
    printf("Please provide a valid array size value.\n");
    nErr = -1;
    print_usage();
    goto bail;
  }

  printf("\nStarting microarch test\n");
  if(runLocal == 0) {
    printf("Attempting to run on %s PD on domain %d\n", unsignedpd_flag==1?"unsigned":"signed", domain_id);
  }

  if(multisession_test == true) {
    nErr = microarch_multisession_test(domain_id, is_unsignedpd_enabled);
    if (nErr) {
      printf("ERROR 0x%x: microarch test failed\n\n", nErr);
    }
  } else {
    nErr = microarch_test(runLocal, domain_id, num, is_unsignedpd_enabled);
    if (nErr) {
      printf("ERROR 0x%x: microarch test failed\n\n", nErr);
    }
  }

bail:
  if (nErr) {
    printf("ERROR 0x%x: microarch example failed\n\n", nErr);
  } else {
    printf("Success\n\n");
  }

  return nErr;
}
