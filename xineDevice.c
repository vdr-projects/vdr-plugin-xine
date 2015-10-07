
#include "xineCommon.h"

#include <vdr/remux.h>
#include <vdr/transfer.h>

#if APIVERSNUM >= 10703
#include "vdr172remux.h"
#define VDR172 ::vdr172
#else
#define VDR172
#endif

#include "xineDevice.h"
#include "xineOsd.h"
#include "xineSettings.h"


//#define LOG_ME(x) x
#define LOG_ME(x)


namespace PluginXine
{
  void cXineSpuDecoder::ptsAdjust(uint32_t &pts)
  {
    if (0 == pts
        || -1 == (int32_t)pts)
    {
      return;
    }
    
    const int64_t ptsXine = m_xineDevice->GetSTC();

    if (-1 == ptsXine)
      return;
    
//    ::fprintf(stderr, "ptsAdjust: %ld, %lld, %lld\n", pts, ptsXine, pts - ptsXine);

    pts = (uint32_t)ptsXine;
  }
  
  int cXineSpuDecoder::setTime(uint32_t pts)
  {
    ptsAdjust(pts);
    
    return cDvbSpuDecoder::setTime(pts);
  }

  static cXineDevice *theXineDevice = 0;

  bool cXineDevice::HasDecoder(void) const
  {
    return true;
  }

  cSpuDecoder *cXineDevice::GetSpuDecoder(void)
  {
    if (!m_spuDecoder
        && IsPrimaryDevice())
    {
      m_spuDecoder = new cXineSpuDecoder(this);
    }
    
    return m_spuDecoder;
  }

  bool cXineDevice::CanReplay(void) const
  {
    return true;
  }

  static bool findVideo = false;
  static bool foundVideo = false;
  static int ts = 0;
  static bool f = false;
  static bool np = false;
  static bool muted = false;
  static int jumboPESsize = 0;
  static int jumboPEStail = 0;
  static ePlayMode pm = pmNone;
  static bool audioSeen = false;

  static int64_t ptsV = -1, ptsA = -1, ptsP = -1, ptsD = -1;
  
  static cMutex softStartMutex;
  static enum { sstNone = 0, sstNormal, sstNoMetronom } softStartTrigger = sstNone;
  static enum
  {
    sIdle
    , sInitiateSequence
    , sStartSequence
    , sContinueSequence
  }
  softStartState = sIdle;

  double tNow()
  {
    timeval tv;
    ::gettimeofday(&tv, 0);
    return tv.tv_sec + tv.tv_usec / 1.0e+6; 
  }

  static double ttt0 = tNow(), ttt1 = tNow(), ttt2 = 0, ttt3 = 0, ttt4 = 0, ttt5 = 0, ttt6 = 0;

//  static int streams[ 256 ];
  static bool doClear = false;

  bool cXineDevice::SetPlayMode(ePlayMode PlayMode)
  {
    if (pmNone == PlayMode) 
    {
      doClear = true;
      ttt0 = tNow();
    }
    else 
      ttt2 = tNow();
/*
    timeval tv0;
    ::gettimeofday(&tv0, 0);
*/
    if (0)
    {
      time_t t1 = time(0);
      static time_t t0 = t1;

      if (0 == PlayMode && (t1 - t0) > (30 * 60))
        *(char *)0 = 0;
      
      t0 = t1;
    }

    bool playModeSupported = false;

    switch (PlayMode)
    {
    case pmNone:
    case pmAudioVideo:
    case pmAudioOnlyBlack:
    case pmExtern_THIS_SHOULD_BE_AVOIDED:
      playModeSupported = true;
      break;
    case pmAudioOnly:
#if APIVERSNUM >= 10308
    case pmVideoOnly:
#endif
      break;
    }

    ptsV = ptsA = ptsP = ptsD = -1;
    
    ts = 0;
    np = 0;
    f  = 0;
    
    xfprintf(stderr, "SetPlayMode: %d\n", PlayMode);

    if (pmExtern_THIS_SHOULD_BE_AVOIDED == pm
      && pmNone == PlayMode)
    {
      m_xineLib.enableExternal(false);
    }
 
    m_xineLib.pause(false);
    m_xineLib.execFuncTrickSpeedMode(false);
    m_xineLib.execFuncSetSpeed(100.0);

    if (muted)
    {
      muted = false;
      m_xineLib.execFuncMute(false);
    } 

    if (pmNone == PlayMode)
    {
      pm = PlayMode;
    
      jumboPESsize = 0;
      jumboPEStail = 0;
/*
      for (unsigned int i = 0; i < sizeof (streams) / sizeof (*streams); i++)
      {
        if (streams[ i ])
          fprintf(stderr, "stream: 0x%02x\n", i);
      }
*/
      
      m_xineLib.ignore();
//      m_xineLib.execFuncMute();
//      m_xineLib.execFuncSetPrebuffer(0);
      m_xineLib.execFuncClear(-2);
//      m_xineLib.execFuncStart();
//      m_xineLib.execFuncMetronom(0);
      m_xineLib.execFuncStillFrame();
      m_xineLib.execFuncWait();

//      for (int i = 0; i < 2; i++)
        m_xineLib.showNoSignal();

      PushOut();
      m_xineLib.execFuncFlush();

      {
        cMutexLock lock(&softStartMutex);
        softStartTrigger = sstNone;
        softStartState = sIdle;
      }

      foundVideo = false;
      findVideo = false;
    }
    else
    {
      audioSeen = false;
      {
        cMutexLock lock(&softStartMutex);
        softStartTrigger = sstNone;
        softStartState = sIdle;
      }
//      ::memset(&streams, 0, sizeof (streams));
      
      m_xineLib.freeze();
      m_xineLib.ignore(false);

      m_xineLib.freeze(false);
/*
      PushOut();
      m_xineLib.execFuncFlush();
      m_xineLib.execFuncWait();
*/      
//      m_xineLib.execFuncSetPrebuffer(m_settings.GetModeParams()->m_prebufferFrames);
//      m_xineLib.execFuncSetPrebuffer(0);
      m_xineLib.execFuncClear(-4);
//      m_xineLib.execFuncStart();
      m_xineLib.execFuncWait();

#if APIVERSNUM < 10342
      m_settings.SelectReplayPrebufferMode(0 == cTransferControl::ReceiverDevice());
#else
      m_settings.SelectReplayPrebufferMode(!Transferring());
#endif
      
      if (m_settings.LiveTV())
      {
        cMutexLock lock(&softStartMutex);
//        ::fprintf(stderr, "LiveTV\n");
        softStartTrigger = sstNormal;
      }
      else
        np = true;

      foundVideo = false;
      findVideo = true;

      cMutexLock pmMutexLock(&m_pmMutex);
      pm = PlayMode;
      m_pmCondVar.Broadcast();
    }
    
    if (pmExtern_THIS_SHOULD_BE_AVOIDED == PlayMode)
      m_xineLib.enableExternal();
/*    
    timeval tv1;
    ::gettimeofday(&tv1, 0);

    fprintf(stderr, "spm: %.3lf ms\n", 1000 * ((tv1.tv_sec + tv1.tv_usec / 1.0e+6) - (tv0.tv_sec + tv0.tv_usec / 1.0e+6)));
*/
    if (pmNone == PlayMode) {
      ttt1 = tNow(); ttt4 = 0; ttt5 = 0; }
    else
      ttt3 = tNow();
    return playModeSupported;
  }

  void cXineDevice::DiscontinuityDetected()
  {
    if (m_settings.LiveTV())
    {
      cMutexLock lock(&softStartMutex);
      if (softStartState == sIdle
        && softStartTrigger == sstNone
        && pm != pmNone)
      {
        softStartTrigger = sstNormal;
        xfprintf(stderr, "DiscontinuityDetected: triggering soft start\n");
      }
    }
  }

  static bool lastCmdWasClear = false;

  bool cXineDevice::HasIBPTrickSpeed(void)
  {
/*
#if APIVERSNUM >= 10706
    return true;
#else
*/
    return false;
//#endif
  }

  void cXineDevice::TrickSpeed(int Speed)
  {
    TrickSpeed(Speed, false);
  }

  void cXineDevice::TrickSpeed(int Speed, bool IBP)
  {
    f = false;
    ts = Speed;

    xfprintf(stderr, "TrickSpeed: %d\n", Speed);
    m_xineLib.execFuncTrickSpeedMode(lastCmdWasClear);
    m_xineLib.execFuncSetSpeed(100.0 / Speed * (IBP ? 12 : 1));
    m_xineLib.execFuncWait();
    m_xineLib.freeze(false);
    m_xineLib.pause(false);
  }

  void cXineDevice::Clear(void)
  {
    lastCmdWasClear = true;

    doClear = true;
    ptsV = ptsA = ptsP = ptsD = -1;
    
    static int cntClear = 0;

    xfprintf(stderr, "Clear(%d)", cntClear);

    m_xineLib.pause();
    
    jumboPESsize = 0;
    jumboPEStail = 0;

    if (f)
        m_xineLib.execFuncSetSpeed(100.0);

    m_xineLib.execFuncClear(cntClear++);
//    m_xineLib.execFuncStart();
    np = true;
    
    if (f)
        m_xineLib.execFuncSetSpeed(0.0);
    
    m_xineLib.execFuncWait();
    m_xineLib.pause(false);
  xfprintf(stderr, "!\n");
    if (m_settings.LiveTV())
    {
      cMutexLock lock(&softStartMutex);
      softStartTrigger = sstNoMetronom;
    }
   
    cDevice::Clear();
  }

  void cXineDevice::Play(void)
  {
    lastCmdWasClear = false;

    f = false;
    ts = 0;

    xfprintf(stderr, "Play\n");
    m_xineLib.execFuncTrickSpeedMode(false);
    m_xineLib.execFuncSetSpeed(100.0);

    if (muted)
    {
      muted = false;
      m_xineLib.execFuncMute(false);
    }
    
    m_xineLib.execFuncWait();
    m_xineLib.freeze(false);
    m_xineLib.pause(false);
    LOG_ME(::fprintf(stderr, "----\n");)
  }

  void cXineDevice::Freeze(void)
  {
    lastCmdWasClear = false;

    f = true;
    
    xfprintf(stderr, "Freeze\n");  
    m_xineLib.freeze();
    m_xineLib.pause();
    m_xineLib.execFuncSetSpeed(0.0);
    m_xineLib.execFuncWait();
    LOG_ME(::fprintf(stderr, "------\n");)  
  }

  void cXineDevice::Mute(void)
  {
    xfprintf(stderr, "Mute\n");
    m_xineLib.execFuncMute(true);

    muted = true;
  }

  static void store_frame(const unsigned char *buf, int len, int line)
  {
    if (0)
    {
      static int cnt = 0;
      
      char name[ 100 ];
      ::sprintf(name, "/tmp/frame_%05d_%05d", line, cnt++);
      
      FILE *f = fopen(name, "wb");
      size_t r = fwrite(buf, 1, len, f);
      (void)r;
      fclose(f);
    }
  }
  
#define VERBOSE_NOP() do{ xfprintf(stderr, "FIXME: %s:%d\n", __FILE__, __LINE__); } while (0)
#define VERBOSE_NOP1() do{ store_frame(Data, Length, __LINE__); xfprintf(stderr, "FIXME: %s:%d\n", __FILE__, __LINE__); } while (0)
#define VERBOSE_RETURN0(x) do{ xfprintf(stderr, "FIXME: %s:%d\n", __FILE__, __LINE__); return x; } while (0)
#define VERBOSE_RETURN1(x) do{ store_frame(buf0, len0, __LINE__); xfprintf(stderr, "FIXME: %s:%d\n", __FILE__, __LINE__); return x; } while (0)
#define VERBOSE_RETURN2(x) do{ store_frame(buf, len, __LINE__); xfprintf(stderr, "FIXME: %s:%d\n", __FILE__, __LINE__); return x; } while (0)
#define VERBOSE_RETURN3(x) do{ store_frame(Data, Length, __LINE__); xfprintf(stderr, "FIXME: %s:%d\n", __FILE__, __LINE__); return x; } while (0)
  
#if APIVERSNUM < 10331

enum ePesHeader {
  phNeedMoreData = -1,
  phInvalid = 0,
  phMPEG1 = 1,
  phMPEG2 = 2
  };

static ePesHeader AnalyzePesHeader(const uchar *Data, int Count, int &PesPayloadOffset, bool *ContinuationHeader = NULL)
{
  if (Count < 7)
     return phNeedMoreData; // too short

  if ((Data[6] & 0xC0) == 0x80) { // MPEG 2
     if (Count < 9)
        return phNeedMoreData; // too short

     PesPayloadOffset = 6 + 3 + Data[8];
     if (Count < PesPayloadOffset)
        return phNeedMoreData; // too short

     if (ContinuationHeader)
        *ContinuationHeader = ((Data[6] == 0x80) && !Data[7] && !Data[8]);

     return phMPEG2; // MPEG 2
     }

  // check for MPEG 1 ...
  PesPayloadOffset = 6;

  // skip up to 16 stuffing bytes
  for (int i = 0; i < 16; i++) {
      if (Data[PesPayloadOffset] != 0xFF)
         break;

      if (Count <= ++PesPayloadOffset)
         return phNeedMoreData; // too short
      }

  // skip STD_buffer_scale/size
  if ((Data[PesPayloadOffset] & 0xC0) == 0x40) {
     PesPayloadOffset += 2;

     if (Count <= PesPayloadOffset)
        return phNeedMoreData; // too short
     }

  if (ContinuationHeader)
     *ContinuationHeader = false;

  if ((Data[PesPayloadOffset] & 0xF0) == 0x20) {
     // skip PTS only
     PesPayloadOffset += 5;
     }
  else if ((Data[PesPayloadOffset] & 0xF0) == 0x30) {
     // skip PTS and DTS
     PesPayloadOffset += 10;
     }
  else if (Data[PesPayloadOffset] == 0x0F) {
     // continuation header
     PesPayloadOffset++;

     if (ContinuationHeader)
        *ContinuationHeader = true;
     }
  else
     return phInvalid; // unknown

  if (Count < PesPayloadOffset)
     return phNeedMoreData; // too short

  return phMPEG1; // MPEG 1
}

#endif 
 
//#if APIVERSNUM < 10345

  namespace cRemux
  {
// Start codes:
#define SC_SEQUENCE 0xB3  // "sequence header code"
#define SC_GROUP    0xB8  // "group start code"
#define SC_PICTURE  0x00  // "picture start code"

    int GetPacketLength(const uchar *Data, int Count, int Offset)
    {
      // Returns the length of the packet starting at Offset, or -1 if Count is
      // too small to contain the entire packet.
      int Length = (Offset + 5 < Count) ? (Data[Offset + 4] << 8) + Data[Offset + 5] + 6 : -1;
      if (Length > 0 && Offset + Length <= Count)
         return Length;
      return -1;
    }

bool IsFrameH264(const uchar *Data, int Length)
{
  int PesPayloadOffset;
  const uchar *limit = Data + Length;
  if (AnalyzePesHeader(Data, Length, PesPayloadOffset) <= phInvalid)
     return false; // neither MPEG1 nor MPEG2

  Data += PesPayloadOffset + 3; // move to video payload and skip 00 00 01
  if (Data < limit) {
     // cVideoRepacker ensures that in case of H264 we will see an access unit delimiter here
     if (0x01 == Data[-1] && 9 == Data[0] && 0x00 == Data[-2] && 0x00 == Data[-3])
        return true;
     }

  return false;
}

int ScanVideoPacket(const uchar *Data, int Count, int Offset, uchar &PictureType)
{
  // Scans the video packet starting at Offset and returns its length.
  // If the return value is -1 the packet was not completely in the buffer.
  int Length = GetPacketLength(Data, Count, Offset);
  if (Length > 0) {
     const bool h264 = IsFrameH264(Data + Offset, Length);
     int PesPayloadOffset = 0;
     if (AnalyzePesHeader(Data + Offset, Length, PesPayloadOffset) >= phMPEG1) {
        const uchar *p = Data + Offset + PesPayloadOffset + 2;
        const uchar *pLimit = Data + Offset + Length - 3;
#if APIVERSNUM >= 10326
        // cVideoRepacker ensures that a new PES packet is started for a new sequence,
        // group or picture which allows us to easily skip scanning through a huge
        // amount of video data.
        if (p < pLimit) {
           if (p[-2] || p[-1] || p[0] != 0x01)
              pLimit = 0; // skip scanning: packet doesn't start with 0x000001
           else {
              if (h264) {
                 int nal_unit_type = p[1] & 0x1F;
                 switch (nal_unit_type) {
                   case 9: // access unit delimiter
                        // when the MSB in p[1] is set (which violates H.264) then this is a hint
                        // from cVideoRepacker::HandleNalUnit() that this bottom field shall not
                        // be reported as picture.
                        if (p[1] & 0x80)
                           ((uchar *)p)[1] &= ~0x80; // revert the hint and fall through
                        else
                           break;
                   default: // skip scanning: packet doesn't start a new picture
                        pLimit = 0;
                   }
                 }
              else {
                 switch (p[1]) {
                   case SC_SEQUENCE:
                   case SC_GROUP:
                   case SC_PICTURE:
                        break;
                   default: // skip scanning: packet doesn't start a new sequence, group or picture
                        pLimit = 0;
                   }
                 }
              }
           }
#endif
        while (p < pLimit && (p = (const uchar *)memchr(p, 0x01, pLimit - p))) {
              if (!p[-2] && !p[-1]) { // found 0x000001
                 if (h264) {
                    int nal_unit_type = p[1] & 0x1F;
                    switch (nal_unit_type) {
                      case 9: { // access unit delimiter
                              int primary_pic_type = p[2] >> 5;
                              switch (primary_pic_type) {
                                case 0: // I
                                case 3: // SI
                                case 5: // I, SI
                                     PictureType = I_FRAME;
                                     break;
                                case 1: // I, P
                                case 4: // SI, SP
                                case 6: // I, SI, P, SP
                                     PictureType = P_FRAME;
                                     break;
                                case 2: // I, P, B
                                case 7: // I, SI, P, SP, B
                                     PictureType = B_FRAME;
                                     break;
                                }
                              return Length;
                              }
                      }
                    }
                 else {
                    switch (p[1]) {
                      case SC_PICTURE: PictureType = (p[3] >> 3) & 0x07;
                                       return Length;
                      }
                    }
                 p += 4; // continue scanning after 0x01ssxxyy
                 }
              else
                 p += 3; // continue scanning after 0x01xxyy
              }
        }
     PictureType = NO_PICTURE;
     return Length;
     }
  return -1;
}
  }

//#endif

/*
  static bool IsNotVideoIorPframe(const uchar *buf, int len)
  {
    if (0xe0 != (0xf0 & buf[ 3 ]))     // not video
      return true;

    uchar pt = NO_PICTURE;
    cRemux::ScanVideoPacket(buf, len, 0, pt);

    return (I_FRAME == pt || P_FRAME == pt);
  }
*/
/*
  static char *getFrameType(const uchar *buf, int len)
  {
    if (0xe0 != (0xf0 & buf[ 3 ]))     // not video
      return "";

    static char *frameTypes[ 8 ] = 
    {
      "",
      "i",
      "p",
      "b",
      "4",
      "5",
      "6",
      "7"
    };

    uchar pt = NO_PICTURE;
    cRemux::ScanVideoPacket(buf, len, 0, pt);

    return frameTypes[ pt ];
  }
*/
  static bool getPTS(const unsigned char *buf0, int len0, int64_t &pts)
  {
    while (len0 > 0)
    {
      while (len0 > 3
             && 0x00 == buf0[ 0 ]
             && 0x00 == buf0[ 1 ]
             && 0x00 == buf0[ 2 ])
      {
        buf0++;
        len0--;
      }
      
      if (3 == len0
          && 0x00 == buf0[ 0 ]
          && 0x00 == buf0[ 1 ]
          && 0x00 == buf0[ 2 ])
      {
        break;
      }
      
      if (len0 < 6)
        VERBOSE_RETURN1(false);
      
      if (0x00 != buf0[ 0 ]
          || 0x00 != buf0[ 1 ]
          || 0x01 != buf0[ 2 ])
      {
        VERBOSE_RETURN1(false);
      }
      
      if (0xe0 != (0xf0 & buf0[ 3 ])      // video
          && 0xc0 != (0xe0 & buf0[ 3 ])   // audio
          && 0xbd != (0xff & buf0[ 3 ])   // dolby
          && 0xbe != (0xff & buf0[ 3 ]))  // padding (DVD)
      {
        VERBOSE_RETURN1(false);
      }
      
      int len = (6 + buf0[ 4 ] * 0x100 + buf0[ 5 ]);
      if (len > len0)
        VERBOSE_RETURN1(false);
      
      const unsigned char *buf = buf0;
      buf0 += len;
      len0 -= len;

//      if (len0 != 0)
//        VERBOSE_NOP();
      
      if (0xbe == (0xff & buf[ 3 ]))  // padding (DVD)
        continue;
      
      if (len < (6 + 3))
        VERBOSE_RETURN2(false);
      
      if (0x80 != (0xc0 & buf[ 6 ]))  // MPEG1
      {
        do // ... while (false);
        {
          int o = 0;

          for (int i = 0; i < 16; i++)
          {
             if (buf[ 6 + o ] != 0xff)
               break;

             if (len < (6 + ++o))
               VERBOSE_RETURN2(false);
          }

          if (0x40 == (0xc0 & buf[ 6 + o ]))
            o += 2;
        
          if (len < (6 + o))
            VERBOSE_RETURN2(false);
          
          if (0x31 == (0xf1 & buf[ 6 + o + 0 ]))
          {
            if (len < (6 + o + 5 + 5))
              VERBOSE_RETURN2(false);
          
            if (0x01 != (0x01 & buf[ 6 + o + 2 ]))
              VERBOSE_RETURN2(false);
            
            if (0x01 != (0x01 & buf[ 6 + o + 4 ]))
              VERBOSE_RETURN2(false);
            
            if (0x11 != (0xf1 & buf[ 6 + o + 5 + 0 ]))
              VERBOSE_RETURN2(false);
            
            if (0x01 != (0x01 & buf[ 6 + o + 5 + 2 ]))
              VERBOSE_RETURN2(false);
            
            if (0x01 != (0x01 & buf[ 6 + o + 5 + 4 ]))
              VERBOSE_RETURN2(false);
            
            int64_t _pts = ((int64_t)(0x0e & buf[ 6 + o + 0 ])) << 29
              |                      (0xff & buf[ 6 + o + 1 ])  << 22
              |                      (0xfe & buf[ 6 + o + 2 ])  << 14
              |                      (0xff & buf[ 6 + o + 3 ])  << 7
              |                      (0xfe & buf[ 6 + o + 4 ])  >> 1;
            
//            ::fprintf(stderr, "pts: %lld\n", _pts);
            
            if (0 == _pts)
              break;
            
//            if (!IsNotVideoIorPframe(buf, len)) // only PTS of I and P frames are progressive in time
//              break;
            
            pts = _pts;
            
            return true;
          }
          else if (0x21 == (0xf1 & buf[ 6 + o + 0 ]))
          {
            if (len < (6 + o + 5))
              VERBOSE_RETURN2(false);
          
            if (0x01 != (0x01 & buf[ 6 + o + 2 ]))
              VERBOSE_RETURN2(false);
            
            if (0x01 != (0x01 & buf[ 6 + o + 4 ]))
              VERBOSE_RETURN2(false);
            
            int64_t _pts = ((int64_t)(0x0e & buf[ 6 + o + 0 ])) << 29
              |                      (0xff & buf[ 6 + o + 1 ])  << 22
              |                      (0xfe & buf[ 6 + o + 2 ])  << 14
              |                      (0xff & buf[ 6 + o + 3 ])  << 7
              |                      (0xfe & buf[ 6 + o + 4 ])  >> 1;
            
//            ::fprintf(stderr, "pts: %lld\n", _pts);
            
            if (0 == _pts)
              break;

//            if (!IsNotVideoIorPframe(buf, len)) // only PTS of I and P frames are progressive in time
//              break;
            
            pts = _pts;
            
            return true;
          }
          else if (0x0f == (0xff & buf[ 6 + o + 0 ]))
          {
            break;
          }
          
          for (int i = 0; i < 30; i++)
            xfprintf(stderr, "%02x ", buf[ i ]);
          xfprintf(stderr, "\n");
          
          VERBOSE_RETURN2(false);
        }
        while (false);

        continue;
      }
      
      if (0x40 == (0xc0 & buf[ 7 ]))
        VERBOSE_RETURN2(false);
      
      if (0x00 == (0xc0 & buf[ 7 ]))
        continue;

// ignore      
//      if (0x00 != (0x3f & buf[ 7 ]))
//        VERBOSE_RETURN2(false);
      
      bool hasPTS = (0 != (0x80 & buf[ 7 ]));
      bool hasDTS = (0 != (0x40 & buf[ 7 ]));
      
      unsigned char hdl = buf[ 8 ];
      
      if (hdl < ((hasPTS + hasDTS) * 5))
        VERBOSE_RETURN2(false);
      
      if (len < (6 + 3 + hdl))
        VERBOSE_RETURN2(false);
      
      if ((0x20 * hasPTS + 0x10 * hasDTS + 0x01) != (0xf1 & buf[ 9 ]))
      {
        if ((0x20 * hasPTS + 0x00 * hasDTS + 0x01) != (0xf1 & buf[ 9 ]))
        {
          // accept streams, that start with '00X0' instead of '00X1'.
        }
        else if ((0x00 * hasPTS + 0x10 * hasDTS + 0x01) != (0xf1 & buf[ 9 ]))
        {
          // accept streams, that start with '000X' instead of '001X'.
        }
        else
        {
fprintf(stderr, "buf:"); for (int i = 0; i < 6 + 3 + hdl; i++) fprintf(stderr, " %02x", buf[i]); fprintf(stderr, "\n");
          VERBOSE_RETURN2(false);
        }
      }
      
      if (0x01 != (0x01 & buf[ 11 ]))
        VERBOSE_RETURN2(false);
      
      if (0x01 != (0x01 & buf[ 13 ]))
        VERBOSE_RETURN2(false);
      
      if (hasDTS)
      {
        if (0x11 != (0xf1 & buf[ 14 ]))
        {
          if (0x21 == (0xf1 & buf[ 14 ]))
          {
            // accept streams, that start with '0010' instead of '0001'.
          }
          else if (0xa1 == (0xf1 & buf[ 14 ]))
          {
            // accept streams, that start with '1010' instead of '0001'.
          }
          else
          {
fprintf(stderr, "buf:"); for (int i = 0; i < 6 + 3 + hdl; i++) fprintf(stderr, " %02x", buf[i]); fprintf(stderr, "\n");
            VERBOSE_RETURN2(false);
          }
        }

// accept streams that have no marker bits set                  
//        if (0x01 != (0x01 & buf[ 16 ]))
//          VERBOSE_RETURN2(false);
              
//        if (0x01 != (0x01 & buf[ 18 ]))
//          VERBOSE_RETURN2(false);
      }
/*
      fprintf(stderr, " %02x %02x %02x %02x %02x\n"
              , buf[  9 ]
              , buf[ 10 ]
              , buf[ 11 ]
              , buf[ 12 ]
              , buf[ 13 ]);
*/
      int64_t _pts = ((int64_t)(0x0e & buf[  9 ])) << 29
        |                      (0xff & buf[ 10 ])  << 22
        |                      (0xfe & buf[ 11 ])  << 14
        |                      (0xff & buf[ 12 ])  << 7
        |                      (0xfe & buf[ 13 ])  >> 1;

      if (0 == _pts)
        return false;

//      if (!IsNotVideoIorPframe(buf, len)) // only PTS of I and P frames are progressive in time
//        return false;
            
      pts = _pts;
      
      return true;
    }

//    VERBOSE_RETURN2(false);
    return false;
  }
/*
  static bool stripPTSandDTS(unsigned char *buf0, int len0)
  {
    while (len0 > 0)
    {
      while (len0 > 3
             && 0x00 == buf0[ 0 ]
             && 0x00 == buf0[ 1 ]
             && 0x00 == buf0[ 2 ])
      {
        buf0++;
        len0--;
      }
      
      if (3 == len0
          && 0x00 == buf0[ 0 ]
          && 0x00 == buf0[ 1 ]
          && 0x00 == buf0[ 2 ])
      {
        break;
      }
      
      if (len0 < 6)
        VERBOSE_RETURN1(false);
      
      if (0x00 != buf0[ 0 ]
          || 0x00 != buf0[ 1 ]
          || 0x01 != buf0[ 2 ])
      {
        VERBOSE_RETURN1(false);
      }
      
      if (0xe0 != (0xf0 & buf0[ 3 ])      // video
          && 0xc0 != (0xe0 & buf0[ 3 ])   // audio
          && 0xbd != (0xff & buf0[ 3 ])   // dolby
          && 0xbe != (0xff & buf0[ 3 ])   // padding (DVD)
          && 0xba != (0xff & buf0[ 3 ])   // system layer: pack header
          && 0xbb != (0xff & buf0[ 3 ])   // system layer: system header
          && 0xb9 != (0xff & buf0[ 3 ]))  // system layer: stream end
      {
//fprintf(stderr, "buf0[ 3 ]: %02x\n", buf0[ 3 ]);
        VERBOSE_RETURN1(false);
      }

      int len = (6 + buf0[ 4 ] * 0x100 + buf0[ 5 ]);
      if (0xba == buf0[ 3 ]) // pack header has fixed length
      {
        if (0x00 == (0xc0 & buf0[ 4 ])) // MPEG 1
          len = 12;
        else // MPEG 2
          len = 14 + (buf0[ 13 ] & 0x07);
      }
      else if (0xb9 == buf0[ 3 ]) // stream end has fixed length
      {
        len = 4;
      }

      if (len > len0)
        VERBOSE_RETURN1(false);
      
      unsigned char *buf = buf0;
      buf0 += len;
      len0 -= len;

//      if (len0 != 0)
//        VERBOSE_NOP();
      
      if (0xbe == (0xff & buf[ 3 ])    // padding (DVD)
        || 0xba == (0xff & buf[ 3 ])   // system layer: pack header
        || 0xbb == (0xff & buf[ 3 ])   // system layer: system header
        || 0xb9 == (0xff & buf[ 3 ]))  // system layer: stream end
      {
        continue;
      }
      
      if (len < (6 + 3))
        VERBOSE_RETURN2(false);
      
      if (0x80 != (0xc0 & buf[ 6 ]))  // MPEG1
      {
        bool ok = false;
        
        do // ... while (false);
        {
          int o = 0;

          for (int i = 0; i < 16; i++)
          {
             if (buf[ 6 + o ] != 0xff)
               break;

             if (len < (6 + ++o))
               VERBOSE_RETURN2(false);
          }

          if (0x40 == (0xc0 & buf[ 6 + o ]))
            o += 2;
        
          if (len < (6 + o))
            VERBOSE_RETURN2(false);
          
          if (0x31 == (0xf1 & buf[ 6 + o + 0 ]))
          {
            if (0x01 != (0x01 & buf[ 6 + o + 2 ]))
              VERBOSE_RETURN2(false);
            
            if (0x01 != (0x01 & buf[ 6 + o + 4 ]))
              VERBOSE_RETURN2(false);
            
            if (0x11 != (0xf1 & buf[ 6 + o + 5 + 0 ]))
              VERBOSE_RETURN2(false);
            
            if (0x01 != (0x01 & buf[ 6 + o + 5 + 2 ]))
              VERBOSE_RETURN2(false);
            
            if (0x01 != (0x01 & buf[ 6 + o + 5 + 4 ]))
              VERBOSE_RETURN2(false);
            
            buf[ 6 + o + 0 ] = 0xff;
            buf[ 6 + o + 1 ] = 0xff;
            buf[ 6 + o + 2 ] = 0xff;
            buf[ 6 + o + 3 ] = 0xff;
            buf[ 6 + o + 4 ] = 0xff;
            
            buf[ 6 + o + 5 + 0 ] = 0xff;
            buf[ 6 + o + 5 + 1 ] = 0xff;
            buf[ 6 + o + 5 + 2 ] = 0xff;
            buf[ 6 + o + 5 + 3 ] = 0xff;
            buf[ 6 + o + 5 + 4 ] = 0x0f;
            
            ok = true;
          }
          else if (0x21 == (0xf1 & buf[ 6 + o + 0 ]))
          {
            if (0x01 != (0x01 & buf[ 6 + o + 2 ]))
              VERBOSE_RETURN2(false);
            
            if (0x01 != (0x01 & buf[ 6 + o + 4 ]))
              VERBOSE_RETURN2(false);
            
            buf[ 6 + o + 0 ] = 0xff;
            buf[ 6 + o + 1 ] = 0xff;
            buf[ 6 + o + 2 ] = 0xff;
            buf[ 6 + o + 3 ] = 0xff;
            buf[ 6 + o + 4 ] = 0x0f;
            
            ok = true;
          }
          else if (0x0f == (0xff & buf[ 6 + o + 0 ]))
          {
            ok = true;
          }

          if (ok)
            break;
            
          for (int i = 0; i < 30; i++)
            xfprintf(stderr, "%02x ", buf[ i ]);
          xfprintf(stderr, "\n");
          
          VERBOSE_RETURN2(false);
        }
        while (false);

        if (ok)
          continue;
      }
      
      if (0x40 == (0xc0 & buf[ 7 ]))
        VERBOSE_RETURN2(false);
      
      if (0x00 == (0xc0 & buf[ 7 ]))
        continue;

// ignore      
//      if (0x00 != (0x3f & buf[ 7 ]))
//        VERBOSE_RETURN2(false);
      
      bool hasPTS = (0 != (0x80 & buf[ 7 ]));
      bool hasDTS = (0 != (0x40 & buf[ 7 ]));
      
      unsigned char hdl = buf[ 8 ];
      
      if (hdl < ((hasPTS + hasDTS) * 5))
        VERBOSE_RETURN2(false);
      
      if (len < (6 + 3 + hdl))
        VERBOSE_RETURN2(false);
      
      if ((0x20 * hasPTS + 0x10 * hasDTS + 0x01) != (0xf1 & buf[ 9 ]))
      {
        if ((0x20 * hasPTS + 0x00 * hasDTS + 0x01) != (0xf1 & buf[ 9 ]))
        {
          // accept streams, that start with '00X0' instead of '00X1'.
        }
        else if ((0x00 * hasPTS + 0x10 * hasDTS + 0x01) != (0xf1 & buf[ 9 ]))
        {
          // accept streams, that start with '000X' instead of '001X'.
        }
        else
        {
          VERBOSE_RETURN2(false);
        }
      }
      
      if (0x01 != (0x01 & buf[ 11 ]))
        VERBOSE_RETURN2(false);
      
      if (0x01 != (0x01 & buf[ 13 ]))
        VERBOSE_RETURN2(false);
      
      if (hasDTS)
      {
        if (0x11 != (0xf1 & buf[ 14 ]))
          VERBOSE_RETURN2(false);
        
        if (0x01 != (0x01 & buf[ 16 ]))
          VERBOSE_RETURN2(false);
                  
        if (0x01 != (0x01 & buf[ 18 ]))
          VERBOSE_RETURN2(false);
      }
      
      buf[ 7 ] &= 0x3f;
      
      for (int i = 9; i < (9 + ((hasPTS + hasDTS) * 5)); i++)
        buf[ i ] = 0xff;
    }
    
    return true;
  }
*/
  static uchar padding[ 6 + 0xffff ] =
  {
    0x00
    , 0x00
    , 0x01
    , 0xbe
    , 0xff
    , 0xff
  };

  int cXineDevice::PushOut()
  {
    uchar *Data = padding;
    int Length = sizeof (padding);
    
    return PlayCommon3(Data, Length, -1);
  }
  
//static bool blahblah = false;

  void cXineDevice::StillPicture(const uchar *Data, int Length)
  {
#if APIVERSNUM >= 10701
    if (Length > 0 && Data[0] == 0x47)
    {
const uchar *tsData = Data;
const int tsLength = Length;
uchar *pesData = (uchar *)malloc(Length);
int pesLength = 0;
uchar tsHead[4] = { 0xff, 0xff, 0xff, 0xff };
VDR172::cRemux *vRemux = 0;
cPatPmtParser patPmtParser;
while (Length >= TS_SIZE)
{
  if (TsHasPayload(Data))
  {
    int plo = TsPayloadOffset(Data);
    if (plo < Length)
    {
      int pid = TsPid(Data);
      if (pid == 0)
        patPmtParser.ParsePat(Data, TS_SIZE);
      else if (pid == patPmtParser.PmtPid())
        patPmtParser.ParsePmt(Data, TS_SIZE);
      else if (pid == patPmtParser.Vpid())
      {
        if (!vRemux)
        {
          vRemux = new VDR172::cRemux(patPmtParser.Vpid(), 0, 0, 0);
          vRemux->SetTimeouts(0, 0);
        }

        memcpy(tsHead, Data, sizeof (tsHead));
        vRemux->Put(Data, TS_SIZE);

        int n = 0;
        uchar *p = vRemux->Get(n);
        if (p)
        {
          memcpy(pesData + pesLength, p, n);
          pesLength += n;
          vRemux->Del(n);
        }
      }
    }
  }

Data += TS_SIZE;
Length -= TS_SIZE;
}

if (!vRemux)
{
  free(pesData);
  return;
}

uchar tsTail[TS_SIZE];
memset(tsTail + sizeof (tsHead), 0xff, sizeof (tsTail) - sizeof (tsHead));
memcpy(tsTail, tsHead, sizeof (tsHead)); 
tsTail[1] &= ~TS_ERROR;
tsTail[1] |= TS_PAYLOAD_START;
tsTail[3] &= ~TS_SCRAMBLING_CONTROL;
tsTail[3] |= TS_ADAPT_FIELD_EXISTS;
tsTail[3] |= TS_PAYLOAD_EXISTS;
tsTail[3] = (tsTail[3] & 0xf0) | (((tsTail[3] & TS_CONT_CNT_MASK) + 1) & TS_CONT_CNT_MASK);
tsTail[4] = TS_SIZE - 5 - 13;
tsTail[5] = 0x00;
tsTail[TS_SIZE - 13] = 0x00;
tsTail[TS_SIZE - 12] = 0x00;
tsTail[TS_SIZE - 11] = 0x01;
tsTail[TS_SIZE - 10] = pesData[3];
tsTail[TS_SIZE - 9] = 0x00;
tsTail[TS_SIZE - 8] = 0x07;
tsTail[TS_SIZE - 7] = 0x80;
tsTail[TS_SIZE - 6] = 0x00;
tsTail[TS_SIZE - 5] = 0x00;
tsTail[TS_SIZE - 4] = 0x00;
tsTail[TS_SIZE - 3] = 0x00;
tsTail[TS_SIZE - 2] = 0x01;
tsTail[TS_SIZE - 1] = VDR172::cRemux::IsFrameH264(pesData, pesLength) ? 10 : 0xb7;

if (0)
{
FILE *ff = fopen("/tmp/still.ts", "wb");
fwrite(tsData, 1, tsLength, ff);
fwrite(tsTail, 1, TS_SIZE, ff);
fclose(ff);
}

        vRemux->Put(tsTail, TS_SIZE);

        int n = 0;
        uchar *p = vRemux->Get(n);
        if (p)
        {
          memcpy(pesData + pesLength, p, n);
          pesLength += n;
          vRemux->Del(n);
        }

if (0)
{
FILE *ff = fopen("/tmp/still.pes", "wb");
fwrite(pesData, 1, pesLength, ff);
fclose(ff);
}

delete vRemux;

StillPicture(pesData, pesLength);
free(pesData);
      return;
    }
#endif
    
    xfprintf(stderr, "StillPicture: %p, %d\n", Data, Length);

    if (0)
    {
      for (int i = 0; i < Length - 3; i++)
      {
        if (i != 0
            && Data[ i + 0 ] == 0x00
            && Data[ i + 1 ] == 0x00
            && Data[ i + 2 ] == 0x01)
        {
          xfprintf(stderr, "\n");
        }
        
        xfprintf(stderr, "%02x ", Data[ i ]);
      }
      
      for (int i = Length - 3; i < Length; i++)
      {
        xfprintf(stderr, "%02x ", Data[ i ]);
      }
      
      xfprintf(stderr, "\n");
    }
    
    const int maxPackets = 3;
    uchar pes[ maxPackets * (6 + 0xffff) ];
    static const uchar header[ 6 + 3 ] = { 0x00, 0x00, 0x01, 0xe0, 0x00, 0x00, 0x80, 0x00, 0x00 };
    
    do // ... while (false);
    {
      if (Length < 6)
      {
        VERBOSE_NOP();
        break;
      }
      
      if (0x00 != Data[ 0 ]
          || 0x00 != Data[ 1 ]
          || 0x01 != Data[ 2 ])
      {
        VERBOSE_NOP();
        break;
      }
      
      if (0xe0 != (0xf0 & Data[ 3 ])      // video
          && 0xc0 != (0xe0 & Data[ 3 ])   // audio
          && 0xbd != (0xff & Data[ 3 ])   // dolby
          && 0xbe != (0xff & Data[ 3 ]))  // padding (DVD)
      {
        if (Length > maxPackets * (0xffff - 3))
        {
          VERBOSE_NOP();
          break;
        }
      
	int todo = Length;
        const uchar *src = Data;
        uchar *dst = pes;

        while (todo > 0)
        {
          ::memcpy(dst, header, sizeof (header));
          
          int bite = todo;

          if (bite > 0xffff - 3)
            bite = 0xffff - 3;

          todo -= bite;
          
          dst[ 4 ] = 0xff & ((bite + 3) >> 8);
          dst[ 5 ] = 0xff & (bite + 3);

          ::memcpy(dst + sizeof (header), src, bite);

          Length += sizeof (header);
          dst += sizeof (header) + bite;
          src += bite;
        }
        
        Data = pes;
      }
    }
    while (false);
    
//NNN    stripPTSandDTS((uchar *)Data, Length);
    
    ts = 0;

    m_xineLib.execFuncTrickSpeedMode(false);
    m_xineLib.execFuncSetSpeed(100.0);
    m_xineLib.execFuncStillFrame();
    m_xineLib.execFuncWait();

    f = 0;
    
    m_xineLib.pause(false);

//    PushOut();

//blahblah = true;
#if APIVERSNUM >= 10704
uchar pesTail[] = { 0x00, 0x00, 0x01, Data[3], 0x00, 3 + 4,
0x80, 0x00, 0x00,
0x00, 0x00, 0x01, VDR172::cRemux::IsFrameH264(Data, Length) ? 10 : 0xb7
};
#endif

    for (int i = 0; i < 1 /* 4 */; i++)
    {
//fprintf(stderr, " (%d) ", i);
      int r = PlayVideo1(Data, Length, true);      
      if (r < 0)
        return;
#if APIVERSNUM >= 10704
      r = PlayVideo1(pesTail, sizeof (pesTail), true);      
      if (r < 0)
        return;
#endif
    }
/*
FILE *ff = fopen("/tmp/still.pes", "wb");
fwrite(Data, 1, Length, ff);
fclose(ff);
*/
    PushOut();
//    m_xineLib.execFuncFlush(0);
    m_xineLib.execFuncFlush();

//    ::fprintf(stderr, "------------\n");      
    LOG_ME(::fprintf(stderr, "------------\n");)
  }

  static bool softStartPoll(cXineLib &xineLib, cPoller &poller, const int timeout, const bool result);

  bool cXineDevice::Poll(cPoller &Poller, int TimeoutMs /* = 0 */)
  {
    cMutexLock lock(&softStartMutex);

    if (softStartState != sIdle)
      return true;

    if (m_xineLib.Poll(Poller, TimeoutMs))
      return softStartPoll(m_xineLib, Poller, TimeoutMs, true);

    return softStartPoll(m_xineLib, Poller, TimeoutMs, false);
  }

  static bool jw = false;

  bool cXineDevice::Flush(int TimeoutMs /* = 0 */)
  {
    const bool jw0 = jw;
    
    m_xineLib.pause(false);

    if (!jw0)
    {
      int r = PushOut();
      if (r < 0)
        return true;
    }

    bool retVal = m_xineLib.execFuncFlush(TimeoutMs, jw0);

    if (!retVal)
      xfprintf(stderr, jw0 ? "f" : "F");
    
    jw = true;
    
    return retVal;
  }
  
  static bool dumpAudio(const char *proc, const uchar *Data, int Length)
  {
    return false;
    
  nextPacket:
    if (Length == 0)
      return true;
/*    
    fprintf(stderr
            , "%s: "
            , proc);
*/  
    if (Length < 6)
      VERBOSE_RETURN0(false);
        
    if (0x00 != Data[ 0 ]
        || 0x00 != Data[ 1 ]
        || 0x01 != Data[ 2 ])
    {
      VERBOSE_RETURN3(false);
    }

    int l = Data[ 4 ] * 0x0100 + Data[ 5 ];
    if (Length < (6 + l))
      VERBOSE_RETURN0(false);
    
    if (0xe0 == (Data[ 3 ] & 0xf0)     //video
        || 0xc0 == (Data[ 3 ] & 0xe0)  //audio
        || 0xbe == Data[ 3 ])          //padding
    {
      Data += (6 + l);
      Length -= (6 + l);
      goto nextPacket;
    }
    
  //  if (0xbd != Data[ 3 ])             //private (dolby, pcm)
  //    VERBOSE_RETURN0(false);

//    fprintf(stderr, "private ");
    
    if (l < (3 + 0 + 2))
      VERBOSE_RETURN0(false);
    
    int h = Data[ 8 ];
    if (l < (3 + h + 2))
      VERBOSE_RETURN0(false);

    xfprintf(stderr
            , "%s: "
            , proc);

    xfprintf(stderr
            , "0x%02x 0x%02x\n"
            , Data[ 6 + 3 + h + 0 ]
            , Data[ 6 + 3 + h + 1 ]);

    Data += (6 + l);
    Length -= (6 + l);
    goto nextPacket;
  }
  
  static bool IsVideo(const uchar *Data, int Length)
  {
    return (Length >= 4
            && 0x00 == Data[ 0 ]
            && 0x00 == Data[ 1 ]
            && 0x01 == Data[ 2 ]
            && 0xe0 == (0xf0 & Data[ 3 ]));
  }

#if APIVERSNUM >= 10701
  static int m_tsVideoPid = 0;
#endif

  int cXineDevice::PlayTsVideo(const uchar *Data, int Length)
  {
#if APIVERSNUM >= 10701
    if (Length >= TS_SIZE)
    {
#if APIVERSNUM >= 10712
      m_tsVideoPid = PatPmtParser()->Vpid();
#else
      m_tsVideoPid = TsPid(Data);
#endif
    }
#endif
    return Length;
  }

  int cXineDevice::PlayTsAudio(const uchar *Data, int Length)
  {
    return Length;
  }

#if APIVERSNUM >= 10701

static bool m_trampoline = false;

struct sTrampoline
{
  uchar Header[6];
  const uchar *Data;
  int Length;
  bool VideoOnly;
};
  static VDR172::cRemux *m_tsVideoRemux = 0;
  static VDR172::cRemux *m_tsAudioRemux = 0;
#endif
  int cXineDevice::PlayTs(const uchar *Data, int Length, bool VideoOnly /* = false */)
  {
#if APIVERSNUM >= 10701

    cMutexLock lock(&m_playTsMutex);

    int ret = PlayTsImpl(Data, Length, VideoOnly);

//    if (Length > 0 && ret <= 0) { char cmd[500]; sprintf(cmd, "ddd /soft/vdr-" APIVERSION "/bin/vdr %d", getpid()); system(cmd); sleep(10); } //should never happen!

    return ret;

#endif
    return -1;
  }

  int cXineDevice::PlayTsImpl(const uchar *Data, int Length, bool VideoOnly /* = false */)
  {
    if (!Data)
      return PlaySingleTs(Data, Length, VideoOnly);

    int ret = 0;
    while (Length >= TS_SIZE)
    {
      int r = PlaySingleTs(Data, TS_SIZE, VideoOnly);
      if (r < 0)
      {
        if (!ret)
          return r;

        return ret;
      }

      ret += r;
      Length -= r;
      Data += r;
    } 

    if (ret)
      return ret;

    if (Length > 0)
      return PlaySingleTs(Data, Length, VideoOnly);

    return ret;
  }
 
  int cXineDevice::PlaySingleTs(const uchar *Data, int Length, bool VideoOnly /* = false */)
  {
#if APIVERSNUM >= 10701

    if (!Data)
    {
      m_tsVideoPid = 0;
      delete m_tsVideoRemux;
      delete m_tsAudioRemux;
      m_tsVideoRemux = 0;
      m_tsAudioRemux = 0;
    }

    int ret = cDevice::PlayTs(Data, Length, VideoOnly);
    if (ret <= 0)
      return ret;

    if (!TsHasPayload(Data))
      return ret; // silently ignore TS packets w/o payload
if (TsIsScrambled(Data)) fprintf(stderr, "TS is scrambled **************\n");
    int payloadOffset = TsPayloadOffset(Data);
    if (payloadOffset >= Length)
      return ret;

    int pid = TsPid(Data);
    if (pid == 0)
    {
      m_tsVideoPid = 0;
      return ret;
    }

    m_trampoline = true;
    sTrampoline trampoline = { { 0x00, 0x00, 0x01, 0xe0, 0x00, sizeof (trampoline) - 6 }, Data, Length, VideoOnly };
    PlayPesPacket(trampoline.Header, sizeof (trampoline));

    return ret;

#endif
    return -1;
  }
 
//static int aaz = 0;
  void cXineDevice::PlayTsTrampoline(const uchar *Data, int Length, bool VideoOnly /* = false */)
  {
#if APIVERSNUM >= 10701
m_trampoline = false;
    const tTrackId *track = 0;
    int pid = TsPid(Data);
    if (pid == m_tsVideoPid)
      goto remux;
    else if ((track = GetTrack(GetCurrentAudioTrack())) && pid == track->id)
    {
      if (!VideoOnly || HasIBPTrickSpeed())
        goto remux;
    }
    return;

remux:
    static int vpid = 0;
    static int apid[2] = { 0, 0 };
    static int dpid[2] = { 0, 0 };

    int v = 0;
    int a = 0;
    int d = 0;

    v = m_tsVideoPid;
    if (!m_tsVideoRemux
      || vpid != v)
    {
//fprintf(stderr, "vpid: %d, v: %d\n", vpid, v);

      delete m_tsVideoRemux;

      vpid = v;

      m_tsVideoRemux = new VDR172::cRemux(vpid, 0, 0, 0);
      m_tsVideoRemux->SetTimeouts(0, 0);
    }

    if (ttAudioFirst <= GetCurrentAudioTrack() && GetCurrentAudioTrack() <= ttAudioLast)
      a = (track = GetTrack(GetCurrentAudioTrack())) ? track->id : 0;
    else
      d = (track = GetTrack(GetCurrentAudioTrack())) ? track->id : 0;
/*
if (a || d)
{
if (aaz)
if (aaz != (d ? 1 : 2)) *(char *)0 =0 ;
}
*/
    if (!m_tsAudioRemux
      || *apid != a
      || *dpid != d)
    {
//fprintf(stderr, "a: %d, d: %d\n", a, d);

      delete m_tsAudioRemux;

      *apid = a;
      *dpid = d;

      m_tsAudioRemux = new VDR172::cRemux(0, apid, dpid, 0);
      m_tsAudioRemux->SetTimeouts(0, 0);
    }

    if (pid == vpid)
    {
      m_tsVideoRemux->Put(Data, Length);

      int n = 0;
      uchar pt;
      uchar *p = m_tsVideoRemux->Get(n, &pt);
      if (p)
      {
        if (m_xineLib.isTrickSpeedMode())
        {
          if (pt == I_FRAME && VDR172::cRemux::IsFrameH264(p, n))
          {
fprintf(stderr, "TrickSpeedMode: push H.264 SequenceEndCode\n");
            uchar sequenceEndCode[] = { 0x00, 0x00, 0x01, p[3], 0x00, 0x07, 0x80, 0x00, 0x00, 0x00, 0x00, 0x01, 10 };
            PlayVideo(sequenceEndCode, sizeof (sequenceEndCode));
          }
        }
        int w = PlayVideo(p, n);
        m_tsVideoRemux->Del(w);
      }
    }
    else
    {
      m_tsAudioRemux->Put(Data, Length);

      int n = 0;
      uchar *p = m_tsAudioRemux->Get(n);
      if (p)
      {
        int w = PlayAudio(p, n, 0);
        m_tsAudioRemux->Del(w);
      }
    }
#endif
  }

  int cXineDevice::PlayVideo(const uchar *Data, int Length)
  {
#if APIVERSNUM >= 10701
if (m_trampoline)
{
  sTrampoline *t = (sTrampoline *)Data;
  PlayTsTrampoline(t->Data, t->Length, t->VideoOnly);
  return Length;
}
#endif
    if (ttt4 == 0) ttt4 = tNow();
//    static FILE *f = fopen("/tmp/egon1", "wb");
//    fwrite(Data, Length, 1, f);

    return PlayVideo1(Data, Length, false);
  }
  
  int cXineDevice::PlayVideo1(const uchar *Data, int Length, const bool stillImageData)
  {
    LOG_ME(::fprintf(stderr, "V");)

    if (f)
    {      
      LOG_ME(::fprintf(stderr, "<");)
      return 0;
    }

    if (pmNone == pm)
    {
      cMutexLock pmMutexLock(&m_pmMutex);

      if (pmNone == pm)
        m_pmCondVar.Wait(m_pmMutex);
    }
    
    int retVal = PlayVideo2(Data, Length, stillImageData);

    LOG_ME(::fprintf(stderr, "v");)

    return retVal;
  }

  int cXineDevice::PlayCommon2(const uchar *Data, int Length, int64_t ptsForce)
  {
/*
if (blahblah)
{
  fprintf(stderr, "blahblah C2");
  for (int i = 0; i < 50 && i < Length; i++)
    fprintf(stderr, " %02x", Data[ i ]);
  fprintf(stderr, "\n");
}
*/
    do // ... while (false);
    {
      if (Length < 6)
      {
        VERBOSE_NOP();
        break;
      }
      
      if (0x00 != Data[ 0 ]
          || 0x00 != Data[ 1 ]
          || 0x01 != Data[ 2 ])
      {
        VERBOSE_NOP();
        break;
      }
      
      int l = 6 + Data[ 4 ] * 0x0100 + Data[ 5 ];
      if (Length < l)
      {
        VERBOSE_NOP();
        break;
      }
      
      if (0xe0 != (Data[ 3 ] & 0xf0)     //video
          && 0xc0 != (Data[ 3 ] & 0xe0)  //audio
          && 0xbd != Data[ 3 ])          //private (dolby, pcm)
      {
        VERBOSE_NOP();
        break;
      }

      int payloadOffset = 0;
      if (AnalyzePesHeader(Data, Length, payloadOffset) <= phInvalid)
      {
        VERBOSE_NOP();
        break;
      }

      if (l <= payloadOffset)
      {
        // drop short frames
//        ::fprintf(stderr, "i");
        return Length;
      }

//      if (0xc0 == (Data[ 3 ] & 0xe0))  //audio
//        return Length;
    }
    while (false);

    return PlayCommon3(Data, Length, ptsForce);
  }

typedef long long int lld_t;

static int xzabc = 0;

  int cXineDevice::PlayCommon3(const uchar *Data, int Length, int64_t ptsForce)
  {
//    if (!m_settings.LiveTV())
//    {
//      VERBOSE_NOP1();
//      return Length;
//    }

//    if (xzabc)
    if (0)
    {
      if (0xe0 == (Data[ 3 ] & 0xf0))      //video
      {
        uchar pt = NO_PICTURE;
        cRemux::ScanVideoPacket(Data, Length, 0, pt);

        if (pt != NO_PICTURE)
        {
          static int64_t last = -1;
          int64_t pts = -1;
          getPTS(Data, Length, pts);
          fprintf(stderr, "** %c ** %lld ** %lld ** %lld **\n", "0IPB4567"[pt], (lld_t)ptsForce, (lld_t)(ptsForce - last), (lld_t)pts);
          last = ptsForce;
        }
      }
      if (0xc0 == (Data[ 3 ] & 0xe0))      //audio
      {
        {
          static int64_t last = -1;
          int64_t pts = -1;
          getPTS(Data, Length, pts);
          fprintf(stderr, "\t\t\t\t\t\t\t** %c ** %lld ** %lld ** %lld **\n", 'A', (lld_t)ptsForce, (lld_t)(ptsForce - last), (lld_t)pts);
          last = ptsForce;
        }
      }
    }
    if (0)
    {
      int64_t pts = ptsForce;
      if (ptsForce > -1
        || getPTS(Data, Length, pts))
      {
        xzabc = 0;
        int64_t *pPTS = 0;
        
        if (0xe0 == (Data[ 3 ] & 0xf0))      //video
        {
          pPTS = &ptsV;
        }
        else if (0xc0 == (Data[ 3 ] & 0xe0)) //audio
        {
          pPTS = &ptsA;
        }
        else if (0xbd == Data[ 3 ])          //private (dolby, pcm)
        {
          int h = Data[ 6 + 2 ];
          
          if (0xa0 == (0xf0 & Data[ 6 + 3 + h + 0 ]))  // pcm?
            pPTS = &ptsP;
          else
            pPTS = &ptsD;
        }
        else
        {
          xfprintf(stderr, "0x%02x\t", Data[ 3 ]);
          VERBOSE_NOP();        
        }
        
        if (pPTS
            && *pPTS != pts)
        {
          *pPTS = pts;
          
          int64_t ptsX = -1;
          m_xineLib.execFuncGetPTS(ptsX);

          int64_t dV = (ptsV != -1 && ptsX != -1) ? ptsV - ptsX : 0;
          int64_t dA = (ptsA != -1 && ptsX != -1) ? ptsA - ptsX : 0;
          int64_t dP = (ptsP != -1 && ptsX != -1) ? ptsP - ptsX : 0;
          int64_t dD = (ptsD != -1 && ptsX != -1) ? ptsD - ptsX : 0;

          int64_t dVA = (ptsV != -1 && ptsA != -1) ? ptsA - ptsV : 0; 
          int64_t dVD = (ptsV != -1 && ptsD != -1) ? ptsD - ptsV : 0; 

          fprintf(stderr, "ptsVideo: %lld, ptsAudio: %lld, ptsPCM: %lld, ptsDolby: %lld, ptsXine: %lld, dVA: %lld, dVD: %lld, dV: %lld, dA: %lld, dP: %lld, dD: %lld\n", (lld_t)ptsV, (lld_t)ptsA, (lld_t)ptsP, (lld_t)ptsD, (lld_t)ptsX, (lld_t)dVA, (lld_t)dVD, (lld_t)dV, (lld_t)dA, (lld_t)dP, (lld_t)dD);
        }
      }
    }
    
/*
if (blahblah)
{
  blahblah = false;
  fprintf(stderr, "blahblah C3");
  for (int i = 0; i < 50 && i < Length; i++)
    fprintf(stderr, " %02x", Data[ i ]);
  fprintf(stderr, "\n");
}
*/
/*
if (aaz)
{
  if (Data[3] == 0xbd || (Data[3] & 0xe0) == 0xc0) {
fprintf(stderr, "===== %c =====\n", Data[3] == 0xbd ? 'D' : 'A');
if (aaz != (Data[3] == 0xbd ? 1 : 2)) *(char *)0 = 0;
aaz = 0;
  }
}
*/ 
    int done = 0;

    while (done < Length)
    {
      int r = m_xineLib.execFuncStream1(Data + done, Length - done);
      if (r < 0)
        return r;
      
      done += r;
    }

    return done;
  }
  
  int cXineDevice::PlayVideo2(const uchar *Data, int Length, const bool stillImageData)
  {
//    fprintf(stderr, "D");
    
    int done = 0;

    while (done < Length)
    {
      char ch = 'X';
      
      int todo = Length - done;
      
      if (todo >= 6)
      {
        if (0x00 == Data[ done + 0 ]
            && 0x00 == Data[ done + 1 ]
            && 0x01 == Data[ done + 2 ])
        {
          int id  = Data[ done + 3 ];
          int len = 6 + Data[ done + 4 ] * 0x0100 + Data[ done + 5 ];

          if (0xba == id) // pack header has fixed length
          {
            if (0x00 == (0xc0 & Data[ done + 4 ])) // MPEG 1
              len = 12;
            else // MPEG 2
              len = 14 + (Data[ done + 13 ] & 0x07);
          }
          else if (0xb9 == id) // stream end has fixed length
          {
            len = 4;
          }

          if (todo >= len)
          {
            todo = len;

            if (0xbe == id   // padding
              || 0xba == id  // system layer: pack header
              || 0xbb == id  // system layer: system header
              || 0xb9 == id) // system layer: stream end
            {
              done += todo;

//              fprintf(stderr, "x");
              continue;
            }

            ch = '.';            
          }
          else        
          {
//            ::fprintf(stderr, "todo: %d, len: %d\t", todo, len);
            VERBOSE_NOP();
            ch = '3';

//            break;
          }
        }
        else        
        {
          VERBOSE_NOP();
          ch = '2';
        }
      }
      else        
      {
        VERBOSE_NOP();
        ch = '1';
      }
      
//      fprintf(stderr, "%c", ch);
      
      int r = PlayVideo3(Data + done, todo, stillImageData);
      if (r < 0)
        return r;

      done += r;
    }
    
    return done;
  }

  static void resetScramblingControl(const uchar *Data, int Length)
  {
    if (Length < 6)
    {
      VERBOSE_NOP();
      return;
    }
      
    if (0x00 != Data[ 0 ]
        || 0x00 != Data[ 1 ]
        || 0x01 != Data[ 2 ])
    {
      VERBOSE_NOP1();
      return;
    }
      
    if (0xe0 != (0xf0 & Data[ 3 ])      // video
        && 0xc0 != (0xe0 & Data[ 3 ])   // audio
        && 0xbd != (0xff & Data[ 3 ])   // dolby
        && 0xbe != (0xff & Data[ 3 ])   // padding (DVD)
        && 0xba != (0xff & Data[ 3 ])   // system layer: pack header
        && 0xbb != (0xff & Data[ 3 ])   // system layer: system header
        && 0xb9 != (0xff & Data[ 3 ]))  // system layer: stream end
    {
      VERBOSE_NOP();
      return;
    }
        
    if (0xbe == (0xff & Data[ 3 ])    // padding (DVD)
      || 0xba == (0xff & Data[ 3 ])   // system layer: pack header
      || 0xbb == (0xff & Data[ 3 ])   // system layer: system header
      || 0xb9 == (0xff & Data[ 3 ]))  // system layer: stream end
    {
      return;
    }

    int len = (6 + Data[ 4 ] * 0x100 + Data[ 5 ]);
    if (len < 6 + 1
        || Length < 6 + 1)
    {
      VERBOSE_NOP();
      return;
    }

    if (0x00 != (0x30 & Data[ 6 ]))
    {
      if (0x80 == (0xc0 & Data[ 6 ]))  // only touch MPEG2
      {
        xfprintf(stderr, "reseting PES_scrambling_control: 0x%02x\n", Data[ 6 ]);

        ((uchar *)Data)[ 6 ] &= ~0x30;
      }
    }
  }
  
  int cXineDevice::PlayVideo3(const uchar *Data, int Length, const bool stillImageData)
  {
/*
if (blahblah)
{
  fprintf(stderr, "blahblah V3");
  for (int i = 0; i < 50 && i < Length; i++)
    fprintf(stderr, " %02x", Data[ i ]);
  fprintf(stderr, "\n");
}
*/
    resetScramblingControl(Data, Length);
    
    dumpAudio("Video", Data, Length);

    return PlayCommon(Data, Length, stillImageData);
  }
  
  static double videoFrameDuration(const uchar *Data, int Length)
  {
    int PesPayloadOffset = 0;
    ePesHeader ph = AnalyzePesHeader(Data, Length, PesPayloadOffset);
    if (ph < phMPEG1)
      return -1;

    if ((Data[ 3 ] & 0xf0) != 0xe0)
      return -1;

    if (Length < PesPayloadOffset + 8)
      return -1;

    const uchar *p = Data + PesPayloadOffset;

    if (*p++ != 0x00)
      return -1;

    if (*p++ != 0x00)
      return -1;

    if (*p++ != 0x01)
      return -1;

    if (*p++ != 0xb3)
      return -1;

    p += 3;

    static const double frameRates[ 16 ] =
    {
      -1,
      24000/1001.0,
      24,
      25,
      30000/1001.0,
      30,
      50,
      60000/1001.0,
      60,
      -1,
      -1,
      -1,
      -1,
      -1,
      -1,
      -1,
    };

    int frameRateIndex = *p & 0x0f;

    if (frameRates[ frameRateIndex ] < 0)
      return -1;

    long n = 0, d = 0;

    if (ph == phMPEG2)
    {
      const uchar *const limit = Data + Length - 10 + 3;
      while (p < limit)
      {
        if (p[ 0 ] != 0x01 || p[ -1 ] != 0x00 || p[ -2 ] != 0x00)
        {
          p++;
          continue;
        }

        if (p[ 1 ] != 0xb5) // extension start code
          break;

        if ((p[ 2 ] & 0xf0) != 0x10) // sequence extension
          break;

        p += 7;

        n = (*p & 0x60) >> 5;
        d = (*p & 0x1f);
        break;
      }
    }

    return 90000 / (frameRates[ frameRateIndex ] * (n + 1) / (d + 1));
  }

  static int frameSizes[ 256 ];
  
  static double audioFrameDuration(const uchar *Data, int Length)
  {
    int PesPayloadOffset = 0;
    ePesHeader ph = AnalyzePesHeader(Data, Length, PesPayloadOffset);
    if (ph < phMPEG1)
      return -1;

    if ((Data[ 3 ] & 0xff) == 0xbd)
    {
      static int samplingFrequencies[ 4 ] = // all values are specified in Hz
      {
        48000, 44100, 32000, -1
      };

      if (PesPayloadOffset + 5 <= Length
        && Data[ PesPayloadOffset + 0 ] == 0x0b
        && Data[ PesPayloadOffset + 1 ] == 0x77
        && frameSizes[ Data[ PesPayloadOffset + 4 ] ] > 0)
      {
        if (samplingFrequencies[ Data[ PesPayloadOffset + 4 ] >> 6 ] < 0)
          return -1;

        return 90000.0 * 1536 / samplingFrequencies[ Data[ PesPayloadOffset + 4 ] >> 6 ];
      }
      else if (PesPayloadOffset + 4 + 5 <= Length
        && Data[ PesPayloadOffset + 4 + 0 ] == 0x0b
        && Data[ PesPayloadOffset + 4 + 1 ] == 0x77
        && frameSizes[ Data[ PesPayloadOffset + 4 + 4 ] ] > 0)
      {
        if (samplingFrequencies[ Data[ PesPayloadOffset + 4 + 4 ] >> 6 ] < 0)
          return -1;

        return 90000.0 * 1536 / samplingFrequencies[ Data[ PesPayloadOffset + 4 + 4 ] >> 6 ];
      }
      else 
        return -1;
    }

    if ((Data[ 3 ] & 0xe0) != 0xc0)
      return -1;

    if (Length < PesPayloadOffset + 4)
      return -1;

    ulong Header = 0;
    Header |= Data[ PesPayloadOffset + 0 ];
    Header <<= 8;
    Header |= Data[ PesPayloadOffset + 1 ];
    Header <<= 8;
    Header |= Data[ PesPayloadOffset + 2 ];
    Header <<= 8;
    Header |= Data[ PesPayloadOffset + 3 ];

    bool Mpeg2 = (ph == phMPEG2);
    
    int syncword           = (Header & 0xFFF00000) >> 20;
    int id                 = (Header & 0x00080000) >> 19;
    int layer              = (Header & 0x00060000) >> 17;
//  int protection_bit     = (Header & 0x00010000) >> 16;
    int bitrate_index      = (Header & 0x0000F000) >> 12;
    int sampling_frequency = (Header & 0x00000C00) >> 10;
//  int padding_bit        = (Header & 0x00000200) >>  9;
//  int private_bit        = (Header & 0x00000100) >>  8;
//  int mode               = (Header & 0x000000C0) >>  6;
//  int mode_extension     = (Header & 0x00000030) >>  4;
//  int copyright          = (Header & 0x00000008) >>  3;
//  int orignal_copy       = (Header & 0x00000004) >>  2;
    int emphasis           = (Header & 0x00000003);

    if (syncword != 0xFFF)
      return -1;

    if (id == 0 && !Mpeg2) // reserved in MPEG 1
      return -1;

    if (layer == 0) // reserved
      return -1;

    if (bitrate_index == 0xF) // forbidden
      return -1;

    if (sampling_frequency == 3) // reserved
      return -1;

    if (emphasis == 2) // reserved
      return -1;

    static int samplingFrequencies[ 2 ][ 4 ] = // all values are specified in Hz
    {
      { 44100, 48000, 32000, -1 }, // MPEG 1
      { 22050, 24000, 16000, -1 }  // MPEG 2
    };

    static int slots_per_frame[ 2 ][ 3 ] =
    {
      { 12, 144, 144 }, // MPEG 1, Layer I, II, III
      { 12, 144,  72 }  // MPEG 2, Layer I, II, III
    };

    int mpegIndex  = 1 - id;
    int layerIndex = 3 - layer;

    // Layer I (i. e., layerIndex == 0) has a larger slot size
    int slotSize = (layerIndex == 0) ? 4 : 1; // bytes

    int sf = samplingFrequencies[ mpegIndex ][ sampling_frequency ];

//fprintf(stderr, "afd: %.3f ms, PES-Length: %d\n", 1000.0 * 8 * slotSize * slots_per_frame[ mpegIndex ][ layerIndex ] / sf, Length);

    return 90000.0 * 8 * slotSize * slots_per_frame[ mpegIndex ][ layerIndex ] / sf;
  }
 
  static double softStartTime = 0;
  static const double softStartSpeedStart = 0.75;
  static const double softStartSpeedMin = 0.15;
  static double softStartDurationVideoSD = 31 / 25.0;
  static double softStartDurationVideoHD = 31 / 25.0;
  static double softStartDurationAudio   = 31 / 25.0;
  static const int softStartMaxSpeedChanges = 20;
  static double softStartLastSpeed = -1;
  static double softStartSpeedChangeTime = -1;
  static int64_t softStartPtsVdr = -1;
  static const int64_t softStartLogPtsDelta = 4 * 90000;
  static int softStartHitPoll = 0;
  static bool softStartNoMetronom = false;
  static const double pi = 4 * ::atan(1);
  static bool softStartHasVideo = true;
  static bool softStartHasAudio = true;
  static bool softStartIsVideoHD = false;
 
  inline double getSoftStartDuration(const bool forVideo)
  {
    if (forVideo)
      return softStartHasVideo ? (softStartIsVideoHD ? softStartDurationVideoHD : softStartDurationVideoSD) : 0;

    return softStartHasAudio ? softStartDurationAudio : 0;
  }

  inline double getSoftStartDuration()
  {
    double dV = getSoftStartDuration(true);
    double dA = getSoftStartDuration(false);
    return (dV > dA) ? dV : dA;
  }

  static double softStartCalcSpeed0(const double /* t */)
  {
    return 0;
  }
  
  static double softStartCalcSpeed1(const double t)
  {
    const double p = (1 + ::cos(2 * pi * t)) / 2;    
          
    return softStartSpeedMin + p * ((softStartSpeedStart * (1 - t) + 1.0 * t) - softStartSpeedMin);
  }
  
  static double softStartCalcSpeed2(const double t)
  {
    double p = 2 * t - 1;
    if (p < 0)
      p = -p;

//    p = p * p * p;
    p = p * p;
          
    return softStartSpeedMin + p * ((softStartSpeedStart * (1 - t) + 1.0 * t) - softStartSpeedMin);
  }
  
  static double softStartCalcSpeed3(const double t)
  {
    if (t < 0.25)
      return softStartSpeedStart;

    if (t < 0.50)
      return softStartSpeedMin + (1 - softStartSpeedMin) * 0 / 3;

    if (t < 0.75)
      return softStartSpeedMin + (1 - softStartSpeedMin) * 1 / 3;

    return   softStartSpeedMin + (1 - softStartSpeedMin) * 2 / 3;
  }
  
  static double softStartCalcSpeed4(const double t)
  {
    const double p = (1 + ::cos(pi * (1 + t))) / 2;    
          
    return softStartSpeedMin + (1 - softStartSpeedMin) * p;
  }

  static double softStartCalcSpeed(const double t)
  {
    if (t >= 1)
      return 1;

    return softStartCalcSpeed0(t);  // choose a method

    (void)softStartCalcSpeed0;
    (void)softStartCalcSpeed1;
    (void)softStartCalcSpeed2;
    (void)softStartCalcSpeed3;
    (void)softStartCalcSpeed4;
  }

  bool softStartPoll(cXineLib &xineLib, cPoller &poller, const int timeout, bool result)
  {
    if (softStartState > sIdle)
    {
      if (result)
      {
        softStartHitPoll = 0;
      }
      else if (++softStartHitPoll >= 2)
      {
        do
        {
          softStartState = sIdle;
          
          xineLib.execFuncFirstFrame();
          xineLib.execFuncSetSpeed(100.0);
          xineLib.execFuncWait();
        }
        while (!xineLib.Poll(poller, timeout, true));

        softStartHitPoll = 0;
        result = true;
      }

//      ::fprintf(stderr, "softStartHitPoll: %d, %d\n", result, softStartHitPoll);      
    }

    return result;
  }

  static int64_t vpts, apts, extra0, extra;
  static bool useApts;
  static bool seenAudio = false;
  static bool seenVideo = false;
  static bool seenApts = false;
  static bool seenVpts = false;
  
  static const int64_t extra_max = 50 * 90000 / 25;

  static bool oldMode = false;

  static inline int64_t calcPtsDelta(int64_t lhs, int64_t rhs)
  {
    int64_t delta = lhs - rhs;

    if (delta < -(1ll << 32))
      delta += 1ll << 33;
    else if (delta > +(1ll << 32))
      delta -= 1ll << 33;

    return delta;
  }

  static inline double softStartCalcQ0(int64_t vdr, int64_t xine, bool queued, bool forVideo, int64_t hyst)
  {
    if (vdr < 0 || xine < 0)
      return 0;

    double delta = calcPtsDelta(vdr, xine) / 90000.0;
    if (delta <= 0)
      return 0;

    if (delta >= softStartLogPtsDelta / 90000.0)
      return queued ? 0 : 1;

    if (delta >= (getSoftStartDuration(forVideo) + hyst / 90000.0))
      return 1;

    return delta / (getSoftStartDuration(forVideo) + hyst / 90000.0);
  }

  static bool gotQ1 = false;
  static int64_t hystQ = 0;
  static int64_t hystQ1 = 0;
  static bool vdrTime100reload = false;

  static inline double softStartCalcQ(int64_t vdr, int64_t xine, bool queued, bool forVideo)
  {
    return softStartCalcQ0(vdr, xine, queued, forVideo, hystQ);
  }

  static inline double softStartCalcQ(int64_t vdrVideo, int64_t vdrAudio, int64_t xine, bool queued)
  {
    double qV = softStartHasVideo ? softStartCalcQ(vdrVideo, xine, queued, true)  : 1;
    double qA = softStartHasAudio ? softStartCalcQ(vdrAudio, xine, queued, false) : 1;
    double q = (qV < qA) ? qV : qA;

    bool gQ1 = (q >= 1);
    if (gQ1 != gotQ1)
    {
      gotQ1 = gQ1;

      if (gotQ1)
      {
        hystQ = 0;
      }
      else
      {
        vdrTime100reload = true;
        hystQ = (hystQ1 += 90000 / 25);
      }
    }

//fprintf(stderr, "q: %.3lf, gq1: %d, hystQ: %lld, hystQ1: %lld, vdr: %lld, xine: %lld, d: %lld\n", q, gotQ1, hystQ, hystQ1, vdr, xine, vdr - xine);
    return q;
  }

  static int64_t vdrPTSLast = -2;
  static int64_t vdrAptsLast = -1;
  static int64_t vdrVptsLast = -1;

  static double vdrVptsCalced = -1;
  static double vdrAptsCalced = -1;
  static double vdrVduration = -1;
  static double vdrAduration = -1;

  static double vdrVptsBuffered[ 2 ] = { -1, -1 };

  static bool getPTS(const uchar *Data, int Length, int64_t &pts, bool isVideo, bool isAudio, double &vptsCalced, double &aptsCalced, double &vDuration, double &aDuration, double vptsBuffered[2])
  {
    uchar pt = NO_PICTURE;

    if (isVideo)
    {
      cRemux::ScanVideoPacket(Data, Length, 0, pt);

      if (pt != NO_PICTURE && vDuration > 0 && vptsBuffered[ 1 ] > 0)
        vptsBuffered[ 1 ] += vDuration;
    }

    bool retVal = false;

    if (getPTS(Data, Length, pts))
    {
      if (isAudio)
      {
        aptsCalced = pts;

        double duration = audioFrameDuration(Data, Length);
        if (duration >= 0)
          aDuration = duration;
      }

      if (isVideo)
      {
        if (pt == B_FRAME)
          vptsCalced = pts;
        else if (pt != NO_PICTURE)
          vptsBuffered[ 1 ] = pts;

        double duration = videoFrameDuration(Data, Length);
        if (duration >= 0)
          vDuration = duration;

        if (pt != B_FRAME)
          goto other;
      }

      return true;
    }
    else
    {
      if (isAudio)
      {
        double duration = audioFrameDuration(Data, Length);

        bool frameStart = (duration > -1);

        if (aptsCalced > -1 && aDuration > -1 && frameStart)
        {
          aptsCalced += aDuration;
          while (aptsCalced >= 0x200000000ull)
            aptsCalced -= 0x200000000ull;

          pts = (int64_t)aptsCalced;

          retVal = true;
        }

        if (duration >= 0)
          aDuration = duration;
      }

      if (isVideo)
      {
other:
        double dur = vDuration;

        if (pt != B_FRAME && pt != NO_PICTURE)
        {
          vptsCalced = vptsBuffered[ 0 ];
          vptsBuffered[ 0 ] = vptsBuffered[ 1 ];
          dur = 0;
        }
          
        bool frameStart = (pt != NO_PICTURE);

        if (vptsCalced > -1 && dur > -1 && frameStart)
        {
          vptsCalced += dur;
          while (vptsCalced >= 0x200000000ull)
            vptsCalced -= 0x200000000ull;

          pts = (int64_t)vptsCalced;

          retVal = true;
        }

        double duration = videoFrameDuration(Data, Length);
        if (duration >= 0)
          vDuration = duration;
      }

      return retVal;
    }
  }

  static double vdrTime100 = -1;

  int cXineDevice::PlayCommon(const uchar *Data, int Length, const bool stillImageData)
  {
    const bool isAudio = !IsVideo(Data, Length);
    const bool isVideo = !isAudio && !stillImageData;
//if (isAudio && ts) fprintf(stderr, "A");
    if ((stillImageData || m_xineLib.isTrickSpeedMode())
      && isAudio)
    {
//      ::fprintf(stderr, "x");
      return Length;
    }
/*    
    if (stillImageData)
      fprintf(stderr, "writing: %d\n", Length - 6);
*/
    if (findVideo && (isVideo || isAudio))
    {
      findVideo = false;
      foundVideo = isVideo;
//xfprintf(stderr, "foundVideo: %d\n", foundVideo);
    }

if (0 && isAudio) {
static int64_t opts = -1;
static int64_t xpts = -1;
getPTS(Data, Length, xpts, isVideo, isAudio, vdrVptsCalced, vdrAptsCalced, vdrVduration, vdrAduration, vdrVptsBuffered);
//fprintf(stderr, "audio: %.3lf, %d, %lld, %lld\n", audioFrameDuration(Data, Length), Length, xpts, xpts - opts);
opts = xpts;
}
if (0 && isVideo) {
static int64_t opts = -1;
static int64_t xpts = -1;
getPTS(Data, Length, xpts, isVideo, isAudio, vdrVptsCalced, vdrAptsCalced, vdrVduration, vdrAduration, vdrVptsBuffered);
//fprintf(stderr, "video: %.3lf, %d, %lld, %lld\n", videoFrameDuration(Data, Length), Length, xpts, xpts - opts);
opts = xpts;
}

    int64_t ptsForce = -1;

    {
      cMutexLock lock(&softStartMutex);

      if (softStartTrigger
        && !stillImageData)
      {
        softStartNoMetronom = (sstNoMetronom == softStartTrigger);
        softStartTrigger = sstNone;

        softStartState = sInitiateSequence;
//fprintf(stderr, "/////////////////////////////\n");
//        ::fprintf(stderr, "#(%d,%d)", ::getpid(), pthread_self());
      }

      if (stillImageData)
      {
        softStartLastSpeed = -1;
      }
      else if (softStartState > sIdle)
      {
        timeval tv;
        ::gettimeofday(&tv, 0);

        const double now = (tv.tv_sec + tv.tv_usec / 1.0e+6);

        if (softStartState == sInitiateSequence)
        {
          xfprintf(stderr, "[");
      
          softStartDurationVideoSD = m_settings.GetModeParams()->m_prebufferFramesVideoSD / 25.0;
          softStartDurationVideoHD = m_settings.GetModeParams()->m_prebufferFramesVideoHD / 25.0;
          softStartDurationAudio   = m_settings.GetModeParams()->m_prebufferFramesAudio   / 25.0;
          softStartHasVideo = false;
          softStartHasAudio = false;
          softStartIsVideoHD = false;
 
          softStartTime = now;        
        
          softStartSpeedChangeTime = -1;
          softStartLastSpeed = -1;
          softStartPtsVdr = -1;
          softStartHitPoll = 0;

          softStartState = sStartSequence;

          vpts = -1;
          apts = -1;
          extra0 = 0;
          extra = 0;

          useApts = 1;
        
          seenAudio = false;
          seenVideo = false;
          seenApts = false;
          seenVpts = false;

          vdrPTSLast = -2;
          vdrAptsLast = -1;
          vdrVptsLast = -1;
        
          vdrAptsCalced = -1;
          vdrVptsCalced = -1;
          vdrAduration = -1;
          vdrVduration = -1;

          vdrVptsBuffered[ 0 ] = -1;
          vdrVptsBuffered[ 1 ] = -1;

          vdrTime100 = -1;
          vdrTime100reload = false;

          gotQ1 = false;
          hystQ = hystQ1 = 90000 * m_settings.GetModeParams()->m_prebufferHysteresis / 25;
        }

        {
          char packetType = 0;
          char packetTypeOnce = 0;

          if (isVideo)
          {
            if (!seenVideo)
            {
              softStartHasVideo = true;
              softStartIsVideoHD = cRemux::IsFrameH264(Data, Length);
//fprintf(stderr, "\nH264: %d\n", softStartIsVideoHD);

              seenVideo = true;
//              ::fprintf(stderr, "seen video\n");
//              xfprintf(stderr, "v");
              packetTypeOnce = 'v';
            }

            packetType = 'v';
          }
          else if (isAudio)
          {
            if (!seenAudio)
            {
              softStartHasAudio = true;

              audioSeen = true;
              seenAudio = true;
//              ::fprintf(stderr, "seen audio\n");
//              xfprintf(stderr, "a");
              packetTypeOnce = 'a';
            }

            packetType = 'a';
          }
        
          int64_t pts = 0;
          if (getPTS(Data, Length, pts, isVideo, isAudio, vdrVptsCalced, vdrAptsCalced, vdrVduration, vdrAduration, vdrVptsBuffered))
          {
            ptsForce = pts;

            if (isVideo)
            {
              if (!seenVpts)
              {
                seenVpts = true;
//                ::fprintf(stderr, "seen video pts\n");
//                xfprintf(stderr, "V");
                packetTypeOnce = 'V';
              }

              packetType = 'V';
            
              vpts = pts;

              if (apts > -1)
              {
                int64_t delta = calcPtsDelta(vpts, apts);
                if (delta < 0)
                  delta = - delta;

                if (extra0 < delta)
                {
                  extra0 = delta;

//                  ::fprintf(stderr, "max. A/V delta: %lld pts => total extra buffering: %d frames", extra0, (int)(extra0 * 25 / 90000));

                  extra = extra0;
                
                  if (extra > extra_max)
                  {
                    extra = extra_max;

//                    ::fprintf(stderr, ", limited to %d frames", (int)(extra * 25 / 90000));
                  }	

//                ::fprintf(stderr, "\n");
                  if (oldMode)
                    xfprintf(stderr, "+%d", (int)(extra * 25 / 90000));
                }
              }

//              ::fprintf(stderr, "video: v: %lld, a: %lld, d: %lld, e: %lld\n", vpts, apts, vpts - apts, extra);
            }
            else if (isAudio)
            {
              if (!seenApts)
              {
                seenApts = true;
//                ::fprintf(stderr, "seen audio pts\n");
//                xfprintf(stderr, "A");
                packetTypeOnce = 'A';
              }
            
              packetType = 'A';

              apts = pts;
            
              if (vpts > -1)
              {
                int64_t delta = calcPtsDelta(vpts, apts);
                if (delta < 0)
                  delta = - delta;

                if (extra0 < delta)
                {
                  extra0 = delta;
                
//                  ::fprintf(stderr, "max. A/V delta: %lld pts => total extra buffering: %d frames", extra0, (int)(extra0 * 25 / 90000));

                  extra = extra0;
                
                  if (extra > extra_max)
                  {
                    extra = extra_max;

//                    ::fprintf(stderr, ", limited to %d frames", (int)(extra * 25 / 90000));
                  }	

//                  ::fprintf(stderr, "\n");
                  if (oldMode)
                    xfprintf(stderr, "+%d", (int)(extra * 25 / 90000));
                }
              }

//              ::fprintf(stderr, "audio: v: %lld, a: %lld, d: %lld, e: %lld\n", vpts, apts, vpts - apts, extra);
            }
          }

//xfprintf(stderr, "%s%c", getFrameType(Data, Length), packetType); packetTypeOnce = 0; //ZZZ
//
          if (packetTypeOnce)
            xfprintf(stderr, "%c", packetTypeOnce);
        }

        if ((seenVideo && !seenVpts)
            || (seenAudio && !seenApts))
        {
          softStartTime = now;
        }
      
        if (softStartState >= sStartSequence)
        {
          if (isVideo)
            useApts = false;
        
          if (useApts
              || isVideo)
          {
            softStartPtsVdr = ptsForce;
//            getPTS(Data, Length, softStartPtsVdr);

            if (softStartPtsVdr != -1)
            {
              bool queued = false; 
              int64_t ptsXine = -1;
              m_xineLib.execFuncGetPTS(ptsXine, 0, &queued);
           
              const int64_t delta = (ptsXine >= 0) ? calcPtsDelta(softStartPtsVdr, ptsXine) : 0;
              if (softStartState == sStartSequence
                  || delta < -softStartLogPtsDelta
                  || (delta > +softStartLogPtsDelta && queued))
              {
//                if (softStartState != sStartSequence)
//                  ::fprintf(stderr, "SoftStart: ptsVdr: %12"PRId64", ptsXine: %12"PRId64", delta: %12"PRId64", queued: %d\n", softStartPtsVdr, ptsXine, delta, queued);

//AAA                m_xineLib.execFuncStart();
//AAA                m_xineLib.execFuncWait();

                if (!softStartNoMetronom)
                {
                  xfprintf(stderr, "M");
//                  xfprintf(stderr, " %12" PRId64 ", %12" PRId64 ", %12" PRId64 "\n", delta, softStartPtsVdr, ptsXine);
                
//ZZZ                  m_xineLib.execFuncMetronom(softStartPtsVdr);
//ZZZ                  m_xineLib.execFuncWait();
                }
              
                softStartTime = now;
              
                softStartState = sContinueSequence;
              }
            }
          }
        }
        
//        if (softStartState <= sStartSequence)
//          stripPTSandDTS((uchar *)Data, Length);

        m_xineLib.execFuncFirstFrame();

        int64_t vdrPTS = -1;
 
        if ((seenVideo && vpts <= -1)
          || (seenAudio && apts <= -1))
        {
        }
        else if (vpts > -1)
        {
          if (apts > -1 && vpts > apts)
            vdrPTS = apts;
          else
            vdrPTS = vpts;
        }
        else if (apts > -1)
          vdrPTS = apts;

        bool queued = false;
        int64_t xinePTS = -1;
        m_xineLib.execFuncGetPTS(xinePTS, 0, &queued);

        const double totalDuration = (getSoftStartDuration() + extra / 90000.0);
        const double q = oldMode ? ((now - softStartTime) / totalDuration) : softStartCalcQ(vpts, apts, xinePTS, queued); 
        double p = softStartCalcSpeed(q);
     
        if (!oldMode)
        {
          if (vdrPTSLast == vdrPTS || vdrVptsLast > vpts || vdrAptsLast > apts)
            p = softStartLastSpeed / 100;
          else
            vdrPTSLast = vdrPTS;

          if (apts != vdrAptsLast || vpts != vdrVptsLast)
          {
//          xfprintf(stderr, "p: %.3lf, DA: %lld, DV: %lld, DC: %lld\n", p, (apts > -1) ? (apts - xinePTS) : -1, (vpts > -1) ? (vpts - xinePTS) : -1, (vdrPTS > - 1) ? (vdrPTS - xinePTS) : -1);
//          xfprintf(stderr, "(%.1lf|%.1lf)", 25 * ((apts > -1) ? ((apts - xinePTS) / 90000.0) : 0.0), 25 * ((vpts > -1) ? ((vpts - xinePTS) / 90000.0) : 0.0));
          }

          vdrVptsLast = vpts;
          vdrAptsLast = apts;
        }

        double speed = (p > 0) ? (100.0 * p) : 12.5;
         
        if (speed >= 100.0 || xinePTS == -1)
        { 
          bool newlineRequired = false;
           
          if (vdrTime100 < 0)
          {
ttt6 = tNow();
/*
fprintf(stderr, "+++++ %.3lf ms, %.3lf ms, %.3lf ms, (%.3lf ms, %.3lf ms, %.3lf ms) +++++\n"
, (ttt1 - ttt0) * 1000.0
, (ttt2 - ttt1) * 1000.0
, (ttt3 - ttt2) * 1000.0
, (ttt4 - ttt3) * 1000.0
, (ttt5 - ttt3) * 1000.0
, (ttt6 - ttt3) * 1000.0
);
*/
            vdrTime100 = now;

            xfprintf(stderr, "]");
            newlineRequired = true;
          }
        
          speed = 100.0;

          if (vdrTime100reload)
          {
            vdrTime100reload = false;

            vdrTime100 = now;
          }
      
          if (xinePTS < 0)
            softStartState = sIdle;
          else if ((now - vdrTime100) >= m_settings.GetModeParams()->m_monitoringDuration)
          {
            if (m_settings.GetModeParams()->MonitoringContinuous())
              hystQ1 = 90000 * (m_settings.GetModeParams()->m_prebufferHysteresis - 1) / 25;
            else
              softStartState = sIdle;
          }

          if ((softStartState == sIdle || speed != softStartLastSpeed) && vdrPTS > -1 && xinePTS > -1)
          {
            xfprintf(stderr, "buffered %.1lf frames (v:%.1lf, a:%.1lf)", calcPtsDelta(vdrPTS, xinePTS) / 90000.0 * 25, (vpts > -1) ? (calcPtsDelta(vpts, xinePTS) / 90000.0 * 25) : 0.0, (apts > -1) ? (calcPtsDelta(apts, xinePTS) / 90000.0 * 25) : 0.0);
            if (softStartState == sIdle)
              xfprintf(stderr, " <<<<<");
            newlineRequired = true;
          }
        
          if (newlineRequired)
            xfprintf(stderr, "\n");
        }

        if (queued)
          speed = 100;        

        if (100.0 == speed
            || (speed != softStartLastSpeed
                && (!oldMode || (now - softStartSpeedChangeTime) > (totalDuration / softStartMaxSpeedChanges))))
        {
          softStartSpeedChangeTime = now;
        
//          fprintf(stderr, "slowstart: %lg, %lg\n", speed, p);

          m_xineLib.execFuncSetSpeed(speed);
          m_xineLib.execFuncWait();

//          if (100.0 == speed)
//            m_xineLib.execFuncSetPrebuffer(m_settings.GetModeParams()->m_prebufferFrames);
        
          softStartLastSpeed = speed;
        }
      }
    }

//if (jw) *(char *)0 = 0;
    jw = false;
    
//    fprintf(stderr, "v");
    
    if (ts)
    {
//NNN      stripPTSandDTS((uchar *)Data, Length);
    }
    else if (0) //ZZZ
    {
      int64_t pts = 0;
      
      if (np && getPTS(Data, Length, pts))
      {
        np = false;
        
//        fprintf(stderr, "M %lld %llx\n", pts);
        m_xineLib.execFuncMetronom(pts);
        m_xineLib.execFuncWait();
      }
    }
        
    int r = PlayCommon1(Data, Length, ptsForce);

//    fprintf(stderr, "V");

    return r;
  }

  int cXineDevice::PlayCommon1(const uchar *Data, int Length, int64_t ptsForce)
  {
/*
if (blahblah)
{
  fprintf(stderr, "blahblah C1");
  for (int i = 0; i < 50 && i < Length; i++)
    fprintf(stderr, " %02x", Data[ i ]);
  fprintf(stderr, "\n");
}
*/
    if (oldMode)
      return PlayCommon2(Data, Length, ptsForce);

    struct sBuffer
    {
      sBuffer *next;
      int length;
      int64_t ptsForce;
      uchar data[1];
    };

    static sBuffer *pHead = 0;
    static sBuffer *pTail = 0;
    static int bufCount = 0;

    while (!doClear && pHead)
    {
      if (sIdle != softStartState)
      {
        cPoller p;
        if (!m_xineLib.Poll(p, -1))
          break;
      }

      int r = PlayCommon2(pHead->data, pHead->length, pHead->ptsForce);
      if (r > 0)
      {
        sBuffer *p = pHead;
        pHead = pHead->next;
        ::free(p);
//fprintf(stderr, "bufCount: %d\n", --bufCount);
      }
      else if (r < 0)
      {
        while (pHead)
        {
          sBuffer *p = pHead;
          pHead = pHead->next;
          ::free(p);
        }

        bufCount = 0;
        pTail = 0;
//fprintf(stderr, "--- bufCount: %d\n", bufCount);
        return r;
      }
      else
        break;
    }
    
    if (doClear)
    {
//fprintf(stderr, "----- clearing -----\n");
      while (pHead)
      {
        sBuffer *p = pHead;
        pHead = pHead->next;
        ::free(p);
      }

      bufCount = 0;
      pTail = 0;
      doClear = false;
//fprintf(stderr, "=== bufCount: %d\n", bufCount);

//      return Length;
    }

    if (!pHead)
    {
      pTail = 0;
      
      do // ... while (false);
      {
        if (sIdle != softStartState)
        {
          cPoller p;
          if (!m_xineLib.Poll(p, -1))
            break;
//fprintf(stderr, "### bufCount: %d\n", bufCount);
        }

        return PlayCommon2(Data, Length, ptsForce);
      }
      while (false);
    }

    sBuffer *p = (sBuffer *)::malloc(sizeof (sBuffer) - sizeof (p->data) + Length);
    if (!p)
      return 0;

    p->next = 0;
    p->length = Length;
    p->ptsForce = ptsForce;
    memcpy(&p->data, Data, Length);

    if (!pTail)
      pHead = pTail = p;
    else
    {
      pTail->next = p;
      pTail = p;
    }

//fprintf(stderr, "*** bufCount: %d\n", ++bufCount);
    return Length;
  }

  static uchar jumboPESdata[ 6 + 0xffff ];
  static uchar *jumboPEStailData = 0;
  
  static bool mkJumboPES(const uchar *Data, int Length)
  {
    int origJumboPESsize = jumboPESsize;
    jumboPESsize = 0;
    
    if (Length < 9)
      VERBOSE_RETURN0(false);

    if (0x00 != Data[ 0 ])
      VERBOSE_RETURN0(false);
    
    if (0x00 != Data[ 1 ])
      VERBOSE_RETURN0(false);
    
    if (0x01 != Data[ 2 ])
      VERBOSE_RETURN0(false);
    
    if (0xbd != Data[ 3 ])
      VERBOSE_RETURN0(false);

    int l = Data[ 4 ] * 256 + Data[ 5 ];    
    if ((6 + l) != Length)
    {
      const uchar *data = Data + (6 + l);
      int length = Length - (6 + l);
      
      if (length < 6)
        VERBOSE_RETURN3(false);
      
      if (0x00 != data[ 0 ])
        VERBOSE_RETURN3(false);
      
      if (0x00 != data[ 1 ])
        VERBOSE_RETURN3(false);
      
      if (0x01 != data[ 2 ])
        VERBOSE_RETURN3(false);
      
      if (0xbe != data[ 3 ])
        VERBOSE_RETURN3(false);
 
      int L = data[ 4 ] * 256 + data[ 5 ];    
      if ((6 + L) != length)
        VERBOSE_RETURN3(false);

      // ignore padding
      Length -= length;
    }
/*
    for (int i = 0; i < 20; i++)
      fprintf(stderr, "%02x ", Data[ i ]);
    fprintf(stderr, "\n");
*/    
    bool cont = (0x80 == Data[ 6 ]
                 && 0x00 == Data[ 7 ]
                 && 0x00 == Data[ 8 ]);

    if (cont
        && Length >= 6 + 3 + 5
        && Data[  9 ] == 0x0b
        && Data[ 10 ] == 0x77
        && frameSizes[ Data[ 13 ] ] > 0)
    {
      cont = false;
    }
    
    if (!cont
      || 0 == origJumboPESsize)
    {
      if (0 != origJumboPESsize)
        VERBOSE_RETURN0(false);

      if ((origJumboPESsize + Length - 0) > (6 + 0xffff))
        VERBOSE_RETURN0(false);

      if (jumboPEStail > 0)
      {
        int headerSize = 6 + 3 + Data[ 8 ];
        ::memcpy(&jumboPESdata[ origJumboPESsize ], &Data[ 0 ], headerSize);

        ::memmove(&jumboPESdata[ origJumboPESsize + headerSize ], jumboPEStailData, jumboPEStail);
        
        ::memcpy(&jumboPESdata[ origJumboPESsize + headerSize + jumboPEStail ], &Data[ headerSize ], Length - headerSize);
        
        origJumboPESsize += headerSize + jumboPEStail + Length - headerSize;

        jumboPEStail = 0;
        jumboPEStailData = 0;

        //FIXME: PTS should be adjusted to take care of jumboPEStail's duration.
        //       Otherwise there is a certain jitter on audio duration <=> PTS.
      }
      else
      {
        ::memcpy(&jumboPESdata[ origJumboPESsize ], &Data[ 0 ], Length - 0);
        origJumboPESsize += Length - 0;
      }
    }
    else
    {
      if (0 == origJumboPESsize)
        VERBOSE_RETURN0(false);
      
      if ((origJumboPESsize + Length - 9) > (6 + 0xffff))
        VERBOSE_RETURN0(false);
        
      ::memcpy(&jumboPESdata[ origJumboPESsize ], &Data[ 9 ], Length - 9);
      origJumboPESsize += Length - 9;
    }

    if (0 == origJumboPESsize)
      VERBOSE_RETURN0(false);

    jumboPESsize = origJumboPESsize;

    if (2048 == Length)
    {
//      fprintf(stderr, " b %d", jumboPESsize);
      return false;
    }
    
    jumboPESdata[ 4 ] = (jumboPESsize - 6) >> 8;
    jumboPESdata[ 5 ] = (jumboPESsize - 6) & 0xff;
    
//    fprintf(stderr, " B %d", jumboPESsize);
    return true;
  }

  static int initFrameSizes()
  {
    ::memset(frameSizes, 0, sizeof (frameSizes));

    // fs = 48 kHz
    frameSizes[ 0x00 ] =   64;
    frameSizes[ 0x01 ] =   64;
    frameSizes[ 0x02 ] =   80;
    frameSizes[ 0x03 ] =   80;
    frameSizes[ 0x04 ] =   96;
    frameSizes[ 0x05 ] =   96;
    frameSizes[ 0x06 ] =  112;
    frameSizes[ 0x07 ] =  112;
    frameSizes[ 0x08 ] =  128;
    frameSizes[ 0x09 ] =  128;
    frameSizes[ 0x0a ] =  160;
    frameSizes[ 0x0b ] =  160;
    frameSizes[ 0x0c ] =  192;
    frameSizes[ 0x0d ] =  192;
    frameSizes[ 0x0e ] =  224;
    frameSizes[ 0x0f ] =  224;
    frameSizes[ 0x10 ] =  256;
    frameSizes[ 0x11 ] =  256;
    frameSizes[ 0x12 ] =  320;
    frameSizes[ 0x13 ] =  320;
    frameSizes[ 0x14 ] =  384;
    frameSizes[ 0x15 ] =  384;
    frameSizes[ 0x16 ] =  448;
    frameSizes[ 0x17 ] =  448;
    frameSizes[ 0x18 ] =  512;
    frameSizes[ 0x19 ] =  512;
    frameSizes[ 0x1a ] =  640;
    frameSizes[ 0x1b ] =  640;
    frameSizes[ 0x1c ] =  768;
    frameSizes[ 0x1d ] =  768;
    frameSizes[ 0x1e ] =  896;
    frameSizes[ 0x1f ] =  896;
    frameSizes[ 0x20 ] = 1024;
    frameSizes[ 0x21 ] = 1024;
    frameSizes[ 0x22 ] = 1152;
    frameSizes[ 0x23 ] = 1152;
    frameSizes[ 0x24 ] = 1280;
    frameSizes[ 0x25 ] = 1280;
    
    // fs = 44.1 kHz
    frameSizes[ 0x40 ] =   69;
    frameSizes[ 0x41 ] =   70;
    frameSizes[ 0x42 ] =   87;
    frameSizes[ 0x43 ] =   88;
    frameSizes[ 0x44 ] =  104;
    frameSizes[ 0x45 ] =  105;
    frameSizes[ 0x46 ] =  121;
    frameSizes[ 0x47 ] =  122;
    frameSizes[ 0x48 ] =  139;
    frameSizes[ 0x49 ] =  140;
    frameSizes[ 0x4a ] =  174;
    frameSizes[ 0x4b ] =  175;
    frameSizes[ 0x4c ] =  208;
    frameSizes[ 0x4d ] =  209;
    frameSizes[ 0x4e ] =  243;
    frameSizes[ 0x4f ] =  244;
    frameSizes[ 0x50 ] =  278;
    frameSizes[ 0x51 ] =  279;
    frameSizes[ 0x52 ] =  348;
    frameSizes[ 0x53 ] =  349;
    frameSizes[ 0x54 ] =  417;
    frameSizes[ 0x55 ] =  418;
    frameSizes[ 0x56 ] =  487;
    frameSizes[ 0x57 ] =  488;
    frameSizes[ 0x58 ] =  557;
    frameSizes[ 0x59 ] =  558;
    frameSizes[ 0x5a ] =  696;
    frameSizes[ 0x5b ] =  697;
    frameSizes[ 0x5c ] =  835;
    frameSizes[ 0x5d ] =  836;
    frameSizes[ 0x5e ] =  975;
    frameSizes[ 0x5f ] =  976;
    frameSizes[ 0x60 ] = 1114;
    frameSizes[ 0x61 ] = 1115;
    frameSizes[ 0x62 ] = 1253;
    frameSizes[ 0x63 ] = 1254;
    frameSizes[ 0x64 ] = 1393;
    frameSizes[ 0x65 ] = 1394;
    
    // fs = 32 kHz
    frameSizes[ 0x80 ] =   96;
    frameSizes[ 0x81 ] =   96;
    frameSizes[ 0x82 ] =  120;
    frameSizes[ 0x83 ] =  120;
    frameSizes[ 0x84 ] =  144;
    frameSizes[ 0x85 ] =  144;
    frameSizes[ 0x86 ] =  168;
    frameSizes[ 0x87 ] =  168;
    frameSizes[ 0x88 ] =  192;
    frameSizes[ 0x89 ] =  192;
    frameSizes[ 0x8a ] =  240;
    frameSizes[ 0x8b ] =  240;
    frameSizes[ 0x8c ] =  288;
    frameSizes[ 0x8d ] =  288;
    frameSizes[ 0x8e ] =  336;
    frameSizes[ 0x8f ] =  336;
    frameSizes[ 0x90 ] =  384;
    frameSizes[ 0x91 ] =  384;
    frameSizes[ 0x92 ] =  480;
    frameSizes[ 0x93 ] =  480;
    frameSizes[ 0x94 ] =  576;
    frameSizes[ 0x95 ] =  576;
    frameSizes[ 0x96 ] =  672;
    frameSizes[ 0x97 ] =  672;
    frameSizes[ 0x98 ] =  768;
    frameSizes[ 0x99 ] =  768;
    frameSizes[ 0x9a ] =  960;
    frameSizes[ 0x9b ] =  960;
    frameSizes[ 0x9c ] = 1152;
    frameSizes[ 0x9d ] = 1152;
    frameSizes[ 0x9e ] = 1344;
    frameSizes[ 0x9f ] = 1344;
    frameSizes[ 0xa0 ] = 1536;
    frameSizes[ 0xa1 ] = 1536;
    frameSizes[ 0xa2 ] = 1728;
    frameSizes[ 0xa3 ] = 1728;
    frameSizes[ 0xa4 ] = 1920;
    frameSizes[ 0xa5 ] = 1920;
    
    return 0;
  };
  
#if APIVERSNUM < 10318

  void cXineDevice::PlayAudio(const uchar *Data, int Length)
  {
    cDevice::PlayAudio(Data, Length);

    PlayAudioCommon(Data, Length);
  }

#else

  int cXineDevice::GetAudioChannelDevice(void)
  {
    return m_audioChannel;
  }
static int m_acd = -1;
  void cXineDevice::SetAudioChannelDevice(int AudioChannel)
  {
#if APIVERSNUM >= 10701
if (m_acd == AudioChannel)
  return;
#endif
m_acd = AudioChannel;

    xfprintf(stderr, "SetAudioChannelDevice: %d\n", AudioChannel);

    m_audioChannel = AudioChannel;

    m_xineLib.execFuncSelectAudio(m_audioChannel);
  }
static int m_dad = -1;
  void cXineDevice::SetDigitalAudioDevice(bool On)
  {
#if APIVERSNUM >= 10701
if (m_dad == On)
  return;
#endif
m_dad = On;

    xfprintf(stderr, "SetDigitalAudioDevice: %d\n", On);
//aaz = On ? 1 : 2;
    m_xineLib.execFuncSelectAudio(On ? -1 : m_audioChannel);

    if (pmNone == pm)
      return;

    doClear = true;

    if (m_settings.LiveTV()
      && !audioSeen)
    {
      cMutexLock lock(&softStartMutex);
      if (softStartState == sIdle)
        softStartTrigger = sstNoMetronom;
      
      return;
    }
    
    m_xineLib.pause();
//xzabc = 1;    
    jumboPESsize = 0;
    jumboPEStail = 0;

    if (f)
        m_xineLib.execFuncSetSpeed(100.0);
//double t0 = tNow();
    if (m_settings.LiveTV()
      || !foundVideo) // radio recording: audio channels are not related
    {
      ptsV = ptsA = ptsP = ptsD = -1;
    
      m_xineLib.execFuncClear(-3);

//      if (!foundVideo)
//        m_xineLib.execFuncStart();
//      np = true;
    }
    
    m_xineLib.execFuncResetAudio();

    if (f)
        m_xineLib.execFuncSetSpeed(0.0);
    
    m_xineLib.execFuncWait();
//xzabc = 2;
    m_xineLib.pause(false);
//double t1 = tNow(); fprintf(stderr, "!!!!!!! %.3lf ms\n", (t1 - t0) * 1000);
    if (m_settings.LiveTV())
    {
      cMutexLock lock(&softStartMutex);
      softStartTrigger = sstNoMetronom;
    } 
//fprintf(stderr, "~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\n");    
  }

#if APIVERSNUM < 10342  

  int cXineDevice::PlayAudio(const uchar *Data, int Length)
  {
//    fprintf(stderr, " 0x%02x ", Data[ 3 ]);
    return PlayAudioCommon(Data, Length);
  }

#else
  
  int cXineDevice::PlayAudio(const uchar *Data, int Length, uchar /* Id */)
  {
//    fprintf(stderr, " 0x%02x ", Data[ 3 ]);
    return PlayAudioCommon(Data, Length);
  }

#endif
#endif
  
  int cXineDevice::PlayAudioCommon(const uchar *Data, int Length)
  {
    if (ttt5 == 0) ttt5 = tNow();
//    fprintf(stderr, " 0x%02x: %d ", Data[ 3 ], Data[4] * 256 + Data[5]);
/*
if (Data[ 3 ] == 0xd0) {
  FILE *f = fopen("/tmp/d0", "ab");
  fwrite(Data, Length, 1, f);
  fclose(f);
}
*/
    {
      static int i = initFrameSizes();
      (void)i;
    }
    
store_frame(Data, Length, __LINE__);
 
    LOG_ME(::fprintf(stderr, "A");)
    
    if (f)
    {      
      LOG_ME(::fprintf(stderr, "<");)
      return Length;
    }
    
    if (pmNone == pm)
    {
      cMutexLock pmMutexLock(&m_pmMutex);

      if (pmNone == pm)
        m_pmCondVar.Wait(m_pmMutex);
    }

    int retVal = PlayAudio2(Data, Length);
    
    LOG_ME(::fprintf(stderr, "a"));

    return retVal;
  }
  
  int cXineDevice::PlayAudio2(const uchar *Data, int Length)
  {
//    fprintf(stderr, "D");
    
    int done = 0;

    while (done < Length)
    {
      char ch = 'X';

      int id = 0x00;
      
      int todo = Length - done;
      if (todo >= 6)
      {
        if (0x00 == Data[ done + 0 ]
            && 0x00 == Data[ done + 1 ]
            && 0x01 == Data[ done + 2 ])
        {
          id  = Data[ done + 3 ];
          int len = Data[ done + 4 ] * 0x0100 + Data[ done + 5 ];

          if (todo >= (6 + len))
          {
            todo = (6 + len);

            if (0xbe == id)
            {
              done += todo;

//              ::fprintf(stderr, "x");
              continue;
            }

            ch = '.';            
          }
          else        
          {
            VERBOSE_NOP();
            ch = '3';
          }
        }
        else        
        {
          VERBOSE_NOP();
          ch = '2';
        }
      }
      else        
      {
        VERBOSE_NOP();
        ch = '1';
      }
      
//      ::fprintf(stderr, "%c", ch);

      int r;
      
      if (0xbd == id)
        r = PlayAudio3(Data + done, todo);
      else
        r = PlayVideo3(Data + done, todo, false);
        
      if (r < 0)
        return r;
      
      if (r != todo)
        VERBOSE_NOP();
                
      done += r;
    }
    
    return done;
  }
  
  int cXineDevice::PlayAudio3(const uchar *Data, int Length)
  {
    resetScramblingControl(Data, Length);
    
store_frame(Data, Length, __LINE__);    
/*    
    ::fprintf(stderr, "l: %d\t", Length);
    for (int i = 0; i < 20; i++)
      ::fprintf(stderr, "%02x ", Data[ i ]);
    ::fprintf(stderr, "\n");
*/ 
//    fprintf(stderr, "A");
    
    if (mkJumboPES(Data, Length))
    {
      int todo = jumboPESsize;
      jumboPESsize = 0;

      dumpAudio("Audio", jumboPESdata, todo);

      bool dolby = false;
      bool pcm = false;
      
      do
      {
        if (todo < (6 + 3 + 0 + 2))
          break;
        
        if (0x00 != jumboPESdata[ 0 ]
            || 0x00 != jumboPESdata[ 1 ]
            || 0x01 != jumboPESdata[ 2 ]
            || 0xbd != jumboPESdata[ 3 ])
        {
          break;
        }

        int l = jumboPESdata[ 4 ] * 0x0100 + jumboPESdata[ 5 ];
        if (l < (3 + 0 + 2))
          break;

        if (todo < (6 + l))
          break;

        int h = jumboPESdata[ 8 ];
        if (l < (3 + h + 2))
          break;

        if (0x0b == jumboPESdata[ 6 + 3 + h + 0 ]
            && 0x77 == jumboPESdata[ 6 + 3 + h + 1 ])
        {
          if (l < (3 + h + 2 + 2 + 1))
          {
            VERBOSE_NOP();            
            break;
          }
          
          int frameStart = 6 + 3 + h;
          bool failed = false;
          
          while (true)
          {
            int frameSize = 2 * frameSizes[ jumboPESdata[ frameStart + 4 ] ];
            if (frameSize <= 0)
            {
              failed = true;
              
              xfprintf(stderr, "frame_size_code: 0x%02x\n", jumboPESdata[ frameStart + 4 ]);
              VERBOSE_NOP();              
              break;
            }

            if (frameStart + frameSize > todo)
              break;

            frameStart += frameSize;
            
            if (frameStart + 2 + 2 + 1 > todo)
              break;

            if (0x0b != jumboPESdata[ frameStart + 0 ]
                || 0x77 != jumboPESdata[ frameStart + 1 ])
            {
              failed = true;
              
              VERBOSE_NOP();
              break;
            }
          }
          
          if (failed)
            break;
          
          jumboPEStail = todo - frameStart;
          jumboPEStailData = jumboPESdata + frameStart;

          todo = frameStart;

          jumboPESdata[ 4 + 0 ] = (todo - 6) >> 8;
          jumboPESdata[ 4 + 1 ] = (todo - 6) & 0xff;
          
          dolby = true;
store_frame(jumboPESdata, todo, __LINE__);    
          break;
        }

        if (0x80 == (0xf0 & jumboPESdata[ 6 + 3 + h + 0 ]))
        {
          dolby = true;
          break;
        }
        
        if (0xa0 == (0xf0 & jumboPESdata[ 6 + 3 + h + 0 ]))
        {
          pcm = true;
          break;
        }

        for (int i = 6 + 3 + h; i < todo - 2 - 2 - 1; i++)
        {
          if (0x0b == jumboPESdata[ i + 0 ]
              && 0x77 == jumboPESdata[ i + 1 ]
              && 0 != frameSizes[ jumboPESdata[ i + 4 ] ])
          {
            jumboPEStail = todo - i;
            jumboPEStailData = jumboPESdata + i;
          }
        }
      }
      while (false);

      if (pcm
          || (dolby && m_settings.AudioDolbyOn()))
      {
        int done = 0;
        
        while (done < todo)
        {
          int r = PlayCommon(jumboPESdata + done, todo - done, false);
          if (r < 0)
            return r;
          
//          fprintf(stderr, ".");
          
          done += r;
        }

// Don't return done here as the remaining bytes were buffered elsewhere!
//        return done;
      }
    }
    else if (jumboPESsize == Length)
    {
      int todo = jumboPESsize;

      bool dolby = false;
      bool pcm = false;
      
      do
      {
        if (todo < (6 + 3 + 0 + 2))
          break;
        
        if (0x00 != jumboPESdata[ 0 ]
            || 0x00 != jumboPESdata[ 1 ]
            || 0x01 != jumboPESdata[ 2 ]
            || 0xbd != jumboPESdata[ 3 ])
        {
          break;
        }

        int l = jumboPESdata[ 4 ] * 0x0100 + jumboPESdata[ 5 ];
        if (l < (3 + 0 + 2))
          break;

        if (todo < (6 + l))
          break;

        int h = jumboPESdata[ 8 ];
        if (l < (3 + h + 2))
          break;

        if (0x80 == (0xf0 & jumboPESdata[ 6 + 3 + h + 0 ]))
        {
          dolby = true;
          break;
        }
        
        if (0xa0 == (0xf0 & jumboPESdata[ 6 + 3 + h + 0 ]))
        {
          pcm = true;
          break;
        }
      }
      while (false);

      if (dolby || pcm)
      {
        dumpAudio("Audio", jumboPESdata, todo);

        jumboPESsize = 0;
      }

      if (pcm
          || (dolby && m_settings.AudioDolbyOn()))
      {
        int done = 0;
        
        while (done < todo)
        {
          int r = PlayCommon(jumboPESdata + done, todo - done, false);
          if (r < 0)
            return r;
          
//          fprintf(stderr, ".");
          
          done += r;
        }

// Don't return done here as the remaining bytes were buffered elsewhere!
//        return done;
      }
    }

//    fprintf(stderr, "\n");

    return Length;
  }
  
#if APIVERSNUM >= 10338
  uchar *cXineDevice::GrabImage(int &Size, bool Jpeg /* = true */, int Quality /* = -1 */, int SizeX /* = -1 */, int SizeY /* = -1 */)
  {
    const char *const FileName = 0;
#else
  bool cXineDevice::GrabImage(const char *FileName, bool Jpeg /* = true */, int Quality /* = -1 */, int SizeX /* = -1 */, int SizeY /* = -1 */)
  {
    int Size = 0;
#endif
    xfprintf(stderr, "GrabImage ...\n\n");

    if (-1 == Quality)
      Quality = 100;

    if (-1 == SizeX)
      SizeX = m_settings.DefaultGrabSizeX();
 
    if (-1 == SizeY)
      SizeY = m_settings.DefaultGrabSizeY();

    uchar *result = m_xineLib.execFuncGrabImage(FileName, Size, Jpeg, Quality, SizeX, SizeY);

    xfprintf(stderr, result ? "\nGrabImage succeeded.\n" : "\nGrabImage failed.\n");    
    return result;
  }

  int64_t cXineDevice::GetSTC(void)
  {
//    ::fprintf(stderr, "GetSTC: ");
    
    int64_t pts = -1;

    if (!m_xineLib.execFuncGetPTS(pts, 100) || pts < 0)
      pts = cDevice::GetSTC();

//    ::fprintf(stderr, "%lld\n", pts);
    
    return pts;
  }
  
  void cXineDevice::SetVideoFormat(bool VideoFormat16_9)
  {
    xfprintf(stderr, "SetVideoFormat: %d\n", VideoFormat16_9);
    cDevice::SetVideoFormat(VideoFormat16_9); 

    m_is16_9 = VideoFormat16_9;
    m_xineLib.selectNoSignalStream(VideoFormat16_9);
  }

#if APIVERSNUM >= 10707
#if APIVERSNUM >= 10708
  void cXineDevice::GetVideoSize(int &Width, int &Height, double &VideoAspect)
  {
    int x, y, zx, zy;
    m_xineLib.execFuncVideoSize(x, y, Width, Height, zx, zy, &VideoAspect); 
  }

  void cXineDevice::GetOsdSize(int &Width, int &Height, double &PixelAspect)
#else
  void cXineDevice::GetVideoSize(int &Width, int &Height, eVideoAspect &Aspect)
  // function name misleading -- max osd extent shall be returned
#endif
  {
    Width  = m_settings.OsdExtentParams().GetOsdExtentWidth();
    Height = m_settings.OsdExtentParams().GetOsdExtentHeight();
#if APIVERSNUM >= 10708
    PixelAspect = (m_is16_9 ? 16.0 : 4.0) * Height / ((m_is16_9 ? 9.0 : 3.0) * Width);
//fprintf(stderr, "GetOsdSize(%d, %d, %lg)\n", Width, Height, PixelAspect);
#else
    Aspect = m_is16_9 ? va16_9 : va4_3;
#endif
  }
#endif

  static bool firstCallToSetVolume = true;
  static void switchSkin(const bool restore);
  
  void cXineDevice::SetVolumeDevice(int Volume)
  {
    if (firstCallToSetVolume)
    {
      firstCallToSetVolume = false;

      if (m_settings.ShallSwitchSkin())
        switchSkin(false);
    }
    
    xfprintf(stderr, "SetVolumeDevice: %d\n", Volume);
    m_xineLib.execFuncSetVolume(Volume);
  }
  
#if APIVERSNUM < 10307
  cOsdBase *cXineDevice::NewOsd(int w, int h, int x, int y)
#elif APIVERSNUM < 10509    
  cOsd *cXineDevice::NewOsd(int w, int h, int x, int y)
#else
  cOsd *cXineDevice::NewOsd(int w, int h, int x, int y, uint Level, bool TrueColor /* = false */)
#endif    
  {
//    ::fprintf(stderr, "NewOsd ---: %s\n", ::ctime(&(const time_t &)::time(0)));
    cXineOsdMutexLock osdLock(&m_osdMutex);
//    ::fprintf(stderr, "NesOsd +++: %s\n", ::ctime(&(const time_t &)::time(0)));
#if APIVERSNUM < 10509    
    if (m_currentOsd)
    {
      esyslog("vdr-xine: new OSD(%d, %d) requested while another OSD is active", x, y);
      xfprintf(stderr, "vdr-xine: new OSD(%d, %d) requested while another OSD is active", x, y);
      return 0;
    }
#endif    
    if (x < 0 || y < 0)
    {
      esyslog("vdr-xine: new OSD(%d, %d) requested with coordinates out of range", x, y);
      xfprintf(stderr, "vdr-xine: new OSD(%d, %d) requested with coordinates out of range", x, y);
    }

    if (w <= 0 || h <= 0)
    {
      w = m_settings.OsdExtentParams().GetOsdExtentWidth();
      h = m_settings.OsdExtentParams().GetOsdExtentHeight();
    }

#if APIVERSNUM < 10509    
    m_currentOsd = new cXineOsd(*this, w, h, x, y);
#else
    return new cXineOsd(*this, w, h, x, y, Level);
#endif    

    return m_currentOsd;
  }

#if APIVERSNUM < 10307
  void cXineDevice::OnFreeOsd(cOsdBase *const osd)
#else    
  void cXineDevice::OnFreeOsd(cOsd *const osd)
#endif    
  {
#if APIVERSNUM < 10509    
    cXineOsdMutexLock osdLock(&m_osdMutex);
    
    assert(osd == m_currentOsd);

    m_currentOsd = 0;
#endif    
  }

  bool cXineDevice::ChangeCurrentOsd(cXineOsd *const osd, bool on)
  {
    cXineOsdMutexLock osdLock(&m_osdMutex);

    if (m_currentOsd)
    {
      if (on)
      {
        esyslog("vdr-xine: OSD activation requested while another OSD is active -- ignoring request");
        xfprintf(stderr, "vdr-xine: OSD activation requested while another OSD is active -- ignoring request");
        return false;
      }
      else if (m_currentOsd != osd)
      {
        esyslog("vdr-xine: OSD deactivation requested for an OSD which is not active -- ignoring request");
        xfprintf(stderr, "vdr-xine: OSD deactivation requested for an OSD which is not active -- ignoring request");
        return false;
      }
      else
      {
        m_currentOsd = 0;
      }
    }
    else
    {
      if (!on)
      {
        esyslog("vdr-xine: OSD deactivation requested while no OSD is active -- ignoring request");
        xfprintf(stderr, "vdr-xine: OSD deactivation requested while no OSD is active -- ignoring request");
        return false;
      }
      else
      {
        m_currentOsd = osd;
      }
    }

    return true;
  }


  
  static cDevice *originalPrimaryDevice = 0;
  
#if APIVERSNUM >= 10307
  
  void cXineDevice::MakePrimaryDevice(bool On)
  {
    cDevice::MakePrimaryDevice(On);

    xfprintf(stderr, "-------------------------\n");
    xfprintf(stderr, "MakePrimaryDevice: %d\n", On);
    xfprintf(stderr, "=========================\n");

    if (On)
      new cXineOsdProvider(*this);
    else
      cOsdProvider::Shutdown();
    
    originalPrimaryDevice = 0;
  }
  
#endif    



#if APIVERSNUM >= 10320
  static cSkin *origSkin = 0;
#endif
  
  static void switchSkin(const bool restore)
  {
#if APIVERSNUM >= 10320
    if (restore)
    {
      Skins.SetCurrent(origSkin->Name());
      cThemes::Load(Skins.Current()->Name(), Setup.OSDTheme, Skins.Current()->Theme());
    }
    else
    {
      origSkin = Skins.Current();
      Skins.SetCurrent("curses");
      cThemes::Load(Skins.Current()->Name(), Setup.OSDTheme, Skins.Current()->Theme());
    }
#else
#warning vdr-xine: switching skins is only available for VDR versions >= 1.3.20
    isyslog("vdr-xine: switching skins is only available for VDR versions >= 1.3.20");
#endif
  }
  
  void cXineDevice::reshowCurrentOsd(const bool dontOptimize /* = true */, const int frameLeft /* = -1 */, const int frameTop /* = -1 */, const int frameWidth /* = -1 */, const int frameHeight /* = -1 */, const int frameZoomX /* = -1 */, const int frameZoomY /* = -1 */, const bool unlockOsdUpdate /* = false */)
  {
    cXineOsdMutexLock osdLock(&m_osdMutex);
    
    if (unlockOsdUpdate)
      m_osdUpdateLocked = false;

    if (m_currentOsd)
      m_currentOsd->ReshowCurrentOsd(dontOptimize, frameLeft, frameTop, frameWidth, frameHeight, frameZoomX, frameZoomY);
  }

  void cXineDevice::LockOsdUpdate()
  {
    cXineOsdMutexLock osdLock(&m_osdMutex);

    m_osdUpdateLocked = true;
  }

  bool cXineDevice::OsdUpdateLocked()
  {
    cXineOsdMutexLock osdLock(&m_osdMutex);

    return m_osdUpdateLocked;
  }

  void cXineDevice::mainMenuTrampoline()
  {
#if APIVERSNUM >= 10332
    cMutexLock switchPrimaryDeviceLock(&m_switchPrimaryDeviceMutex);
    if (m_switchPrimaryDeviceDeviceNo < 0)
      return;

    cControl::Shutdown();

    if (m_switchPrimaryDeviceDeviceNo == (1 + DeviceNumber()))
    {  
      char *msg = 0;
      ::asprintf(&msg, tr("Switching primary DVB to %s..."), m_plugin->Name());

      Skins.Message(mtInfo, msg);
      ::free(msg);
    }

    SetPrimaryDevice(m_switchPrimaryDeviceDeviceNo);

    if (m_switchPrimaryDeviceDeviceNo != (1 + DeviceNumber()))
    {
      char *msg = 0;
      ::asprintf(&msg, tr("Switched primary DVB back from %s"), m_plugin->Name());

      Skins.Message(mtInfo, msg);
      ::free(msg);
    }

    m_switchPrimaryDeviceDeviceNo = -1;

    m_switchPrimaryDeviceCond.Broadcast();
#endif
  }

  void cXineDevice::switchPrimaryDevice(const int deviceNo, const bool waitForExecution)
  {
#if APIVERSNUM >= 10332
#if APIVERSNUM < 10347
    while (cRemote::HasKeys())
      cCondWait::SleepMs(10);
#endif

    cMutexLock switchPrimaryDeviceLock(&m_switchPrimaryDeviceMutex);
    m_switchPrimaryDeviceDeviceNo = deviceNo;

#if APIVERSNUM < 10347
    cRemote::CallPlugin(m_plugin->Name());
#endif

    if (waitForExecution)
      m_switchPrimaryDeviceCond.Wait(m_switchPrimaryDeviceMutex);
#else
#warning vdr-xine: switching primary device is no longer supported for VDR versions < 1.3.32 
    isyslog("vdr-xine: switching primary device is no longer supported for VDR versions < 1.3.32");
#endif
  }

  void cXineDevice::OnClientConnect()
  {
    reshowCurrentOsd();

    if (m_settings.LiveTV())
    {
      cMutexLock lock(&softStartMutex);
      softStartTrigger = sstNormal;
    }

    if (m_settings.AutoPrimaryDevice())
    {
      cDevice *primaryDevice = cDevice::PrimaryDevice();
      if (this != primaryDevice)
        switchPrimaryDevice(1 + DeviceNumber(), true);

      originalPrimaryDevice = primaryDevice;
    }

    if (m_settings.ShallSwitchSkin())
      switchSkin(true);
  }

  void cXineDevice::OnClientDisconnect()
  {
    if (m_settings.ShallSwitchSkin())
      switchSkin(false);

    if (m_settings.AutoPrimaryDevice()
      && originalPrimaryDevice)
    {
      if (this != originalPrimaryDevice)
        switchPrimaryDevice(1 + originalPrimaryDevice->DeviceNumber(), false);
    }
  }
  
  void cXineDevice::ReshowCurrentOSD(const int frameLeft, const int frameTop, const int frameWidth, const int frameHeight, const int frameZoomX, const int frameZoomY, const bool unlockOsdUpdate)
  {
//    ::fprintf(stderr, ">>> cXineDevice::ReshowCurrentOSD()\n");
    reshowCurrentOsd(false, frameLeft, frameTop, frameWidth, frameHeight, frameZoomX, frameZoomY, unlockOsdUpdate);
//    ::fprintf(stderr, "<<< cXineDevice::ReshowCurrentOSD()\n");
  }

  bool cXineDevice::DeviceReplayingOrTransferring()
  {
#if APIVERSNUM >= 10341
    return Replaying() || Transferring();
#else
    return Replaying() /* || Transferring() */;
#endif
  }

  bool cXineDevice::open()
  {
    if (!m_xineLib.Open())
      return false;

    if (m_settings.ShallSwitchSkin())
    {
#if APIVERSNUM >= 10320
      Skins.SetCurrent(Setup.OSDSkin);
      cThemes::Load(Skins.Current()->Name(), Setup.OSDTheme, Skins.Current()->Theme());
#endif
      switchSkin(false);
    }
    
    return true;
  }

  void cXineDevice::close()
  {
    m_xineLib.Close();

    if (m_settings.ShallSwitchSkin())
      switchSkin(true);
  }

  void cXineDevice::Stop()
  {
    if (!theXineDevice)
      return;

    theXineDevice->close();
  }

  cXineDevice::cXineDevice(cPlugin *const plugin, cXineSettings &settings, cXineRemote *remote)
    : cDevice()
    , m_settings(settings)
    , m_osdUpdateLocked(false)
    , m_currentOsd(0)
    , m_spuDecoder(0)
    , m_is16_9(true)
    , m_audioChannel(0)
    , m_plugin(plugin)
    , m_switchPrimaryDeviceDeviceNo(-1)
    , m_xineLib(plugin, settings, m_osdMutex, remote)
  {
    m_xineLib.SetEventSink(this);
  }
  
  cXineDevice::~cXineDevice()
  {
#if APIVERSNUM < 10320
    close();
#endif
    if (m_spuDecoder)
      delete m_spuDecoder;
  }
  
  bool cXineDevice::Create(cPlugin *const plugin, cXineSettings &settings, cXineRemote *remote)
  {
    if (theXineDevice)
      return false;

    theXineDevice = new cXineDevice(plugin, settings, remote);
    
    return 0 != theXineDevice
      && theXineDevice->hasNoSignalStream();
  }

  bool cXineDevice::Open()
  {
    if (!theXineDevice)
      return false;

    return theXineDevice->open();
  }

  cXineDevice *cXineDevice::GetDevice()
  {
    return theXineDevice;
  }

};
