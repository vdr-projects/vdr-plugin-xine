
#include "xineCommon.h"

#include "xineOsd.h"
#include "xineDevice.h"
#include "xineLib.h"



namespace PluginXine
{
#if APIVERSNUM < 10307
  
  bool cXineOsd::OpenWindow(cWindow *Window)
  {
    cXineOsdMutexLock osdLock(&m_osdMutex);
    
    m_windowVisible[ Window->Handle() ] = false;
    
    int maxOsdWidth, maxOsdHeight;
    GetMaxOsdSize(maxOsdWidth, maxOsdHeight);

    return m_xineLib.OpenWindow(this, Window, maxOsdWidth, maxOsdHeight);
  }

  void cXineOsd::CommitWindow(cWindow *Window)
  {
    cXineOsdMutexLock osdLock(&m_osdMutex);
    
    m_xineLib.CommitWindow(this, Window);
  }

  void cXineOsd::ShowWindow(cWindow *Window)
  {
    cXineOsdMutexLock osdLock(&m_osdMutex);
    
    m_windowVisible[ Window->Handle() ] = true;
    
    m_xineLib.ShowWindow(this, Window);
  }

  void cXineOsd::HideWindow(cWindow *Window, bool Hide)
  {
    cXineOsdMutexLock osdLock(&m_osdMutex);
    
    m_windowVisible[ Window->Handle() ] = !Hide;
    
    m_xineLib.HideWindow(this, Window, Hide);
  }

  void cXineOsd::MoveWindow(cWindow *Window, int x, int y)
  {
    cXineOsdMutexLock osdLock(&m_osdMutex);
    
    m_xineLib.MoveWindow(this, Window, x, y);
  }

  void cXineOsd::CloseWindow(cWindow *Window)
  {
    cXineOsdMutexLock osdLock(&m_osdMutex);
    
    m_windowVisible[ Window->Handle() ] = false;
    
    m_xineLib.CloseWindow(this, Window);
  }

#endif


  static int vl = -1, vt = -1, vw = -1, vh = -1, zx = -1, zy = -1;

  
  void cXineOsd::ReshowCurrentOsd(const bool dontOptimize, const int frameLeft /* = -1 */, const int frameTop /* = -1 */, const int frameWidth /* = -1 */, const int frameHeight /* = -1 */, const int frameZoomX /* = -1 */, const int frameZoomY /* = -1 */)
  {
    cXineOsdMutexLock osdLock(&m_osdMutex);
    
    int maxOsdWidth, maxOsdHeight;
    GetMaxOsdSize(maxOsdWidth, maxOsdHeight);

#if APIVERSNUM < 10307    

    (void)vl;
    (void)vt;
    (void)vw;
    (void)vh;
    (void)zx;
    (void)zy;

    for (int i = 0; i < NumWindows(); i++)
    {
      cWindow *const window = GetWindowNr(i);
      if (!window)
        continue;
      
      if (dontOptimize)
        m_xineLib.OpenWindow(this, window, maxOsdWidth, maxOsdHeight);
      
      m_xineLib.CommitWindow(this, window, !dontOptimize);

      if (m_windowVisible[ i ])
        Show(window->Handle());
      else
        Hide(window->Handle());
    }

#else

#ifdef SET_VIDEO_WINDOW
    
    m_xineLib.SetVideoWindow(maxOsdWidth, maxOsdHeight, vidWin, dontOptimize);
    
#endif    
    
    int videoLeft   = frameLeft;
    int videoTop    = frameTop;
    int videoWidth  = frameWidth;
    int videoHeight = frameHeight;
    int videoZoomX  = frameZoomX;
    int videoZoomY  = frameZoomY;

    if (frameLeft < 0
        || frameTop < 0
        || frameWidth < 0
        || frameHeight < 0
        || frameZoomX < 0
        || frameZoomY < 0)
    {
      m_xineLib.execFuncVideoSize(videoLeft, videoTop, videoWidth, videoHeight, videoZoomX, videoZoomY);
    }

//    ::fprintf(stderr, "frame: %d x %d, %d x %d\n", videoLeft, videoTop, videoWidth, videoHeight, videoZoomX, videoZoomY);

    vl = videoLeft;
    vt = videoTop;
    vw = videoWidth;
    vh = videoHeight;
    zx = videoZoomX;
    zy = videoZoomY;
   
    callSendWindow(maxOsdWidth, maxOsdHeight, videoLeft, videoTop, videoWidth, videoHeight, videoZoomX, videoZoomY, dontOptimize);
#endif

    m_xineLib.execFuncOsdFlush();
  }

#if APIVERSNUM < 10509
  cXineOsd::cXineOsd(cXineDevice &xineDevice, int w, int h, int x, int y)
#else
  cXineOsd::cXineOsd(cXineDevice &xineDevice, int w, int h, int x, int y, uint Level)
#endif
#if APIVERSNUM < 10307    
    : cOsdBase(x, y)
#elif APIVERSNUM < 10509
    : cOsd(x, y)
#else
    : cOsd(x, y, Level)
#endif
    , m_xineDevice(xineDevice)
    , m_xineLib(xineDevice.m_xineLib)
    , m_osdMutex(xineDevice.m_osdMutex)
    , m_extentWidth(w)
    , m_extentHeight(h)
#if APIVERSNUM >= 10717
    , m_pRawOsd(0)
#endif
  {
//static int cnt = 0;
//fprintf(stderr, "x: %d, y: %d, cnt: %d\n", x, y, cnt);
//    if (x == 58 && y == 46 && cnt++ == 5) { char cmd[500]; sprintf(cmd, "ddd /soft/vdr-" APIVERSION "/bin/vdr %d", getpid()); system(cmd); sleep(10); } //should never happen!

#if APIVERSNUM < 10307    
    ::memset(m_windowVisible, 0, sizeof (m_windowVisible));
#endif
  }

  cXineOsd::~cXineOsd()
  {
#if APIVERSNUM < 10509
    HideOsd();
#else
    cXineOsdMutexLock osdLock(&m_osdMutex);
    
    if (Active())
      SetActive(false);
#endif
#if APIVERSNUM >= 10717
    delete m_pRawOsd;
#endif
  }

  void cXineOsd::HideOsd()
  {
    cXineOsdMutexLock osdLock(&m_osdMutex);
    
#if APIVERSNUM < 10307    

    for (int i = 0; i < MAXNUMWINDOWS; i++)
      m_xineLib.CloseWindow(this, i);

#else

    int maxOsdWidth, maxOsdHeight;
    GetMaxOsdSize(maxOsdWidth, maxOsdHeight);

    tArea defaultWindow;
    defaultWindow.x1 = 0;
    defaultWindow.y1 = 0;
    defaultWindow.x2 = 0;
    defaultWindow.y2 = 0;
    defaultWindow.bpp = 0;

    m_xineLib.SetVideoWindow(maxOsdWidth, maxOsdHeight, defaultWindow);
    
    for (int i = 0; i < MAXNUMWINDOWS; i++)
      m_xineLib.SendWindow(maxOsdWidth, maxOsdHeight, this, i);
    
#endif    

    m_xineDevice.OnFreeOsd(this);

    m_xineLib.execFuncOsdFlush();

//    m_xineLib.execFuncGrabImage("/tmp/grab.jpg", true, 50, 1000, 1000);
  }

#if APIVERSNUM >= 10307

  void cXineOsd::SetActive(bool On)
  {
#if APIVERSNUM >= 10509

    cXineOsdMutexLock osdLock(&m_osdMutex);
    
    if (On == Active())
      return;

    cOsd::SetActive(On);

    if (!m_xineDevice.ChangeCurrentOsd(this, On))
      return;

    if (On)
      m_xineLib.ReshowCurrentOSD();
    else
      HideOsd();

#endif    
  }

#endif

#if APIVERSNUM >= 10717

  cPixmap *cXineOsd::CreatePixmap(int Layer, const cRect &ViewPort, const cRect &DrawPort /* = cRect::Null */)
  {
    cXineOsdMutexLock osdLock(&m_osdMutex);
    return cOsd::CreatePixmap(Layer, ViewPort, DrawPort);
  }

  void cXineOsd::DestroyPixmap(cPixmap *Pixmap)
  {
    cXineOsdMutexLock osdLock(&m_osdMutex);
    cOsd::DestroyPixmap(Pixmap);
  }

  void cXineOsd::DrawImage(const cPoint &Point, const cImage &Image)
  {
    cXineOsdMutexLock osdLock(&m_osdMutex);
    cOsd::DrawImage(Point, Image);
  }

  void cXineOsd::DrawImage(const cPoint &Point, int ImageHandle)
  {
    cXineOsdMutexLock osdLock(&m_osdMutex);
    cOsd::DrawImage(Point, ImageHandle);
  }

#endif

#if APIVERSNUM >= 10307
  
  eOsdError cXineOsd::CanHandleAreas(const tArea *Areas, int NumAreas)
  {
//    fprintf(stderr, "cXineOsd::CanHandleAreas(%p, %d)\n", Areas, NumAreas);

    const eOsdError retVal = cOsd::CanHandleAreas(Areas, NumAreas);
    if (oeOk != retVal)
      return retVal;

    for (int i = 0; i < NumAreas; i++)
    {
      const tArea &a = Areas[ i ];
/*      
      fprintf(stderr, "Areas[ %d ]: (%d,%d)-(%d,%d)@%d\n"
              , i
              , a.x1
              , a.y1
              , a.x2
              , a.y2
              , a.bpp);
*/      
      assert(a.x1 <= a.x2);
      assert(a.y1 <= a.y2);

      if (1 != a.bpp
          && 2 != a.bpp
          && 4 != a.bpp
          && 8 != a.bpp
#if APIVERSNUM >= 10717
          && (32 != a.bpp || !m_xineDevice.SupportsTrueColorOSD())
#endif
          )
      {
        return oeBppNotSupported;
      }
    }

    return oeOk;
  }

  eOsdError cXineOsd::SetAreas(const tArea *Areas, int NumAreas)
  {
    cXineOsdMutexLock osdLock(&m_osdMutex);
    return cOsd::SetAreas(Areas, NumAreas);
  }
  
  void cXineOsd::SaveRegion(int x1, int y1, int x2, int y2)
  {
    cXineOsdMutexLock osdLock(&m_osdMutex);
    cOsd::SaveRegion(x1, y1, x2, y2);
  }
  
  void cXineOsd::RestoreRegion(void)
  {
    cXineOsdMutexLock osdLock(&m_osdMutex);
    cOsd::RestoreRegion();
  }
  
  eOsdError cXineOsd::SetPalette(const cPalette &Palette, int Area)
  {
    cXineOsdMutexLock osdLock(&m_osdMutex);
    return cOsd::SetPalette(Palette, Area);
  }
  
  void cXineOsd::DrawPixel(int x, int y, tColor Color)
  {
    cXineOsdMutexLock osdLock(&m_osdMutex);
    cOsd::DrawPixel(x, y, Color);
  }
  
#if APIVERSNUM < 10327    
  void cXineOsd::DrawBitmap(int x, int y, const cBitmap &Bitmap, tColor ColorFg /* = 0 */, tColor ColorBg /* = 0 */)
  {
    cXineOsdMutexLock osdLock(&m_osdMutex);
    cOsd::DrawBitmap(x, y, Bitmap, ColorFg, ColorBg);
//    fprintf(stderr, "drawbitmap\n");
  }
#elif APIVERSNUM < 10344
  void cXineOsd::DrawBitmap(int x, int y, const cBitmap &Bitmap, tColor ColorFg /* = 0 */, tColor ColorBg /* = 0 */, bool ReplacePalette /* = false */)
  {
    cXineOsdMutexLock osdLock(&m_osdMutex);
    cOsd::DrawBitmap(x, y, Bitmap, ColorFg, ColorBg, ReplacePalette);
//    fprintf(stderr, "drawbitmap\n");
  }
#else  
  void cXineOsd::DrawBitmap(int x, int y, const cBitmap &Bitmap, tColor ColorFg /* = 0 */, tColor ColorBg /* = 0 */, bool ReplacePalette /* = false */, bool Overlay /* = false */)
  {
    cXineOsdMutexLock osdLock(&m_osdMutex);
    cOsd::DrawBitmap(x, y, Bitmap, ColorFg, ColorBg, ReplacePalette, Overlay);
//    fprintf(stderr, "drawbitmap: (%d x %d) at (%d, %d) in (%d x %d)\n", Bitmap.Width(), Bitmap.Height(), x, y, Width(), Height());
  }
#endif

  void cXineOsd::DrawText(int x, int y, const char *s, tColor ColorFg, tColor ColorBg, const cFont *Font, int Width /* = 0 */, int Height /* = 0 */, int Alignment /* = taDefault */)
  {
    cXineOsdMutexLock osdLock(&m_osdMutex);
    cOsd::DrawText(x, y, s, ColorFg, ColorBg, Font, Width, Height, Alignment);
  }
  
  void cXineOsd::DrawRectangle(int x1, int y1, int x2, int y2, tColor Color)
  {
    cXineOsdMutexLock osdLock(&m_osdMutex);
    cOsd::DrawRectangle(x1, y1, x2, y2, Color);
  }
  
  void cXineOsd::DrawEllipse(int x1, int y1, int x2, int y2, tColor Color, int Quadrants /* = 0 */)
  {
    cXineOsdMutexLock osdLock(&m_osdMutex);
    cOsd::DrawEllipse(x1, y1, x2, y2, Color, Quadrants);
  }
  
  void cXineOsd::DrawSlope(int x1, int y1, int x2, int y2, tColor Color, int Type)
  {
    cXineOsdMutexLock osdLock(&m_osdMutex);
    cOsd::DrawSlope(x1, y1, x2, y2, Color, Type);
  }
  
  void cXineOsd::Flush(void)
  {
//    ::fprintf(stderr, "Flush ---: %s\n", ::ctime(&(const time_t &)::time(0)));
    cXineOsdMutexLock osdLock(&m_osdMutex);
//    ::fprintf(stderr, "Flush +++: %s\n", ::ctime(&(const time_t &)::time(0)));
#if APIVERSNUM >= 10509
    if (!Active())
      return;
#endif
//    static int cnt = 0;
//    fprintf(stderr, "cXineOsd::Flush() %d\n", cnt++);

    int maxOsdWidth, maxOsdHeight;
    GetMaxOsdSize(maxOsdWidth, maxOsdHeight);

#ifdef SET_VIDEO_WINDOW
    
    m_xineLib.SetVideoWindow(maxOsdWidth, maxOsdHeight, vidWin);

#endif    
    
    int videoLeft   = -1;
    int videoTop    = -1;
    int videoWidth  = -1;
    int videoHeight = -1;
    int videoZoomX  = -1;
    int videoZoomY  = -1;

    m_xineLib.execFuncVideoSize(videoLeft, videoTop, videoWidth, videoHeight, videoZoomX, videoZoomY);

    if (videoLeft  < 0) videoLeft  = 0;
    if (videoTop   < 0) videoTop   = 0;
    if (videoZoomX < 0) videoZoomX = 100;
    if (videoZoomY < 0) videoZoomY = 100;

    if (vl != videoLeft
        || vt != videoTop
        || vw != videoWidth
        || vh != videoHeight
        || zx != videoZoomX
        || zy != videoZoomY)
    {
      xfprintf(stderr, "frame: (%d, %d)-(%d, %d), zoom: (%.2lf, %.2lf)\n", videoLeft, videoTop, videoWidth, videoHeight, videoZoomX / 100.0, videoZoomY / 100.0);
//      ::fprintf(stderr, "video: %d x %d, %d x %d @ %d %% x %d %%\n", videoLeft, videoTop, videoWidth, videoHeight, videoZoomX, videoZoomY);

      vl = videoLeft;
      vt = videoTop;
      vw = videoWidth;
      vh = videoHeight;
      zx = videoZoomX;
      zy = videoZoomY;
    }

    callSendWindow(maxOsdWidth, maxOsdHeight, videoLeft, videoTop, videoWidth, videoHeight, videoZoomX, videoZoomY);

    m_xineLib.execFuncOsdFlush();
  }

  void cXineOsd::callSendWindow(const int maxOsdWidth, const int maxOsdHeight, const int videoLeft, const int videoTop, const int videoWidth, const int videoHeight, const int videoZoomX, const int videoZoomY, const bool dontOptimize /* = false */)
  {
#if APIVERSNUM >= 10717
bool head = false;
    if (IsTrueColor())
    {
      if (m_pRawOsd)
      {
        if (m_pRawOsd->DrawPort().Width() != Width()
          || m_pRawOsd->DrawPort().Height() != Height())
        {
          delete m_pRawOsd;
          m_pRawOsd = 0;
        }
      }

      if (!m_pRawOsd)
      {
        m_pRawOsd = new cPixmapMemory(0, cRect(0, 0, Width(), Height()));
        m_pRawOsd->Clear();
if (!head)
{
  head = true;
//  fprintf(stderr, "OSD: -c-c-c-c-c-c-c-c-c-c-c-c-c-c-c-c-c-c-c-c-c-c-c-c-c\n");
}
      }

      while (cPixmapMemory *pm = RenderPixmaps())
      {
if (!head)
{
  head = true;
//  fprintf(stderr, "OSD: --------------------------------------------------\n");
}
/*
  fprintf(stderr, "OSD: X: %4d, Y: %4d, W: %4d, H: %4d, X: %4d, Y: %4d, W: %4d, H: %4d\n"
, pm->ViewPort().X()
, pm->ViewPort().Y()
, pm->ViewPort().Width()
, pm->ViewPort().Height()
, pm->DrawPort().X()
, pm->DrawPort().Y()
, pm->DrawPort().Width()
, pm->DrawPort().Height()
);
*/
        m_pRawOsd->Copy(pm, pm->DrawPort().Shifted(-pm->DrawPort().Point()), pm->ViewPort().Point());
        delete pm; 
      }
    }
    else if (m_pRawOsd)
    {
      delete m_pRawOsd;
      m_pRawOsd = 0;
/*
if (head)
  fprintf(stderr, "OSD: =d=d=d=d=d=d=d=d=d=d=d=d=d=d=d=d=d=d=d=d=d=d=d=d=d\n");
*/
    }

    if (m_pRawOsd)
    {
      cXinePixmapMemoryAdapter adapter(m_pRawOsd);
/*
cXineAdapter *bm = adapter;
int x0, y0, x1, y1;
bool d = bm->Dirty(x0, y0, x1, y1);
if (head)
  fprintf(stderr, d ? "OSD: x: %4d, y: %4d, w: %4d, h: %4d, x0: %4d, y0: %4d, x1: %4d, y1: %4d\n" : "OSD: x: %4d, y: %4d, w: %4d, h: %4d\n"
, bm->X0()
, bm->Y0()
, bm->Width()
, bm->Height()
, x0
, y0
, x1
, y1
);
*/
      m_xineLib.SendWindow(maxOsdWidth, maxOsdHeight, this, 0, adapter, videoLeft, videoTop, videoWidth, videoHeight, videoZoomX, videoZoomY, dontOptimize);

      for (int i = 1; i < MAXNUMWINDOWS; i++)
        m_xineLib.SendWindow(maxOsdWidth, maxOsdHeight, this, i);
/*
if (head)
  fprintf(stderr, "OSD: ==================================================\n");
*/
    }
    else
#endif
    {
      for (int i = 0; i < MAXNUMWINDOWS; i++)
      {
        cXineBitmapAdapter adapter(GetBitmap(i));
        m_xineLib.SendWindow(maxOsdWidth, maxOsdHeight, this, i, adapter, videoLeft, videoTop, videoWidth, videoHeight, videoZoomX, videoZoomY, dontOptimize);
      }
    }
  }

  void cXineOsd::GetMaxOsdSize(int &maxOsdWidth, int &maxOsdHeight)
  {
    maxOsdWidth  = m_extentWidth;
    maxOsdHeight = m_extentHeight;
    return;
/*
    maxOsdWidth  = 0;
    maxOsdHeight = 0;

    for (int i = 0; i < MAXNUMWINDOWS; i++)
    {
      cBitmap *const p = GetBitmap(i);
      if (!p)
        continue;

      int w = Left() + p->X0() + p->Width();
      int h = Top()  + p->Y0() + p->Height();

      if (maxOsdWidth < w)
        maxOsdWidth = w;

      if (maxOsdHeight < h)
        maxOsdHeight = h;
    }
//fprintf(stderr, "GetMaxOsdSize(%d, %d)\n", maxOsdWidth, maxOsdHeight);
#if APIVERSNUM >= 10707
    if (maxOsdWidth <= m_xineDevice.Settings().OsdExtentParams().GetOsdExtentWidth()
      && maxOsdHeight <= m_xineDevice.Settings().OsdExtentParams().GetOsdExtentHeight())
    {
      maxOsdWidth  = m_xineDevice.Settings().OsdExtentParams().GetOsdExtentWidth();
      maxOsdHeight = m_xineDevice.Settings().OsdExtentParams().GetOsdExtentHeight();
    }
    else
#endif
    if (maxOsdWidth > 1920 || maxOsdHeight > 1080)
    {
      if (maxOsdWidth < 1920)
        maxOsdWidth  = 1920;

      if (maxOsdHeight < 1080)
        maxOsdHeight = 1080;
    }
    else if (maxOsdWidth > 1280 || maxOsdHeight > 720)
    {
      maxOsdWidth  = 1920;
      maxOsdHeight = 1080;
    }
    else if (maxOsdWidth > 720 || maxOsdHeight > 576)
    {
      maxOsdWidth  = 1280;
      maxOsdHeight = 720;
    }
    else
    {
      maxOsdWidth  = 720;
      maxOsdHeight = 576;
    }
*/
  }



  cXineOsdMutexLock::cXineOsdMutexLock(cMutex *const pOsdMutex)
    : m_pOsdLock(0)
#if APIVERSNUM >= 10717
    , m_pPixmapMutexLock(0)
#endif
  {
#if APIVERSNUM >= 10717
    m_pPixmapMutexLock = new cPixmapMutexLock;
#endif
    m_pOsdLock = new cMutexLock(pOsdMutex);
  }

  cXineOsdMutexLock::~cXineOsdMutexLock()
  {
    delete m_pOsdLock;

#if APIVERSNUM >= 10717
    delete m_pPixmapMutexLock;
#endif      
  }



  cXineOsdProvider::cXineOsdProvider(cXineDevice &xineDevice)
    : cOsdProvider()
    , m_xineDevice(xineDevice)
  {
  }

#if APIVERSNUM < 10509  
  cOsd *cXineOsdProvider::CreateOsd(int Left, int Top)
  {
    return m_xineDevice.NewOsd(0, 0, Left, Top);
  }
#else
  cOsd *cXineOsdProvider::CreateOsd(int Left, int Top, uint Level)
  {
    return m_xineDevice.NewOsd(0, 0, Left, Top, Level);
  }

  cOsd *cXineOsdProvider::CreateOsd(int ExtentWidth, int ExtentHeight, int Left, int Top, uint Level)
  {
    return m_xineDevice.NewOsd(ExtentWidth, ExtentHeight, Left, Top, Level);
  }
#endif

#if APIVERSNUM >= 10717
  bool cXineOsdProvider::ProvidesTrueColor()
  {
    return true;
  }
#endif
  
#endif


  
};
