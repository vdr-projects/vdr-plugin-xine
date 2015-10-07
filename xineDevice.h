
#ifndef __XINEDEVICE_H
#define __XINEDEVICE_H



#include "xineCommon.h"

#include <vdr/dvbspu.h>
#include <vdr/device.h>

#include "xineLib.h"



class cPlugin;

namespace PluginXine
{
  class cXineSettings;
  class cXineOsd;
  class cXineDevice;
  
  class cXineSpuDecoder : public cDvbSpuDecoder
  {
    cXineDevice *const m_xineDevice;

    void ptsAdjust(uint32_t &pts);
    
  public:
    cXineSpuDecoder(cXineDevice *const xineDevice)
      : cDvbSpuDecoder()
      , m_xineDevice(xineDevice)
    {
    }
    
    virtual int setTime(uint32_t pts);
  };
  
  class cXineDevice : public cDevice, public cXineLibEvents
  {
    cXineSettings &m_settings;

    bool m_osdUpdateLocked;
    cXineOsd *m_currentOsd;
    cXineSpuDecoder *m_spuDecoder;

    virtual bool HasDecoder(void) const;
    virtual cSpuDecoder *GetSpuDecoder(void);
    virtual bool CanReplay(void) const;
    virtual bool SetPlayMode(ePlayMode PlayMode);
    virtual bool HasIBPTrickSpeed(void);
    virtual void TrickSpeed(int Speed, bool IBP);
    virtual void TrickSpeed(int Speed);
    virtual void Clear(void);
    virtual void Play(void);
    virtual void Freeze(void);
    virtual void Mute(void);
    virtual void StillPicture(const uchar *Data, int Length);
    virtual bool Poll(cPoller &Poller, int TimeoutMs = 0);
    virtual bool Flush(int TimeoutMs = 0);
    virtual int PlayTs(const uchar *Data, int Length, bool VideoOnly = false);
    virtual int PlayTsImpl(const uchar *Data, int Length, bool VideoOnly = false);
    virtual int PlaySingleTs(const uchar *Data, int Length, bool VideoOnly = false);
    virtual void PlayTsTrampoline(const uchar *Data, int Length, bool VideoOnly = false);
    virtual int PlayTsVideo(const uchar *Data, int Length);
    virtual int PlayTsAudio(const uchar *Data, int Length);
    virtual int PlayVideo(const uchar *Data, int Length);
    int PlayVideo1(const uchar *Data, int Length, const bool stillImageData);
    int PlayVideo2(const uchar *Data, int Length, const bool stillImageData);
    int PlayVideo3(const uchar *Data, int Length, const bool stillImageData);
    int PlayAudio2(const uchar *Data, int Length);
    int PlayAudio3(const uchar *Data, int Length);
    int PlayCommon(const uchar *Data, int Length, const bool stillImageData);
    int PlayCommon1(const uchar *Data, int Length, int64_t ptsForce);
    int PlayCommon2(const uchar *Data, int Length, int64_t ptsForce);
    int PlayCommon3(const uchar *Data, int Length, int64_t ptsForce);
    
#if APIVERSNUM >= 10338
    virtual uchar *GrabImage(int &Size, bool Jpeg = true, int Quality = -1, int SizeX = -1, int SizeY = -1);
#else
    virtual bool GrabImage(const char *FileName, bool Jpeg = true, int Quality = -1, int SizeX = -1, int SizeY = -1);
#endif

    bool m_is16_9;
    virtual void SetVideoFormat(bool VideoFormat16_9);
    virtual void SetVolumeDevice(int Volume);
#if APIVERSNUM >= 10707
#if APIVERSNUM >= 10708
    virtual void GetVideoSize(int &Width, int &Height, double &VideoAspect);
    virtual void GetOsdSize(int &Width, int &Height, double &PixelAspect);
#else
    virtual void GetVideoSize(int &Width, int &Height, eVideoAspect &Aspect);
#endif
#endif

#if APIVERSNUM >= 10307
    virtual void MakePrimaryDevice(bool On);
#endif

    int m_audioChannel;

#if APIVERSNUM < 10318
    virtual void PlayAudio(const uchar *Data, int Length);
#else
    virtual int GetAudioChannelDevice(void);
    virtual void SetAudioChannelDevice(int AudioChannel);
    virtual void SetDigitalAudioDevice(bool On);
#if APIVERSNUM < 10342
    virtual int PlayAudio(const uchar *Data, int Length);
#else
    virtual int PlayAudio(const uchar *Data, int Length, uchar Id);
#endif
#endif

    int PlayAudioCommon(const uchar *Data, int Length);
    
    bool open();
    void close();

    int PushOut();
    void initStream();
    void reshowCurrentOsd(const bool dontOptimize = true, const int frameLeft = -1, const int frameTop = -1, const int frameWidth = -1, const int frameHeight = -1, const int frameZoomX = -1, const int frameZoomY = -1, const bool unlockOsdUpdate = false);
    virtual void OnClientConnect();
    virtual void OnClientDisconnect();
    virtual void ReshowCurrentOSD(const int frameLeft, const int frameTop, const int frameWidth, const int frameHeight, const int frameZoomX, const int frameZoomY, const bool unlockOsdUpdate);
    virtual void LockOsdUpdate();
    virtual bool OsdUpdateLocked();
    virtual void DiscontinuityDetected();
    virtual bool DeviceReplayingOrTransferring();
    
    cPlugin *const m_plugin;
    int m_switchPrimaryDeviceDeviceNo;
    cMutex m_switchPrimaryDeviceMutex;
    cCondVar m_switchPrimaryDeviceCond;
    void switchPrimaryDevice(const int deviceNo, const bool waitForExecution);
    void mainMenuTrampoline();
    cMutex m_playTsMutex;

  public:
    const cXineSettings &Settings() const
    {
      return m_settings;
    }

    virtual int64_t GetSTC(void);
    
#if APIVERSNUM < 10307
    virtual cOsdBase *NewOsd(int w, int h, int x, int y);
#elif APIVERSNUM < 10509
    virtual cOsd *NewOsd(int w, int h, int x, int y);
#else    
    virtual cOsd *NewOsd(int w, int h, int x, int y, uint Level, bool TrueColor = false);
#endif
    bool ChangeCurrentOsd(cXineOsd *osd, bool on);
    
    cXineDevice(cPlugin *const plugin, cXineSettings &settings, cXineRemote *remote);
    virtual ~cXineDevice();

#if APIVERSNUM < 10307
    void OnFreeOsd(cOsdBase *const osd);
#else    
    void OnFreeOsd(cOsd *const osd);
#endif

    cXineLib m_xineLib;
    cMutex m_osdMutex;

    cMutex m_pmMutex;
    cCondVar m_pmCondVar;
    
    static bool Create(cPlugin *const plugin, cXineSettings &settings, cXineRemote *remote);
    static bool Open();
    static void Stop();
    static cXineDevice *GetDevice();

    static void MainMenuTrampoline()
    {
      GetDevice()->mainMenuTrampoline();
    }

    bool hasNoSignalStream() const
    {
      return m_xineLib.hasNoSignalStream();
    }

    bool SupportsTrueColorOSD() const
    {
      return m_xineLib.SupportsTrueColorOSD();
    }
  };

};



#endif //__XINEDEVICE_H
