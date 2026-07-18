/* Common hooks for SRA32.
   This file is part of GCC.  */

#include "config.h"
#include "system.h"
#include "coretypes.h"
#include "tm.h"
#include "common/common-target.h"
#include "common/common-target-def.h"

/* Use default hooks from common-target-def.h */

#include "common/common-target-hooks-def.h"

struct gcc_targetm_common targetm_common = TARGETM_COMMON_INITIALIZER;