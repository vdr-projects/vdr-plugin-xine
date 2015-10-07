
#ifndef __XINECOMMON_H
#define __XINECOMMON_H



#include <assert.h>
#include <math.h>
#include <signal.h>

#define __STDC_FORMAT_MACROS
#include <inttypes.h>

#include <string>
using namespace std;

#include <xine.h>
#include <xine/vdr.h>

#define MIN_XINE_VDR_VERSION 901

#if !defined(XINE_VDR_VERSION) || XINE_VDR_VERSION < MIN_XINE_VDR_VERSION
#error xine/vdr.h does not match. Please solve this issue by reading section XINE VDR VERSION MISMATCH in INSTALL!
#endif

#include <vdr/config.h>  // poisened

#ifndef APIVERSNUM
#define APIVERSNUM VDRVERSNUM
#endif



namespace PluginXine
{
  extern bool beQuiet;

  template <class DST_TYPE, class SRC_TYPE>
    DST_TYPE &alias_cast(SRC_TYPE &rhs)
  {
    union hlp
    {
      SRC_TYPE src;
      DST_TYPE dst;
    };

    return ((hlp &)rhs).dst;
  }
};

#define xfprintf(fh, fmt, args...)   \
  while (!PluginXine::beQuiet)       \
  {                                  \
    fprintf(fh, fmt, ##args);        \
    /* char xfmt[ 500 ]; */          \
    /* sprintf(xfmt, "%s", fmt); */  \
    /* fprintf(fh, xfmt, ##args); */ \
    break;                           \
  }



#endif //__XINECOMMON_H
