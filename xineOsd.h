
#ifndef __XINEOSD_H
#define __XINEOSD_H



#include "xineCommon.h"

#if APIVERSNUM < 10307
#include <vdr/osdbase.h>
#else
#include <vdr/osd.h>
#endif

#include <vdr/thread.h>



namespace PluginXine
{

  class cXineDevice;
  class cXineLib;

#if APIVERSNUM < 10307
  class cXineOsd : public cOsdBase
#else
  class cXineOsd : public cOsd
#endif
  {
    cXineDevice &m_xineDevice;
    cXineLib &m_xineLib;
    cMutex &m_osdMutex;
    const int m_extentWidth;
    const int m_extentHeight;

#if APIVERSNUM < 10307    
    bool m_windowVisible[ MAXNUMWINDOWS ];
#endif
#if APIVERSNUM >= 10717
    cPixmapMemory *m_pRawOsd;
#endif

    void callSendWindow(const int maxOsdWidth, const int maxOsdHeight, const int videoLeft, const int videoTop, const int videoWidth, const int videoHeight, const int videoZoomX, const int videoZoomY, const bool dontOptimize = false);

#if APIVERSNUM < 10307    
    virtual bool OpenWindow(cWindow *Window);
    virtual void CommitWindow(cWindow *Window);
    virtual void ShowWindow(cWindow *Window);
    virtual void HideWindow(cWindow *Window, bool Hide);
    virtual void MoveWindow(cWindow *Window, int x, int y);
    virtual void CloseWindow(cWindow *Window);
#else
    virtual void SetActive(bool On);
#if APIVERSNUM >= 10717
    virtual cPixmap *CreatePixmap(int Layer, const cRect &ViewPort, const cRect &DrawPort = cRect::Null);
    virtual void DestroyPixmap(cPixmap *Pixmap);
    virtual void DrawImage(const cPoint &Point, const cImage &Image);
    virtual void DrawImage(const cPoint &Point, int ImageHandle);
#endif
    virtual eOsdError CanHandleAreas(const tArea *Areas, int NumAreas);
    virtual eOsdError SetAreas(const tArea *Areas, int NumAreas);
    virtual void SaveRegion(int x1, int y1, int x2, int y2);
    virtual void RestoreRegion(void);
    virtual eOsdError SetPalette(const cPalette &Palette, int Area);
    virtual void DrawPixel(int x, int y, tColor Color);
#if APIVERSNUM < 10327    
    virtual void DrawBitmap(int x, int y, const cBitmap &Bitmap, tColor ColorFg = 0, tColor ColorBg = 0);
#elif APIVERSNUM < 10344
    virtual void DrawBitmap(int x, int y, const cBitmap &Bitmap, tColor ColorFg = 0, tColor ColorBg = 0, bool ReplacePalette = false);
#else
    virtual void DrawBitmap(int x, int y, const cBitmap &Bitmap, tColor ColorFg = 0, tColor ColorBg = 0, bool ReplacePalette = false, bool Overlay = false);
#endif    
    virtual void DrawText(int x, int y, const char *s, tColor ColorFg, tColor ColorBg, const cFont *Font, int Width = 0, int Height = 0, int Alignment = taDefault);
    virtual void DrawRectangle(int x1, int y1, int x2, int y2, tColor Color);
    virtual void DrawEllipse(int x1, int y1, int x2, int y2, tColor Color, int Quadrants = 0);
    virtual void DrawSlope(int x1, int y1, int x2, int y2, tColor Color, int Type);    
    virtual void Flush(void);
#endif
    void HideOsd();
    void GetMaxOsdSize(int &maxOsdWidth, int &maxOsdHeight);
    
  public:
#if APIVERSNUM < 10509
    cXineOsd(cXineDevice &device, int w, int h, int x, int y);
#else
    cXineOsd(cXineDevice &device, int w, int h, int x, int y, uint Level);
#endif
    virtual ~cXineOsd();

    void ReshowCurrentOsd(const bool dontOptimize, const int frameLeft = -1, const int frameTop = -1, const int frameWidth = -1, const int frameHeight = -1, const int frameZoomX = -1, const int frameZoomY = -1);

    friend class cXineLib;
  };



  class cXineOsdMutexLock
  {
    cMutexLock *m_pOsdLock;
#if APIVERSNUM >= 10717
    cPixmapMutexLock *m_pPixmapMutexLock;
#endif

  public:
    cXineOsdMutexLock(cMutex *const pOsdMutex);
    ~cXineOsdMutexLock();
  };


 
#if APIVERSNUM >= 10307
  
  class cXineOsdProvider : public cOsdProvider
  {
    cXineDevice &m_xineDevice;
    
  public:
    cXineOsdProvider(cXineDevice &xineDevice);
#if APIVERSNUM < 10509
    virtual cOsd *CreateOsd(int Left, int Top);
#else
    virtual cOsd *CreateOsd(int Left, int Top, uint Level);
    virtual cOsd *CreateOsd(int ExtentWidth, int ExtentHeight, int Left, int Top, uint Level);
#endif
#if APIVERSNUM >= 10717
    virtual bool ProvidesTrueColor(void);
#endif
  };

#endif

  
  
};



#endif //__XINEOSD_H
