/**
 ** renderthread.h - Renders the animation over and over on a background
 **                  thread and hands the frames to the GUI thread.
 **
 ** All use of SIPP happens on this thread (and on the worker threads SIPP
 ** starts itself): sipp_init(), building the scene, moving the teapot
 ** and rendering.  The GUI thread never touches SIPP; it only reads
 ** finished frames.
 **
 ** Frames are rendered into a ring of QImages.  When a frame is done it
 ** is published as "the latest frame" and the GUI is signalled, at most
 ** once per frame it has not yet collected, so that a slow display skips
 ** frames instead of queueing them.  QImage is implicitly shared, so
 ** publishing is cheap and a buffer that the GUI still holds when the
 ** renderer comes round to it again is copied rather than overwritten.
 **/

#ifndef RENDERTHREAD_H
#define RENDERTHREAD_H

#include <QImage>
#include <QMutex>
#include <QString>
#include <QThread>

#include <atomic>

#include <sipp.h>

struct RenderSettings {
  int size = 256;           /* Image is size x size pixels */
  Render_mode mode = PHONG; /* PHONG, GOURAUD, FLAT or LINE */
  int threads = 0;          /* SIPP rendering threads, 0 = all cores */
  bool shadeOnce = false;   /* sipp_shading_per_pixel() */
  int oversampling = 2;
  double period = 1.0; /* Seconds per loop of the animation; the frames
                          are rendered for the wall-clock time they will
                          be shown at.  0: step through the animation's
                          own frames as fast as possible */
  int maxFps = 0;      /* Cap on frames per second, 0 = none */
  int loops = 0;       /* Stop after this many loops, 0 = run forever */
  QString dumpDir;     /* If set, write the first loop's frames here */
};

class RenderThread : public QThread {
  Q_OBJECT

public:
  explicit RenderThread(const RenderSettings &settings,
                        QObject *parent = nullptr);

  /* Ask the loop to end; call wait() afterwards.  Any thread. */
  void stop();

  /* The most recently published frame, its number within the loop,
     its time in the animation and how long it took.  GUI thread. */
  QImage takeFrame(int *frame, double *time, double *renderMs);

  /* Frames rendered so far.  Any thread. */
  long framesRendered() const { return m_rendered.load(); }

signals:
  /* A new frame can be collected with takeFrame(). */
  void frameReady();

  /* One loop of the animation was rendered in SECONDS. */
  void loopDone(int loop, int frames, double seconds);

protected:
  void run() override;

private:
  static void setPixel(void *data, int x, int y, unsigned char red,
                       unsigned char grn, unsigned char blu,
                       unsigned char alpha);
  static void drawLine(void *data, int x1, int y1, int x2, int y2);

  void renderFrame(QImage &image, double time);
  void publish(const QImage &image, int frame, double time, double renderMs);

  enum { BUFFERS = 3 };

  RenderSettings m_settings;
  QImage m_buffers[BUFFERS];
  int m_next = 0; /* Next buffer to render into */

  QMutex m_mutex; /* Guards m_latest* */
  QImage m_latest;
  int m_latestFrame = 0;
  double m_latestTime = 0.0;
  double m_latestMs = 0.0;

  std::atomic<bool> m_stop{false};
  std::atomic<bool> m_notified{false}; /* frameReady() sent, not collected */
  std::atomic<long> m_rendered{0};
};

#endif /* RENDERTHREAD_H */
