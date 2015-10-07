/*
 * xine.c: A plugin for the Video Disk Recorder
 *
 * See the README file for copyright information and how to reach the author.
 *
 * $Id$
 */

#include "xineCommon.h"

#include <vdr/plugin.h>

#include "xineDevice.h"
#include "xineSettings.h"
#include "xineSetupPage.h"
#include "xineI18n.h"



static const char *VERSION        = "0.9.4";
static const char *DESCRIPTION    = tr("Software based playback using xine");
//static const char *MAINMENUENTRY  = "xine - Toggle prebuffer setting";



class cPluginXine : public cPlugin {
private:
  // Add any member variables or functions you may need here.
  PluginXine::cXineSettings m_settings;

  PluginXine::cXineRemote *m_remote;
  bool m_remoteOn;
  
public:
  PluginXine::cXineLib *m_xineLib;
  int m_instanceNo;
  in_addr_t m_bindIp;
  int m_bindPort;
  
  cPluginXine(void);
  virtual ~cPluginXine();
  virtual const char *Version(void) { return VERSION; }
  virtual const char *Description(void) { return tr(DESCRIPTION); }
  virtual const char *CommandLineHelp(void);
  virtual bool ProcessArgs(int argc, char *argv[]);
  virtual bool Initialize(void);
  virtual bool Start(void);
  virtual void Stop(void);
  virtual void Housekeeping(void);
#if APIVERSNUM >= 10347
  virtual void MainThreadHook(void);
  virtual cString Active(void);
  virtual time_t WakeupTime(void);
#endif
  virtual const char *MainMenuEntry(void);// { return tr(MAINMENUENTRY); }
  virtual cOsdObject *MainMenuAction(void);
  virtual cMenuSetupPage *SetupMenu(void);
  virtual bool SetupParse(const char *Name, const char *Value);
#if APIVERSNUM >= 10330
  virtual bool Service(const char *Id, void *Data = NULL);
  virtual const char **SVDRPHelpPages(void);
  virtual cString SVDRPCommand(const char *Command, const char *Option, int &ReplyCode);
#endif
  };

cPluginXine::cPluginXine(void)
  : cPlugin()
  , m_remote(0)
  , m_remoteOn(false)
  , m_xineLib(0)
  , m_instanceNo(-1)
  , m_bindIp(0)
  , m_bindPort(0)
{
  // Initialize any member variables here.
  // DON'T DO ANYTHING ELSE THAT MAY HAVE SIDE EFFECTS, REQUIRE GLOBAL
  // VDR OBJECTS TO EXIST OR PRODUCE ANY OUTPUT!
}

cPluginXine::~cPluginXine()
{
  // Clean up after yourself!
}

const char *cPluginXine::CommandLineHelp(void)
{
  //Return a string that describes all known command line options.
  //"  -         --             x                                                   \n"
  return
    "  -bIP                     ip address to bind for socket connections (see -p)\n"
    "  -iN                      instance number to append to FIFO directory\n"
    "  -p[N]                    use socket connections on port N (18701)\n" 
    "  -q                       turn off debug messages on console\n"
    "  -r                       turn on remote (pressing keys in xine controls VDR)\n"
#if APIVERSNUM >= 10320
    "  -s                       switch to curses skin, while xine is disconnected\n"
#endif 
    "  -XN                      default 'SizeX' for GRAB command (720, 1..4096)\n"
    "  -YN                      default 'SizeY' for GRAB command (576, 1..4096)\n"
    ;
}

bool cPluginXine::ProcessArgs(int argc, char *argv[])
{
  // Implement command line argument processing here if applicable.

  ::optind = 0;
  ::opterr = 0;

#define INVALID_ARG(fmt, args...) do { esyslog(fmt, ##args); fprintf(stderr, fmt "\n", ##args); } while (false)

  for (int r = -1; (r = ::getopt(argc, argv, ":b:i:p::qrsX:Y:")) >= 0; )
  {
    switch (r)
    {
    case ':':
      INVALID_ARG("vdr-xine: missing argument for option -%c", ::optopt);
      return false;

    case 'b':
      m_bindIp = ::inet_addr(::optarg);
      if (m_bindIp == INADDR_NONE)
      {
        INVALID_ARG("vdr-xine: invalid argument '%s' for option -%c", ::optarg, r);
        return false;
      }
      break;

    case 'i':
      m_instanceNo = ::atoi(::optarg);
      if (m_instanceNo < 0)
      {
        INVALID_ARG("vdr-xine: invalid argument '%s' for option -%c", ::optarg, r);
        return false;
      }
      break;
      
    case 'p':
      if (!::optarg)
        m_bindPort = 18701;
      else
      {
        m_bindPort = ::atoi(::optarg);
        if (m_bindPort <= 0)
        {
          INVALID_ARG("vdr-xine: invalid argument '%s' for option -%c", ::optarg, r);
          return false;
        }
      }
      break;

    case 'r':
      m_remoteOn = true;
      break;

#if APIVERSNUM >= 10320      
    case 's':
      m_settings.SetSwitchSkin(true);
      break;
#endif

    case 'q':
      m_settings.SetBeQuiet(true);
      break;
      
    case 'X':
      {
        const int X = ::atoi(::optarg);
        if (X < 1 || X > 4096)
        {
          INVALID_ARG("vdr-xine: invalid argument '%s' for option -%c", ::optarg, r);
          return false;
        }
        
        m_settings.SetDefaultGrabSizeX(X);
      }
      break;

    case 'Y':
      {
        const int Y = ::atoi(::optarg);
        if (Y < 1 || Y > 4096)
        {
          INVALID_ARG("vdr-xine: invalid argument '%s' for option -%c", ::optarg, r);
          return false;
        }
        
        m_settings.SetDefaultGrabSizeY(Y);
      }
      break;

    default:
      INVALID_ARG("vdr-xine: invalid option -%c", r);
      return false;
    }
  }

  if (argv[::optind])
  {
    INVALID_ARG("vdr-xine: invalid argument '%s'", argv[::optind]);
    return false;
  }
  
#undef INVALID_ARG

  return true;
}

bool cPluginXine::Initialize(void)
{
#if APIVERSNUM < 10507
  RegisterI18n(PluginXine::Phrases);
#endif
  
  // Initialize any background activities the plugin shall perform.
  m_remote = new PluginXine::cXineRemote(m_remoteOn);
  if (!m_remote)
    return false;
  
  if (!PluginXine::cXineDevice::Create(this, m_settings, m_remote))
    return false;

  return true;
}

bool cPluginXine::Start(void)
{
  // Start any background activities the plugin shall perform.
  if (!PluginXine::cXineDevice::Open())
    return false;

  return true;
}

void cPluginXine::Stop(void)
{
  PluginXine::cXineDevice::Stop();
}

void cPluginXine::Housekeeping(void)
{
  // Perform any cleanup or other regular tasks.
}

#if APIVERSNUM >= 10347

void cPluginXine::MainThreadHook(void)
{
  // Perform actions in the context of the main program thread.
  // WARNING: Use with great care - see PLUGINS.html!

  PluginXine::cXineDevice::MainMenuTrampoline();
}

cString cPluginXine::Active(void)
{
  // Return a message string if shutdown should be postponed
  return NULL;
}

time_t cPluginXine::WakeupTime(void)
{
  // Return custom wakeup time for shutdown script
  return 0;
}

#endif

cOsdObject *cPluginXine::MainMenuAction(void)
{
  // Perform the action when selected from the main VDR menu.
#if APIVERSNUM < 10347
  PluginXine::cXineDevice::MainMenuTrampoline();
#endif
  return NULL;
}

cMenuSetupPage *cPluginXine::SetupMenu(void)
{
  // Return a setup menu in case the plugin supports one.
  return new PluginXine::cXineSetupPage(m_xineLib, m_settings);
}

bool cPluginXine::SetupParse(const char *Name, const char *Value)
{
  // Parse your own setup parameters and store their values.
  return m_settings.SetupParse(Name, Value);
}

#if APIVERSNUM >= 10330

bool cPluginXine::Service(const char *Id, void *Data)
{
  // Handle custom service requests from other plugins
  return cPlugin::Service(Id, Data);
}

const char **cPluginXine::SVDRPHelpPages(void)
{
  // Return help text for SVDRP commands this plugin implements
  return cPlugin::SVDRPHelpPages();
}

cString cPluginXine::SVDRPCommand(const char *Command, const char *Option, int &ReplyCode)
{
  // Process SVDRP commands this plugin implements
  return cPlugin::SVDRPCommand(Command, Option, ReplyCode);
}

#endif

const char *cPluginXine::MainMenuEntry(void)
{
//  return m_settings.GetMainMenuEntry();
  return 0;
}

namespace PluginXine
{
  int GetBindIp(cPlugin *const plugin)
  {
    return ((cPluginXine *)plugin)->m_bindIp;
  }

  int GetBindPort(cPlugin *const plugin)
  {
    return ((cPluginXine *)plugin)->m_bindPort;
  }

  int GetInstanceNo(cPlugin *const plugin)
  {
    return ((cPluginXine *)plugin)->m_instanceNo;
  }

  cXineLib *&GetXineLib(cPlugin *const plugin)
  {
    return ((cPluginXine *)plugin)->m_xineLib;
  }
}



VDRPLUGINCREATOR(cPluginXine); // Don't touch this!
