/*
The newlib subdirectory is a collection of software from several sources.

Each file may have its own copyright/license that is embedded in the source
file.  Unless otherwise noted in the body of the source file(s), the following copyright
notices will apply to the contents of the newlib subdirectory:

(1) Red Hat Incorporated

Copyright (c) 1994-2009  Red Hat, Inc. All rights reserved.

This copyrighted material is made available to anyone wishing to use,
modify, copy, or redistribute it subject to the terms and conditions
of the BSD License.   This program is distributed in the hope that
it will be useful, but WITHOUT ANY WARRANTY expressed or implied,
including the implied warranties of MERCHANTABILITY or FITNESS FOR
A PARTICULAR PURPOSE.  A copy of this license is available at
http://www.opensource.org/licenses. Any Red Hat trademarks that are
incorporated in the source code or documentation are not subject to
the BSD License and may only be used or replicated with the express
permission of Red Hat, Inc.
*/
#include <sys/types.h>
#include "local.h"

/* Shared timezone information for libc/time functions.  */
static __tzinfo_type tzinfo = {1, 0,
    { {'J', 0, 0, 0, 0, (time_t)0, 0L },
      {'J', 0, 0, 0, 0, (time_t)0, 0L }
    }
};

__tzinfo_type *
__gettzinfo (void)
{
    return &tzinfo;
}
