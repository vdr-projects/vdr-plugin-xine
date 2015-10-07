
#include "xineCommon.h"

#include <vdr/interface.h>
#include <vdr/i18n.h>

#include "xineSettings.h"
#include "xineSetupPage.h"
#include "xineDevice.h"



namespace PluginXine
{

  cXineSettings::cModeParams::cModeParams()
    : m_prebufferFramesVideoSD(4)
    , m_prebufferFramesVideoHD(4)
    , m_prebufferFramesAudio(4)
    , m_prebufferHysteresis(4)
    , m_monitoringDuration(10)
    , m_monitoringMode(monitoringOnce)
  {
  }
  
  bool cXineSettings::cModeParams::SetupParse(const char *optionName, int &optionValue, const char *Name, const char *Value)
  {
    if (0 != ::strcasecmp(optionName, Name))
      return false;

    char c;
    int v = 0;    
    if (1 != ::sscanf(Value, "%d%c", &v, &c))
      return false;

    if (0 <= v && v <= 100)
    {
      optionValue = v;
      return true;
    }   
    
    return false;
  }

  bool cXineSettings::cModeParams::SetupParse(const char *optionName, eMonitoringMode &optionValue, const char *Name, const char *Value)
  {
    if (0 != ::strcasecmp(optionName, Name))
      return false;

    if (0 == ::strcasecmp("monitoringOnce", Value))
    {
      optionValue = monitoringOnce;
      return true;
    }

    if (0 == ::strcasecmp("monitoringContinuous", Value))
    {
      optionValue = monitoringContinuous;
      return true;
    }

    return false;
  }
 
  bool cXineSettings::cModeParams::SetupParse(const char *prefix, const char *Name, const char *Value)
  {
    const int prefixLen = ::strlen(prefix);
    
    if (0 != ::strncasecmp(prefix, Name, prefixLen))
      return false;

    Name += prefixLen;
    
    if (SetupParse("prebufferFrames", m_prebufferFramesVideoSD, Name, Value)
      && SetupParse("prebufferFrames", m_prebufferFramesVideoHD, Name, Value)
      && SetupParse("prebufferFrames", m_prebufferFramesAudio, Name, Value))
    {
      return true;
    }

    if (SetupParse("prebufferFramesVideoSD", m_prebufferFramesVideoSD, Name, Value))
      return true;

    if (SetupParse("prebufferFramesVideoHD", m_prebufferFramesVideoHD, Name, Value))
      return true;

    if (SetupParse("prebufferFramesAudio", m_prebufferFramesAudio, Name, Value))
      return true;

    if (SetupParse("prebufferHysteresis", m_prebufferHysteresis, Name, Value))
      return true;

    if (SetupParse("monitoringDuration", m_monitoringDuration, Name, Value))
      return true;
 
    if (SetupParse("monitoringMode", m_monitoringMode, Name, Value))
      return true;

    return false;
  }

  bool cXineSettings::cModeParams::MonitoringContinuous() const
  {
    return monitoringContinuous == m_monitoringMode;
  }

  
  cXineSettings::cZoomParams::cZoomParams()
    : m_zoomX(imageZoomDefault)
    , m_zoomY(imageZoomDefault)
  {
  }
  
  bool cXineSettings::cZoomParams::SetupParse(const char *optionName, eImageZoom &optionValue, const char *Name, const char *Value)
  {
    if (0 != ::strcasecmp(optionName, Name))
      return false;

    char c;
    int v = 0;    
    if (1 != ::sscanf(Value, "%d%c", &v, &c))
      return false;

    if (imageZoomMin <= v && v <= imageZoomMax)
    {
      optionValue = (eImageZoom)v;
      return true;
    }   
    
    return false;
  }
 
  bool cXineSettings::cZoomParams::SetupParse(const char *prefix, const char *Name, const char *Value)
  {
    const int prefixLen = ::strlen(prefix);
    
    if (0 != ::strncasecmp(prefix, Name, prefixLen))
      return false;

    Name += prefixLen;
    
    if (SetupParse("zoomX", m_zoomX, Name, Value))
      return true;

    if (SetupParse("zoomY", m_zoomY, Name, Value))
      return true;

    return false;
  }

  bool cXineSettings::cZoomParams::operator !=(const cXineSettings::cZoomParams &rhs) const
  {
    return GetZoomX() != rhs.GetZoomX()
      || GetZoomY() != rhs.GetZoomY();
  }

  
  cXineSettings::cOsdExtentParams::cOsdExtentParams()
    : m_osdExtentWidth(osdExtentWidthDefault)
    , m_osdExtentHeight(osdExtentHeightDefault)
  {
  }
  
  bool cXineSettings::cOsdExtentParams::SetupParse(const char *optionName, eOsdExtentWidth &optionValue, const char *Name, const char *Value)
  {
    if (0 != ::strcasecmp(optionName, Name))
      return false;

    char c;
    int v = 0;    
    if (1 != ::sscanf(Value, "%d%c", &v, &c))
      return false;

    if (osdExtentWidthMin <= v && v <= osdExtentWidthMax)
    {
      optionValue = (eOsdExtentWidth)v;
      return true;
    }   
    
    return false;
  }
 
  bool cXineSettings::cOsdExtentParams::SetupParse(const char *optionName, eOsdExtentHeight &optionValue, const char *Name, const char *Value)
  {
    if (0 != ::strcasecmp(optionName, Name))
      return false;

    char c;
    int v = 0;    
    if (1 != ::sscanf(Value, "%d%c", &v, &c))
      return false;

    if (osdExtentHeightMin <= v && v <= osdExtentHeightMax)
    {
      optionValue = (eOsdExtentHeight)v;
      return true;
    }   
    
    return false;
  }
   bool cXineSettings::cOsdExtentParams::SetupParse(const char *prefix, const char *Name, const char *Value)
  {
    const int prefixLen = ::strlen(prefix);
    
    if (0 != ::strncasecmp(prefix, Name, prefixLen))
      return false;

    Name += prefixLen;
    
    if (SetupParse("X", m_osdExtentWidth, Name, Value))
      return true;

    if (SetupParse("Y", m_osdExtentHeight, Name, Value))
      return true;

    return false;
  }

  bool cXineSettings::cOsdExtentParams::operator !=(const cXineSettings::cOsdExtentParams &rhs) const
  {
    return GetOsdExtentWidth() != rhs.GetOsdExtentWidth()
      || GetOsdExtentHeight() != rhs.GetOsdExtentHeight();
  }

  
  
  cXineSettings::cXineSettings()
    : m_switchSkin(false)
    , m_beQuiet(false)
    , m_defaultGrabSizeX(720)
    , m_defaultGrabSizeY(576)
    , m_osdMode(osdBlendScaledAuto)
    , m_usageMode(modeLiveTV)
//    , m_usageModeDefault(modeLiveTV)
    , m_audioMode(audioDolbyOff)
    , m_volumeMode(volumeChangeHW)
    , m_muteMode(muteSimulate)
    , m_crtGamma(monitorGammaDefault)
    , m_autoPrimaryDeviceMode(autoPrimaryDeviceOn)
    , m_transparencyMode(transparencyOn)
    , m_interactWithEitScannerMode(interactWithEitScannerOff)
  {
//    m_modeParams[ modeLiveTV ].m_prebufferFrames = 4; //2 * 15 + 1;
//    m_modeParams[ modeLiveTV ].m_prebufferHysteresis = 4; //2 * 15 + 1;
  }

  bool cXineSettings::SetupParse(const char *optionName, eUsageMode &optionValue, const char *Name, const char *Value)
  {
    if (0 != ::strcasecmp(optionName, Name))
      return false;

    if (0 == ::strcasecmp("modeLiveTV", Value))
    {
      optionValue = modeLiveTV;
      return true;
    }

    if (0 == ::strcasecmp("modeReplay", Value))
    {
      optionValue = modeReplay;
      return true;
    }

    return false;
  }
  
  bool cXineSettings::SetupParse(const char *optionName, eVolumeMode &optionValue, const char *Name, const char *Value)
  {
    if (0 != ::strcasecmp(optionName, Name))
      return false;

    if (0 == ::strcasecmp("volumeChangeHW", Value))
    {
      optionValue = volumeChangeHW;
      return true;
    }

    if (0 == ::strcasecmp("volumeChangeSW", Value))
    {
      optionValue = volumeChangeSW;
      return true;
    }

    if (0 == ::strcasecmp("volumeIgnore", Value))
    {
      optionValue = volumeIgnore;
      return true;
    }

    // backward compatibilty
    if (0 == ::strcasecmp("volumeChange", Value))
    {
      optionValue = volumeChangeHW;
      return true;
    }

    if (0 == ::strcasecmp("volumeDontTouch", Value))
    {
      optionValue = volumeIgnore;
      return true;
    }

    return false;
  }
  
  bool cXineSettings::SetupParse(const char *optionName, eMuteMode &optionValue, const char *Name, const char *Value)
  {
    if (0 != ::strcasecmp(optionName, Name))
      return false;

    if (0 == ::strcasecmp("muteExecute", Value))
    {
      optionValue = muteExecute;
      return true;
    }

    if (0 == ::strcasecmp("muteSimulate", Value))
    {
      optionValue = muteSimulate;
      return true;
    }

    if (0 == ::strcasecmp("muteIgnore", Value))
    {
      optionValue = muteIgnore;
      return true;
    }

    return false;
  }
  
  bool cXineSettings::SetupParse(const char *optionName, eOsdMode &optionValue, const char *Name, const char *Value)
  {
    if (0 != ::strcasecmp(optionName, Name))
      return false;

    if (0 == ::strcasecmp("osdOverlay", Value))
    {
      optionValue = osdOverlay;
      return true;
    }

    if (0 == ::strcasecmp("osdBlendClipped", Value))
    {
      optionValue = osdBlendClipped;
      return true;
    }

    if (0 == ::strcasecmp("osdBlendScaledLQ", Value))
    {
      optionValue = osdBlendScaledLQ;
      return true;
    }

    if (0 == ::strcasecmp("osdBlendScaledHQ", Value))
    {
      optionValue = osdBlendScaledHQ;
      return true;
    }

    if (0 == ::strcasecmp("osdBlendScaledSHQ", Value))
    {
      optionValue = osdBlendScaledSHQ;
      return true;
    }

    if (0 == ::strcasecmp("osdBlendScaledAuto", Value))
    {
      optionValue = osdBlendScaledAuto;
      return true;
    }

    // backward compatibilty
    if (0 == ::strcasecmp("osdScaled", Value))
    {
      optionValue = osdBlendScaledAuto;
      return true;
    }

    if (0 == ::strcasecmp("osdUnscaled", Value))
    {
      optionValue = osdOverlay;
      return true;
    }

    return false;
  }
  
  bool cXineSettings::SetupParse(const char *optionName, eMonitorGamma &optionValue, const char *Name, const char *Value)
  {
    if (0 != ::strcasecmp(optionName, Name))
      return false;

    char c;
    int v = 0;    
    if (1 != ::sscanf(Value, "%d%c", &v, &c))
      return false;

    if (monitorGammaMin <= v && v <= monitorGammaMax)
    {
      optionValue = (eMonitorGamma)v;
      return true;
    }   
    
    return false;
  }
  
  bool cXineSettings::SetupParse(const char *optionName, eAudioMode &optionValue, const char *Name, const char *Value)
  {
    if (0 != ::strcasecmp(optionName, Name))
      return false;

    if (0 == ::strcasecmp("audioDolbyOff", Value))
    {
      optionValue = audioDolbyOff;
      return true;
    }

    if (0 == ::strcasecmp("audioDolbyOn", Value))
    {
      optionValue = audioDolbyOn;
      return true;
    }

    return false;
  }
  
  bool cXineSettings::SetupParse(const char *optionName, eAutoPrimaryDeviceMode &optionValue, const char *Name, const char *Value)
  {
    if (0 != ::strcasecmp(optionName, Name))
      return false;

    if (0 == ::strcasecmp("autoPrimaryDeviceOff", Value))
    {
      optionValue = autoPrimaryDeviceOff;
      return true;
    }

    if (0 == ::strcasecmp("autoPrimaryDeviceOn", Value))
    {
      optionValue = autoPrimaryDeviceOn;
      return true;
    }

    return false;
  }
  
  bool cXineSettings::SetupParse(const char *optionName, eInteractWithEitScannerMode &optionValue, const char *Name, const char *Value)
  {
    if (0 != ::strcasecmp(optionName, Name))
      return false;

    if (0 == ::strcasecmp("interactWithEitScannerOff", Value))
    {
      optionValue = interactWithEitScannerOff;
      return true;
    }

    if (0 == ::strcasecmp("interactWithEitScannerOn", Value))
    {
      optionValue = interactWithEitScannerOn;
      return true;
    }

    return false;
  }

  bool cXineSettings::SetupParse(const char *optionName, eTransparencyMode &optionValue, const char *Name, const char *Value)
  {
    if (0 != ::strcasecmp(optionName, Name))
      return false;

    if (0 == ::strcasecmp("transparencyOff", Value))
    {
      optionValue = transparencyOff;
      return true;
    }

    if (0 == ::strcasecmp("transparencyOn", Value))
    {
      optionValue = transparencyOn;
      return true;
    }

    return false;
  }
  
  bool cXineSettings::SetupParse(const char *Name, const char *Value)
  {
    if (m_modeParams[ modeLiveTV ].SetupParse("modeLiveTV.", Name, Value))
      return true;
/*    
    if (m_modeParams[ modeReplay ].SetupParse("modeReplay.", Name, Value))
      return true;

    if (SetupParse("usageModeDefault", m_usageModeDefault, Name, Value))
    {
      m_usageMode = m_usageModeDefault;
      return true;
    }
*/
    if (m_zoomParams[ image4_3 ].SetupParse("image4:3.", Name, Value))
      return true;

    if (m_zoomParams[ image16_9 ].SetupParse("image16:9.", Name, Value))
      return true;

    if (SetupParse("osdMode", m_osdMode, Name, Value))
      return true;

    if (SetupParse("osdGammaCorrection", m_crtGamma, Name, Value))
      return true;

#if APIVERSNUM >= 10707
    if (m_osdExtentParams.SetupParse("osdExtent.", Name, Value))
      return true;
#endif

#if APIVERSNUM < 10318
    if (SetupParse("audioMode", m_audioMode, Name, Value))
      return true;
#endif

    if (SetupParse("volumeMode", m_volumeMode, Name, Value))
      return true;

    if (SetupParse("muteMode", m_muteMode, Name, Value))
      return true;

#if APIVERSNUM >= 10332
    if (SetupParse("autoPrimaryDeviceMode", m_autoPrimaryDeviceMode, Name, Value))
      return true;
#endif

    if (SetupParse("transparencyMode", m_transparencyMode, Name, Value))
      return true;

    if (SetupParse("interactWithEitScannerMode", m_interactWithEitScannerMode, Name, Value))
      return true;

    return false;    
  }

  const cXineSettings::cModeParams *cXineSettings::GetModeParams() const
  {
    return &m_modeParams[ m_usageMode ];
  }

  cXineSettings::eOsdMode cXineSettings::OsdMode() const
  {
    return m_osdMode;
  }

  cXineSettings::eVolumeMode cXineSettings::VolumeMode() const
  {
    return m_volumeMode;
  }  

  cXineSettings::eMuteMode cXineSettings::MuteMode() const
  {
    return m_muteMode;
  }

  bool cXineSettings::AudioDolbyOn() const
  {
#if APIVERSNUM >= 10318
    return true;
#else
    return audioDolbyOn == m_audioMode;
#endif
  }

  void cXineSettings::Create(cXineSetupPage *const setupPage)
  {
    static const char *osdModes[ 6 ];
    osdModes[ osdOverlay ]         = tr("X11 overlay");
    osdModes[ osdBlendClipped ]    = tr("Blend clipped");
    osdModes[ osdBlendScaledLQ ]   = tr("Blend scaled LQ");
    osdModes[ osdBlendScaledHQ ]   = tr("Blend scaled HQ");
    osdModes[ osdBlendScaledSHQ ]  = tr("Blend scaled SHQ");
    osdModes[ osdBlendScaledAuto ] = tr("Blend scaled Auto");

    static const char *volumeModes[ 3 ];
    volumeModes[ volumeIgnore ]   = tr("No");
    volumeModes[ volumeChangeHW ] = tr("Yes (by hardware)");
    volumeModes[ volumeChangeSW ] = tr("Yes (by software)");
    
    static const char *muteModes[ 3 ];
    muteModes[ muteIgnore ]   = tr("Ignore");
    muteModes[ muteExecute ]  = tr("Execute");
    muteModes[ muteSimulate ] = tr("Simulate");
    
    setupPage->Add(new cMenuEditIntItem(tr("Live-TV SD video buffer [frames]"), &m_modeParams[ modeLiveTV ].m_prebufferFramesVideoSD, 0, 50));
    setupPage->Add(new cMenuEditIntItem(tr("Live-TV HD video buffer [frames]"), &m_modeParams[ modeLiveTV ].m_prebufferFramesVideoHD, 0, 50));
    setupPage->Add(new cMenuEditIntItem(tr("Live-TV audio buffer [frames]"), &m_modeParams[ modeLiveTV ].m_prebufferFramesAudio, 0, 50));
//    setupPage->Add(new cMenuEditIntItem(tr("Replay prebuffer [frames]"), &m_modeParams[ modeReplay ].m_prebufferFrames, 0, 50));
//    setupPage->Add(new cMenuEditBoolItem(tr("Default settings optimized for"), (int *)&m_usageModeDefault, tr("Live-TV"), tr("Replay")));
    setupPage->Add(new cMenuEditIntItem(tr("Buffer hysteresis [frames]"), &m_modeParams[ modeLiveTV ].m_prebufferHysteresis, 0, 50));
    setupPage->Add(new cMenuEditIntItem(tr("Buffer monitoring duration [s]"), &m_modeParams[ modeLiveTV ].m_monitoringDuration, 0, 300));
    setupPage->Add(new cMenuEditBoolItem(tr("Buffer monitoring mode"), &alias_cast<int>(m_modeParams[ modeLiveTV ].m_monitoringMode), tr("Once"), tr("Continuous")));
    setupPage->Add(new cMenuEditStraItem(tr("OSD display mode"), &alias_cast<int>(m_osdMode), sizeof (osdModes) / sizeof (*osdModes), osdModes));
    setupPage->Add(new cMenuEditIntItem(tr("OSD gamma correction [ 123 => 1.23 ]"), &alias_cast<int>(m_crtGamma), monitorGammaMin, monitorGammaMax));

#if APIVERSNUM >= 10707
    setupPage->Add(new cMenuEditIntItem(tr("OSD extent X"), &alias_cast<int>(m_osdExtentParams.m_osdExtentWidth), osdExtentWidthMin, osdExtentWidthMax));
    setupPage->Add(new cMenuEditIntItem(tr("OSD extent Y"), &alias_cast<int>(m_osdExtentParams.m_osdExtentHeight), osdExtentHeightMin, osdExtentHeightMax));
#endif

    setupPage->Add(new cMenuEditIntItem(tr("4:3 image zoom X [%]"), &alias_cast<int>(m_zoomParams[ image4_3 ].m_zoomX), imageZoomMin, imageZoomMax));
    setupPage->Add(new cMenuEditIntItem(tr("4:3 image zoom Y [%]"), &alias_cast<int>(m_zoomParams[ image4_3 ].m_zoomY), imageZoomMin, imageZoomMax));
    setupPage->Add(new cMenuEditIntItem(tr("16:9 image zoom X [%]"), &alias_cast<int>(m_zoomParams[ image16_9 ].m_zoomX), imageZoomMin, imageZoomMax));
    setupPage->Add(new cMenuEditIntItem(tr("16:9 image zoom Y [%]"), &alias_cast<int>(m_zoomParams[ image16_9 ].m_zoomY), imageZoomMin, imageZoomMax));
#if APIVERSNUM < 10318
    setupPage->Add(new cMenuEditBoolItem(tr("Audio mode"), &alias_cast<int>(m_audioMode), tr("Dolby off"), tr("Dolby on")));
#endif
    setupPage->Add(new cMenuEditStraItem(tr("Control xine's volume"), &alias_cast<int>(m_volumeMode), sizeof (volumeModes) / sizeof (*volumeModes), volumeModes));
    setupPage->Add(new cMenuEditStraItem(tr("Muting"), &alias_cast<int>(m_muteMode), sizeof (muteModes) / sizeof (*muteModes), muteModes));
    setupPage->Add(new cMenuEditBoolItem(tr("Get primary device when xine connects"), &alias_cast<int>(m_autoPrimaryDeviceMode), tr("No"), tr("Yes")));
    setupPage->Add(new cMenuEditBoolItem(tr("Support semi transparent colors"), &alias_cast<int>(m_transparencyMode), tr("No"), tr("Yes")));
    setupPage->Add(new cMenuEditBoolItem(tr("Connection interacts with EIT scanner"), &alias_cast<int>(m_interactWithEitScannerMode), tr("No"), tr("Yes")));
  }
  
  void cXineSettings::Store(cXineSetupPage *const setupPage)
  {
    setupPage->SetupStore("modeLiveTV.prebufferFramesVideoSD", m_modeParams[ modeLiveTV ].m_prebufferFramesVideoSD);
    setupPage->SetupStore("modeLiveTV.prebufferFramesVideoHD", m_modeParams[ modeLiveTV ].m_prebufferFramesVideoHD);
    setupPage->SetupStore("modeLiveTV.prebufferFramesAudio", m_modeParams[ modeLiveTV ].m_prebufferFramesAudio);
//    setupPage->SetupStore("modeReplay.prebufferFrames", m_modeParams[ modeReplay ].m_prebufferFrames);
//    setupPage->SetupStore("usageModeDefault", (m_usageModeDefault ? "modeReplay" : "modeLiveTV"));
    setupPage->SetupStore("modeLiveTV.prebufferHysteresis", m_modeParams[ modeLiveTV ].m_prebufferHysteresis);
    setupPage->SetupStore("modeLiveTV.monitoringDuration", m_modeParams[ modeLiveTV ].m_monitoringDuration);
    setupPage->SetupStore("modeLiveTV.monitoringMode", (m_modeParams[ modeLiveTV ].m_monitoringMode ? "monitoringContinuous" : "monitoringOnce"));

    setupPage->SetupStore("image4:3.zoomX", m_zoomParams[ image4_3 ].m_zoomX);
    setupPage->SetupStore("image4:3.zoomY", m_zoomParams[ image4_3 ].m_zoomY);
    setupPage->SetupStore("image16:9.zoomX", m_zoomParams[ image16_9 ].m_zoomX);
    setupPage->SetupStore("image16:9.zoomY", m_zoomParams[ image16_9 ].m_zoomY);

    {
      const char *mode = 0;

      switch (m_osdMode)
      {
      case osdOverlay:          mode = "osdOverlay";          break;
      case osdBlendClipped:     mode = "osdBlendClipped";     break;
      case osdBlendScaledLQ:    mode = "osdBlendScaledLQ";    break;
      case osdBlendScaledHQ:    mode = "osdBlendScaledHQ";    break;
      case osdBlendScaledSHQ:   mode = "osdBlendScaledSHQ";   break;
      case osdBlendScaledAuto:  mode = "osdBlendScaledAuto";  break;
      default:
        assert(false);
      }
      
      setupPage->SetupStore("osdMode", mode);
    }
    
    setupPage->SetupStore("osdGammaCorrection", (int)m_crtGamma);

#if APIVERSNUM >= 10707
    setupPage->SetupStore("osdExtent.X", m_osdExtentParams.m_osdExtentWidth);
    setupPage->SetupStore("osdExtent.Y", m_osdExtentParams.m_osdExtentHeight);
#endif

#if APIVERSNUM < 10318
    setupPage->SetupStore("audioMode", (m_audioMode ? "audioDolbyOn" : "audioDolbyOff"));
#endif

    {
      const char *mode = 0;

      switch (m_volumeMode)
      {
      case volumeIgnore:    mode = "volumeIgnore";    break;
      case volumeChangeHW:  mode = "volumeChangeHW";  break;
      case volumeChangeSW:  mode = "volumeChangeSW";  break;
      }

      setupPage->SetupStore("volumeMode", mode);
    }

    {
      const char *mode = 0;

      switch (m_muteMode)
      {
      case muteIgnore:    mode = "muteIgnore";    break;
      case muteExecute:   mode = "muteExecute";   break;
      case muteSimulate:  mode = "muteSimulate";  break;
      default:
        assert(false);
      }
      
      setupPage->SetupStore("muteMode", mode);
    }

    setupPage->SetupStore("autoPrimaryDeviceMode", (m_autoPrimaryDeviceMode ? "autoPrimaryDeviceOn" : "autoPrimaryDeviceOff"));
    setupPage->SetupStore("transparencyMode", (m_transparencyMode ? "transparencyOn" : "transparencyOff"));
    setupPage->SetupStore("interactWithEitScannerMode", (m_interactWithEitScannerMode ? "interactWithEitScannerOn" : "interactWithEitScannerOff"));
  }

  void cXineSettings::SelectReplayPrebufferMode(const bool select /* = true */)
  {
    m_usageMode = (!select ? modeLiveTV : modeReplay);
/*
    if (Interface)
      Interface->Info(m_usageMode ? tr("Settings are now optimized for 'Replay'") : tr("Settings are now optimized for 'Live-TV'"));
*/
//    cXineDevice::GetDevice()->AdjustPrebufferMode();
  } 

  void cXineSettings::TogglePrebufferMode()    
  {
    m_usageMode = (m_usageMode ? modeLiveTV : modeReplay);
/*
    if (Interface)
      Interface->Info(m_usageMode ? tr("Settings are now optimized for 'Replay'") : tr("Settings are now optimized for 'Live-TV'"));
*/
//    cXineDevice::GetDevice()->AdjustPrebufferMode();
  }
  
  const char *cXineSettings::GetMainMenuEntry()
  {
    return 0;
//    return (m_usageMode ? tr("xine - Choose optimized settings for 'Live-TV'") : tr("xine - Choose optimized settings for 'Replay'"));
  }

  bool cXineSettings::LiveTV() const
  {
    return (m_usageMode == modeLiveTV);
  }  

  bool cXineSettings::AutoPrimaryDevice() const
  {
    return (m_autoPrimaryDeviceMode == autoPrimaryDeviceOn);
  }  

  bool cXineSettings::InteractWithEitScanner() const
  {
    return (m_interactWithEitScannerMode == interactWithEitScannerOn);
  }  

  bool cXineSettings::SupportTransparency() const
  {
    return (m_transparencyMode == transparencyOn);
  }

  void cXineSettings::SetSwitchSkin(const bool switchSkin)
  {
    m_switchSkin = switchSkin;
  }
  
  bool cXineSettings::ShallSwitchSkin() const
  {
    return m_switchSkin;
  }

  bool beQuiet = false;

  void cXineSettings::SetBeQuiet(const bool beQuiet)
  {
    m_beQuiet = beQuiet;
  
    PluginXine::beQuiet = m_beQuiet;
  }
  
  bool cXineSettings::ShallBeQuiet() const
  {
    return m_beQuiet;
  }
  
  void cXineSettings::SetDefaultGrabSizeX(const int defaultGrabSizeX)
  {
    m_defaultGrabSizeX = defaultGrabSizeX;
  }
  
  int cXineSettings::DefaultGrabSizeX() const
  {
    return m_defaultGrabSizeX;
  }
  
  void cXineSettings::SetDefaultGrabSizeY(const int defaultGrabSizeY)
  {
    m_defaultGrabSizeY = defaultGrabSizeY;
  }
  
  int cXineSettings::DefaultGrabSizeY() const
  {
    return m_defaultGrabSizeY;
  }

};
