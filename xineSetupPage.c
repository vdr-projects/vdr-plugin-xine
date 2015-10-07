
#include "xineCommon.h"

#include "xineSetupPage.h"
#include "xineSettings.h"
#include "xineLib.h"



namespace PluginXine
{

  void cXineSetupPage::Store()
  {
    m_globalSettings.Store(this);

    bool setupDiffers = m_globalSettings.setupDiffers(m_localSettings);
#if APIVERSNUM >= 10707
    bool lockOsdUpdate = m_globalSettings.OsdExtentParams() != m_localSettings.OsdExtentParams();
#endif

    m_localSettings = m_globalSettings;

    if (!setupDiffers)
      return;

    m_xineLib->execFuncSetup();
#if APIVERSNUM >= 10707
    if (lockOsdUpdate)
    {
      m_xineLib->LockOsdUpdate();
      cOsdProvider::UpdateOsdSize(true);
      SetDisplayMenu();
    }

    m_xineLib->ReshowCurrentOSD(lockOsdUpdate);
#else
    m_xineLib->ReshowCurrentOSD();
#endif
  }
    
  cXineSetupPage::cXineSetupPage(cXineLib *const xineLib, cXineSettings &settings)
    : cMenuSetupPage()
    , m_xineLib(xineLib)
    , m_globalSettings(settings)
    , m_localSettings(settings)
  {
    m_globalSettings.Create(this);
  }

  cXineSetupPage::~cXineSetupPage()
  {
    bool setupDiffers = m_globalSettings.setupDiffers(m_localSettings);
#if APIVERSNUM >= 10707
    bool lockOsdUpdate = m_globalSettings.OsdExtentParams() != m_localSettings.OsdExtentParams();
#endif

    m_globalSettings = m_localSettings;

    if (!setupDiffers)
      return;

    m_xineLib->execFuncSetup();
#if APIVERSNUM >= 10707
    if (lockOsdUpdate)
    {
      m_xineLib->LockOsdUpdate();
      cOsdProvider::UpdateOsdSize(true);
      SetDisplayMenu();
    }

    m_xineLib->ReshowCurrentOSD(lockOsdUpdate);
#else
    m_xineLib->ReshowCurrentOSD();
#endif
  }
  
  eOSState cXineSetupPage::ProcessKey(eKeys Key)
  {
    const cXineSettings prevSettings = m_globalSettings;
    
    eOSState state = cMenuSetupPage::ProcessKey(Key);

    if (prevSettings.setupDiffers(m_globalSettings))
    {
      m_xineLib->execFuncSetup();
#if APIVERSNUM >= 10707
      bool lockOsdUpdate = prevSettings.OsdExtentParams() != m_globalSettings.OsdExtentParams();
      if (lockOsdUpdate)
      {
        m_xineLib->LockOsdUpdate();
        cOsdProvider::UpdateOsdSize(true);
        SetDisplayMenu();
        Display();
      }

      m_xineLib->ReshowCurrentOSD(lockOsdUpdate);
#else
      m_xineLib->ReshowCurrentOSD();
#endif
    }
    
    return state;
  }

};
