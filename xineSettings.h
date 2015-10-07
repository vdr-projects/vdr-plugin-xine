
#ifndef __XINESETTINGS_H
#define __XINESETTINGS_H



#include "xineCommon.h"



namespace PluginXine
{

  class cXineSetupPage;
  
  class cXineSettings
  {
  public:
    enum eUsageMode
    {
      modeLiveTV
      , modeReplay
    };

    enum eOsdMode
    {
      osdOverlay
      , osdBlendClipped
      , osdBlendScaledLQ
      , osdBlendScaledHQ
      , osdBlendScaledSHQ
      , osdBlendScaledAuto
    };

    enum eAudioMode
    {
      audioDolbyOff
      , audioDolbyOn
    };

    enum eVolumeMode
    {
      volumeIgnore
      , volumeChangeHW
      , volumeChangeSW
    };

    enum eMuteMode
    {
      muteIgnore
      , muteExecute
      , muteSimulate
    };

    enum eOsdExtentWidth
    {
      osdExtentWidthMin       = 320
      , osdExtentWidthDefault = 720
      , osdExtentWidthMax     = 1920
    };

    enum eOsdExtentHeight
    {
      osdExtentHeightMin       = 240
      , osdExtentHeightDefault = 576
      , osdExtentHeightMax     = 1080
    };

    enum eMonitorGamma
    {
      monitorGammaBase = 100
      , monitorGammaMin = 100
      , monitorGammaDefault = 123
      , monitorGammaMax = 250
    };
    
    enum eImage
    {
      image4_3 = 0
      , image16_9 = 1
    };
 
    enum eImageZoom
    {
      imageZoomBase = 100
      , imageZoomMin = 25 
      , imageZoomDefault = 100
      , imageZoomMax = 400
    };

    enum eAutoPrimaryDeviceMode
    {
      autoPrimaryDeviceOff
      , autoPrimaryDeviceOn
    };

    enum eTransparencyMode
    {
      transparencyOff
      , transparencyOn
    };

    enum eMonitoringMode
    {
      monitoringOnce
      , monitoringContinuous
    };

    enum eInteractWithEitScannerMode
    {
      interactWithEitScannerOff
      , interactWithEitScannerOn
    };

  private:
    bool m_switchSkin;
    bool m_beQuiet;
    int m_defaultGrabSizeX;
    int m_defaultGrabSizeY;
    
    eOsdMode m_osdMode;
    eUsageMode m_usageMode /* , m_usageModeDefault */;
    eAudioMode m_audioMode;
    eVolumeMode m_volumeMode;
    eMuteMode m_muteMode;
    eMonitorGamma m_crtGamma;
    eAutoPrimaryDeviceMode m_autoPrimaryDeviceMode;
    eTransparencyMode m_transparencyMode;
    eInteractWithEitScannerMode m_interactWithEitScannerMode;

    template <class T>
      static T clip(T a, T x, T b)
    {
      if (x < a)
        return a;

      if (x > b)
        return b;

      return x;
    }

    class cModeParams
    {
      bool SetupParse(const char *optionName, int &optionValue, const char *Name, const char *Value);
      bool SetupParse(const char *optionName, eMonitoringMode &optionValue, const char *Name, const char *Value);
      
    public:
      cModeParams();
      
      int m_prebufferFramesVideoSD;
      int m_prebufferFramesVideoHD;
      int m_prebufferFramesAudio;
      int m_prebufferHysteresis;
      int m_monitoringDuration;
      eMonitoringMode m_monitoringMode;

      bool SetupParse(const char *prefix, const char *Name, const char *Value);
      bool MonitoringContinuous() const;
    }
    m_modeParams[ 2 ];

    class cZoomParams
    {
      bool SetupParse(const char *optionName, eImageZoom &optionValue, const char *Name, const char *Value);
      
    public:
      cZoomParams();
      
      eImageZoom m_zoomX;
      eImageZoom m_zoomY;

      eImageZoom GetZoomX() const
      {
        return clip(imageZoomMin, m_zoomX, imageZoomMax);
      }

      eImageZoom GetZoomY() const
      {
        return clip(imageZoomMin, m_zoomY, imageZoomMax);
      }

      bool SetupParse(const char *prefix, const char *Name, const char *Value);

      bool operator !=(const cZoomParams &rhs) const;
    }
    m_zoomParams[ 2 ];

    class cOsdExtentParams
    {
      bool SetupParse(const char *optionName, eOsdExtentWidth &optionValue, const char *Name, const char *Value);
      bool SetupParse(const char *optionName, eOsdExtentHeight &optionValue, const char *Name, const char *Value);
      
    public:
      cOsdExtentParams();
      
      eOsdExtentWidth m_osdExtentWidth;
      eOsdExtentHeight m_osdExtentHeight;

      eOsdExtentWidth GetOsdExtentWidth() const
      {
        return clip(osdExtentWidthMin, m_osdExtentWidth, osdExtentWidthMax);
      }
      
      eOsdExtentHeight GetOsdExtentHeight() const
      {
        return clip(osdExtentHeightMin, m_osdExtentHeight, osdExtentHeightMax);
      }

      bool SetupParse(const char *prefix, const char *Name, const char *Value);

      bool operator !=(const cOsdExtentParams &rhs) const;
    }
    m_osdExtentParams;

    bool SetupParse(const char *optionName, eUsageMode &optionValue, const char *Name, const char *Value);
    bool SetupParse(const char *optionName, eOsdMode &optionValue, const char *Name, const char *Value);
    bool SetupParse(const char *optionName, eAudioMode &optionValue, const char *Name, const char *Value);
    bool SetupParse(const char *optionName, eVolumeMode &optionValue, const char *Name, const char *Value);
    bool SetupParse(const char *optionName, eMuteMode &optionValue, const char *Name, const char *Value);
    bool SetupParse(const char *optionName, eMonitorGamma &optionValue, const char *Name, const char *Value);
    bool SetupParse(const char *optionName, eAutoPrimaryDeviceMode &optionValue, const char *Name, const char *Value);
    bool SetupParse(const char *optionName, eTransparencyMode &optionValue, const char *Name, const char *Value);
    bool SetupParse(const char *optionName, eInteractWithEitScannerMode &optionValue, const char *Name, const char *Value);

  public:
    cXineSettings();
    
    bool SetupParse(const char *Name, const char *Value);

    const cModeParams *GetModeParams() const;

    eOsdMode OsdMode() const;
    bool AudioDolbyOn() const;
    eVolumeMode VolumeMode() const;
    bool SupportTransparency() const;
    eMuteMode MuteMode() const;

    void Create(cXineSetupPage *const setupPage);
    void Store(cXineSetupPage *const setupPage);

    void SelectReplayPrebufferMode(const bool select = true);
    
    void TogglePrebufferMode();
    const char *GetMainMenuEntry();

    bool LiveTV() const;
    bool AutoPrimaryDevice() const;
    bool InteractWithEitScanner() const;
    
    bool setupDiffers(const cXineSettings &rhs) const
    {
      return m_osdMode != rhs.m_osdMode
        || m_transparencyMode != rhs.m_transparencyMode
        || m_volumeMode != rhs.m_volumeMode
        || m_muteMode != rhs.m_muteMode
        || m_zoomParams[ image4_3 ] != rhs.m_zoomParams[ image4_3 ]
        || m_zoomParams[ image16_9 ] != rhs.m_zoomParams[ image16_9 ]
        || m_osdExtentParams != rhs.m_osdExtentParams;
    }

    int GetCrtGamma() const
    {
      return clip(monitorGammaMin, m_crtGamma, monitorGammaMax);
    }

    const cZoomParams &ZoomParams(eImage image) const
    {
      return m_zoomParams[ image ];
    }

    const cOsdExtentParams &OsdExtentParams() const
    {
      return m_osdExtentParams;
    }

    void SetSwitchSkin(const bool switchSkin);
    bool ShallSwitchSkin() const;

    void SetBeQuiet(const bool beQuiet);
    bool ShallBeQuiet() const;

    void SetDefaultGrabSizeX(const int defaultGrabSizeX);
    int DefaultGrabSizeX() const; 

    void SetDefaultGrabSizeY(const int defaultGrabSizeY);
    int DefaultGrabSizeY() const; 
  };

};



#endif //__XINESETTINGS_H
