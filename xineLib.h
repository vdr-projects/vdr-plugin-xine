
#ifndef __XINELIB_H
#define __XINELIB_H



#include "xineCommon.h"

#include <vdr/remote.h>

#if APIVERSNUM < 10307

#include <vdr/osdbase.h>

typedef uchar tIndex;
typedef eDvbColor tColor;
typedef pid_t tThreadId;
#include <sys/syscall.h>

#else

#include <vdr/osd.h>

#define  MAXNUMWINDOWS MAXOSDAREAS

#endif

#include <vdr/plugin.h>

#include "xineExternal.h"
#include "xineSettings.h"



namespace PluginXine
{

  class cXineLibEvents
  {
  protected:
    virtual ~cXineLibEvents() {}
  public:
    virtual void OnClientConnect() = 0;
    virtual void OnClientDisconnect() = 0;
    virtual void ReshowCurrentOSD(const int frameLeft, const int frameTop, const int frameWidth, const int frameHeight, const int frameZoomX, const int frameZoomY, const bool unlockOsdUpdate) = 0;
    virtual void LockOsdUpdate() = 0;
    virtual bool OsdUpdateLocked() = 0;
    virtual void DiscontinuityDetected() = 0;
    virtual bool DeviceReplayingOrTransferring() = 0;
  };


  
  class cXineLib;

// This class performs the adaption from Xine events
// to VDR's remote control events.
  class cXineRemote
    : public cRemote
    , private cThread 
  {
  private:
    bool m_active;
    cMutex m_activeMutex;
    cCondVar m_activeCondVar;
    const bool m_remoteOn;
    cXineLib *m_xineLib;
    
    virtual void Action (void);
    bool Ready(void) { return false; };
    bool isConnected();

  public:
    cXineRemote(const bool remoteOn);
    virtual ~cXineRemote();

    void setXineLib(cXineLib *const xineLib);
  };
  
 
  class cXineOsd;
  class cXineSettings;

  class cXineAdapter
  {
  public:
    virtual int X0(void) const = 0;
    virtual int Y0(void) const = 0;
    virtual int Width(void) const = 0;
    virtual int Height(void) const = 0;
    virtual bool Dirty(int &x1, int &y1, int &x2, int &y2) = 0;
    virtual void Clean(void) = 0;
    virtual const tColor *Colors(int &NumColors) const = 0;
    virtual const uint8_t *Data(int x, int y) const = 0;
    virtual int SizeOfPixel() const = 0;
    virtual int SizeOfStride() const = 0;
   };

  class cXineBitmapAdapter : public cXineAdapter
  {
    cBitmap *const m_pBitmap;

  public:
    cXineBitmapAdapter(cBitmap *const pBitmap)
      : m_pBitmap(pBitmap)
    {
    }

    virtual int X0(void) const;
    virtual int Y0(void) const;
    virtual int Width(void) const;
    virtual int Height(void) const;
    virtual bool Dirty(int &x1, int &y1, int &x2, int &y2);
    virtual void Clean(void);
    virtual const tColor *Colors(int &NumColors) const;
    virtual const uint8_t *Data(int x, int y) const;
    virtual int SizeOfPixel() const;
    virtual int SizeOfStride() const;

    operator cXineAdapter *()
    {
      if (m_pBitmap)
        return this;

      return 0;
    }
  };

#if APIVERSNUM >= 10717
  
  class cXinePixmapMemoryAdapter : public cXineAdapter
  {
    cPixmapMemory *const m_pPixmapMemory;

  public:
    cXinePixmapMemoryAdapter(const cPixmapMemory *const pPixmapMemory);
    virtual int X0(void) const;
    virtual int Y0(void) const;
    virtual int Width(void) const;
    virtual int Height(void) const;
    virtual bool Dirty(int &x1, int &y1, int &x2, int &y2);
    virtual void Clean(void);
    virtual const tColor *Colors(int &NumColors) const;
    virtual const uint8_t *Data(int x, int y) const;
    virtual int SizeOfPixel() const;
    virtual int SizeOfStride() const;

    operator cXineAdapter *()
    {
      if (m_pPixmapMemory)
        return this;

      return 0;
    }
  };

#endif
  
  class cXineLib : public cThread
  {
  public:
    enum eNeedsScaling
    {
      no = 0
      , yes
      , shq
    };

    string m_fifoDir;
    string m_fifoNameControl;
    string m_fifoNameResult;
    string m_fifoNameRemote;
    string m_fifoNameStream;
    string m_fifoNameExtControl;
    string m_fifoNameExtResult;
    in_addr_t m_bindIp;
    int m_bindPort;

  private:
    cPlugin *const m_plugin;
    cXineExternal m_external;
    
    void internalPaused(const bool paused);
    cMutex m_pausedMutex;
    cCondVar m_pausedCondVar;
    cMutex m_shutdownMutex;
    cCondVar m_shutdownCondVar;
      
    const cXineSettings &m_settings;

    bool osdUpdateLocked(const char *const funcName);
    bool m_osdWindowVisible[ MAXNUMWINDOWS ];

#if APIVERSNUM >= 10307
    int m_osdWindowVideoLeft[ MAXNUMWINDOWS ];
    int m_osdWindowVideoTop[ MAXNUMWINDOWS ];
    int m_osdWindowVideoWidth[ MAXNUMWINDOWS ];
    int m_osdWindowVideoHeight[ MAXNUMWINDOWS ];
    int m_osdWindowVideoZoomX[ MAXNUMWINDOWS ];
    int m_osdWindowVideoZoomY[ MAXNUMWINDOWS ];
    int m_osdWindowLeft[ MAXNUMWINDOWS ];
    int m_osdWindowTop[ MAXNUMWINDOWS ];
    int m_osdWindowWidth[ MAXNUMWINDOWS ];
    int m_osdWindowHeight[ MAXNUMWINDOWS ];
        
    int m_osdWindowColorsNum[ MAXNUMWINDOWS ];
    tColor m_osdWindowColors[ MAXNUMWINDOWS ][ MAXNUMCOLORS ];

    int m_scaledWindowColorsNum[ MAXNUMWINDOWS ];
    tColor m_scaledWindowColors[ MAXNUMWINDOWS ][ MAXNUMCOLORS ];

    cXineSettings::eOsdMode m_osdWindowMode[ MAXNUMWINDOWS ];
    int m_osdWindowGamma[ MAXNUMWINDOWS ];
    bool m_osdWindowSupportTransparency[ MAXNUMWINDOWS ];

    int m_osdWindowBufferSize[ MAXNUMWINDOWS ];
    uint8_t *m_osdWindowBuffer[ MAXNUMWINDOWS ];

    tArea m_vidWin;
#endif    

    result_query_capabilities_t m_capabilities;

    void cloneBitmap(const int windowNum, cXineAdapter *bitmap, int x1, int y1, int x2, int y2);
    bool bitmapDiffers(const int windowNum, cXineOsd *xineOsd, cXineAdapter *bitmap, int &x1, int &y1, int &x2, int &y2, const int osdX, const int osdY, const int osdW, const int osdH); 
    bool clipBitmap(cXineOsd *xineOsd, cXineAdapter *bitmap, int &x1, int &y1, int &x2, int &y2, const int osdX, const int osdY, const int osdW, const int osdH); 
    
    bool m_osdFlushRequired;

  public:
    int getRemoteFD() const
    {
      return fd_remote;
    }
    
  private:    
    int CreateServerSocket(unsigned short port);
    int SocketAcceptHelper(int fd);

    int fd_fifo0_serv, fd_result_serv, fd_control_serv, fd_remote_serv;
    int fd_fifo0, fd_result, fd_control, fd_remote;

    cMutex m_ioMutex, m_dataMutex, m_disconnectMutex;
    cMutex &m_osdMutex;

    bool m_paused;
    bool m_frozen;
    bool m_ignore;
    bool m_shutdown;

    bool m_muted;
    int m_volume;
    int m_audioChannel;
    bool m_trickSpeedMode;

  public:    
    bool isTrickSpeedMode() const
    {
      return m_trickSpeedMode;
    }

    bool isConnected();
    void disconnect();

    int xwrite(int f, const void *b, int n);
    int xread(int f, void *b, int n);

  private:
    bool m_noSignalStream16x9; 
    char m_noSignalStreamData[2][ 6 + 0xffff ];
    long m_noSignalStreamSize[2];

    bool readNoSignalStream(const int index, const string &suffix);

    cXineLibEvents *m_eventSink;

    eNeedsScaling NeedsScaling(const int maxOsdWidth, const int maxOsdHeight, const int videoWidth, const int videoHeight);

    bool pushOut(const uchar id = 0xff);

  public:
    bool hasNoSignalStream() const
    {
      return m_noSignalStreamSize[0] > 0
        && m_noSignalStreamSize[1] > 0;
    }

    bool SupportsTrueColorOSD() const;

    void selectNoSignalStream(const bool stream16x9)
    {
      m_noSignalStream16x9 = stream16x9;
    }
    
    virtual void Action(void);

    bool checkXineVersion();
    bool checkConnect();

    void enableExternal(const bool enable = true);
    void ExternalStreamFinished();

  public:
    cXineLib(cPlugin *const plugin, const cXineSettings &settings, cMutex &osdMutex, cXineRemote *const remote);
    virtual ~cXineLib();

    void SetEventSink(cXineLibEvents *const eventSink);
    
    bool Open();
    void Close();

    bool Poll(cPoller &Poller, int TimeoutMs = 0, const bool special = false);

#if APIVERSNUM < 10307
    
    bool OpenWindow(cXineOsd *const xineOsd, cWindow *Window, const int maxOsdWidth, const int maxOsdHeight);
    void CommitWindow(cXineOsd *const xineOsd, cWindow *Window, const bool optimize = true);
    void ShowWindow(cXineOsd *const xineOsd, cWindow *Window);
    void HideWindow(cXineOsd *const xineOsd, cWindow *Window, bool Hide);
    void MoveWindow(cXineOsd *const xineOsd, cWindow *Window, int x, int y);
    void CloseWindow(cXineOsd *const xineOsd, cWindow *Window);
    void CloseWindow(cXineOsd *const xineOsd, int Window);

#else

    void sendWindow(const int maxOsdWidth, const int maxOsdHeight, cXineOsd *const xineOsd, const int windowNum, cXineAdapter *bitmap = 0, const int videoLeft = -1, const int videoTop = -1, const int videoWidth = -1, const int videoHeight = -1, const int videoZoomX = -1, const int videoZoomY = -1, const bool dontOptimize = false);
    void SendWindow(const int maxOsdWidth, const int maxOsdHeight, cXineOsd *const xineOsd, const int windowNum, cXineAdapter *bitmap = 0, const int videoLeft = -1, const int videoTop = -1, const int videoWidth = -1, const int videoHeight = -1, const int videoZoomX = -1, const int videoZoomY = -1, const bool dontOptimize = false);
    
    bool execFuncOsdNew(const int maxOsdWidth, const int maxOsdHeight, const eNeedsScaling needsScaling, const int videoLeft, const int videoTop, const int videoWidth, const int videoHeight, int window, int x, int y, int width, int height);
    bool execFuncOsdDrawBitmap(const int maxOsdWidth, const int maxOsdHeight, const eNeedsScaling needsScaling, const int videoWidth, const int videoHeight, cXineOsd *const xineOsd, int window, cXineAdapter *const bitmap, int x, int y, int width, int height, int stride);

    void SetVideoWindow(const int maxOsdWidth, const int maxOsdHeight, tArea vidWin, const bool dontOptimize = false);
    
#endif    
    
    int execFuncStream1(const uchar *Data, int Length);
    int execFuncStream(const uchar *Data, int Length);
    bool execFuncStream0(const uchar *Data, int Length);
    bool execFuncStart();
    bool execFuncEnd();
    bool execFuncWait(const bool id = false);
    bool execFuncSetup();
    bool execFuncDiscontinuity();

    bool execFuncOsdFlush();
    bool execFuncOsdNew(int window, int x, int y, int width, int height, int w_ref, int h_ref);
    bool execFuncOsdFree(int window);
    bool execFuncOsdShow(int window);
    bool execFuncOsdHide(int window);
    bool execFuncOsdSetPosition(int window, int x, int y);
    bool execFuncOsdDrawBitmap(int window, const uint8_t *const bitmap, const int sizeOfPixel, int x, int y, int width, int height, int stride, const tColor *const colors);

#if APIVERSNUM < 10307
    bool execFuncSetColor(int window, int index, eDvbColor color);
    bool execFuncSetColor(int window, int index, eDvbColor *const colors, int numColors);
    eDvbColor filterAlpha(eDvbColor color);
#else
    bool execFuncSetColor(int window, int index, tColor color);
    bool execFuncSetColor(int window, int index, tColor *const colors, int numColors);
    tColor filterAlpha(tColor color);
#endif
    
    bool execFuncSetColor(int window, int index, int numColors, uint32_t *const colors);

    bool execFuncMute(bool mute = true);
    bool execFuncClear(int n);
    bool execFuncResetAudio();
    bool execFuncSelectAudio(int channel);
    bool execFuncFirstFrame();
    bool execFuncStillFrame();
    bool execFuncFlush(int TimeoutMs = -1, const bool justWait = false);
    bool execFuncSetVolume(int volume);
    bool execFuncSetSpeed(double speed);
    bool execFuncTrickSpeedMode(bool on);
    bool execFuncDelay(int usDelay);
    bool execFuncMetronom(int64_t pts, uint32_t flags = 0);
    bool execFuncNop();
    bool execFuncSetPrebuffer(int frames);
    bool execFuncGetPTS(int64_t &pts, const int TimeoutMs = 0, bool *const pQueued = 0);
    bool execFuncVideoSize(int &videoLeft, int &videoTop, int &videoWidth, int &videoHeight, int &videoZoomX, int &videoZoomY, double *const pVideoAspect = 0);

    bool execFuncGetVersion(int32_t &version);
    bool execFuncQueryCapabilities(result_query_capabilities_t &capabilities);

    uchar *execFuncGrabImage(const char *FileName, int &Size, bool Jpeg, int Quality, int SizeX, int SizeY);

    bool execFuncSetVideoWindow(int x, int y, int w, int h, int wRef, int hRef);

    bool execFuncPlayExternal(const char *const fileName = 0);

    bool showNoSignal();
    
    void pause(bool doPause = true);
    void freeze(bool doFreeze = true);
    void ignore(bool doIgnore = true);

    void flush();

    void LockOsdUpdate();

    void ReshowCurrentOSD(const int frameLeft, const int frameTop, const int frameWidth, const int frameHeight, const int frameZoomX, const int frameZoomY, const bool unlockOsdUpdate = false);

    void ReshowCurrentOSD(const bool unlockOsdUpdate = false)
    {
      ReshowCurrentOSD(-1, -1, -1, -1, -1, -1, unlockOsdUpdate);
    }

    void DiscontinuityDetected();
  };

};



#endif //__XINELIB_H
