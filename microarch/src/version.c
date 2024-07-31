/*==============================================================================
  Copyright (c) 2022, 2023 Qualcomm Technologies, Inc.
  All rights reserved. Qualcomm Proprietary and Confidential.
==============================================================================*/
#include "version_note.h"

/* Library version needs to be added in the name member of note_type structure in below format
 * "lib.ver.1.0.0." + "<library_name>" + ":" + "<version>"
 */
const lib_ver_note_t so_ver __attribute__ ((section (".note.lib.ver")))
      __attribute__ ((visibility ("default"))) = {
  100,
  0,
  0,
  "lib.ver.1.0.0.libmicroarch_skel.so:4.5.0",
};
