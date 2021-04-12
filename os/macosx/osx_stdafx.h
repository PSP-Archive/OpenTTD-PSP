/* $Id: osx_stdafx.h 3596 2006-02-12 14:31:33Z bjarni $ */

#ifndef MACOS_STDAFX_H
#define MACOS_STDAFX_H

#include <CoreServices/CoreServices.h>
// remove the variables that CoreServices defines, but we define ourselves too
#undef bool
#undef false
#undef true

/* Name conflict */
#define Rect		OTTDRect
#define Point		OTTDPoint
#define GetTime		OTTDGetTime

#define SL_ERROR OSX_SL_ERROR

#endif /* MACOS_STDAFX_H */
