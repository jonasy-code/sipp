/**
 ** renderthread.cpp - See renderthread.h.
 **/

#include "renderthread.h"

#include <QDir>
#include <QElapsedTimer>
#include <QPainter>

#include <cmath>

#include <scene.h>

/*
 * What the pixel function needs to know about the image it writes into.
 * Taking the pointer once per frame keeps QImage from checking for
 * shared data on every pixel.
 */
struct PixelTarget {
  uchar *bits;
  qsizetype bytesPerLine;
};

RenderThread::RenderThread(const RenderSettings &settings, QObject *parent)
    : QThread(parent), m_settings(settings) {}

void RenderThread::stop() {
  m_stop = true;
  sipp_render_terminate(); /* Only sets a flag; safe from any thread */
}

QImage RenderThread::takeFrame(int *frame, double *time, double *renderMs) {
  QMutexLocker lock(&m_mutex);

  m_notified = false;
  if (frame != nullptr) {
    *frame = m_latestFrame;
  }
  if (time != nullptr) {
    *time = m_latestTime;
  }
  if (renderMs != nullptr) {
    *renderMs = m_latestMs;
  }
  return m_latest;
}

void RenderThread::setView(double azimuth, double elevation,
                           double distance) {
  QMutexLocker lock(&m_mutex);

  m_viewAzimuth = azimuth;
  m_viewElevation = elevation;
  m_viewDistance = distance;
  m_viewChanged = true;
}

void RenderThread::setPixel(void *data, int x, int y, unsigned char red,
                            unsigned char grn, unsigned char blu,
                            unsigned char alpha) {
  PixelTarget *target = static_cast<PixelTarget *>(data);
  QRgb *row = reinterpret_cast<QRgb *>(target->bits + y * target->bytesPerLine);

  Q_UNUSED(alpha);
  row[x] = qRgb(red, grn, blu);
}

void RenderThread::drawLine(void *data, int x1, int y1, int x2, int y2) {
  static_cast<QPainter *>(data)->drawLine(x1, y1, x2, y2);
}

void RenderThread::renderFrame(QImage &image, double time) {
  const int size = m_settings.size;
  double azimuth = 0.0, elevation = 0.0, distance = 0.0;
  bool moveCamera;

  {
    QMutexLocker lock(&m_mutex);
    moveCamera = m_viewChanged;
    if (moveCamera) {
      azimuth = m_viewAzimuth;
      elevation = m_viewElevation;
      distance = m_viewDistance;
      m_viewChanged = false;
    }
  }
  if (moveCamera) {
    anim_scene_view(azimuth, elevation, distance);
  }
  anim_scene_place(time);

  if (m_settings.mode == LINE) {
    image.fill(Qt::white);
    QPainter painter(&image);
    painter.setPen(Qt::black);
    /* A Line_func is passed as a Pixel_func in LINE mode; cast through
       a generic function pointer as sipp.h prescribes. */
    Line_func *line = &RenderThread::drawLine;
    render_image_func(size, size,
                      reinterpret_cast<Pixel_func *>(
                          reinterpret_cast<void (*)(void)>(line)),
                      &painter, IMAGE_PPM, LINE, 1);
  } else {
    PixelTarget target = {image.bits(), image.bytesPerLine()};
    render_image_func(size, size, &RenderThread::setPixel, &target, IMAGE_PPM,
                      m_settings.mode, m_settings.oversampling);
  }
}

void RenderThread::publish(const QImage &image, int frame, double time,
                           double renderMs) {
  {
    QMutexLocker lock(&m_mutex);
    m_latest = image;
    m_latestFrame = frame;
    m_latestTime = time;
    m_latestMs = renderMs;
  }
  m_rendered++;
  if (!m_notified.exchange(true)) {
    emit frameReady();
  }
}

/*
 * The render loop.
 *
 * With a period, the animation runs in real time: every frame is
 * rendered for the point in the loop that the clock will have reached
 * when the frame can be shown, which is about one render time from
 * now.  The number of frames in a loop is then whatever the machine
 * manages in that many seconds.  Without a period, the animation's own
 * frames are rendered one after the other, as fast as possible.
 */
void RenderThread::run() {
  QElapsedTimer clock;
  QElapsedTimer frameTimer;
  const double period = m_settings.period;
  const int nframes = anim_frames();
  double lastMs = 0.0;   /* Render time of the previous frame */
  int loop = 0;          /* Loops completed */
  int frame = 0;         /* Frames rendered in this loop */
  qint64 loopStart = 0;  /* When this loop started, clock ns */

  sipp_init();
  sipp_shading_per_pixel(m_settings.shadeOnce);
  sipp_render_threads(m_settings.threads);
  anim_scene_create(m_settings.size < 512 ? 2 * m_settings.size
                                          : m_settings.size);

  for (int i = 0; i < BUFFERS; i++) {
    m_buffers[i] = QImage(m_settings.size, m_settings.size,
                          QImage::Format_RGB32);
    m_buffers[i].fill(Qt::black);
  }

  clock.start();

  while (!m_stop) {
    double time;
    bool loopEnded;

    /* Where in the animation is the next frame? */
    if (period > 0.0) {
      const double shownAt = clock.nsecsElapsed() / 1e9 + lastMs / 1000.0;
      const int loopsPassed = (int)std::floor(shownAt / period);

      loopEnded = (loopsPassed > loop);
      time = (shownAt - loopsPassed * period) / period * anim_time_stop;
    } else {
      loopEnded = (frame == nframes);
      time = frame * anim_time_step;
    }

    if (loopEnded) {
      const qint64 now = clock.nsecsElapsed();

      loop++;
      emit loopDone(loop, frame, (now - loopStart) / 1e9);
      if (m_settings.loops > 0 && loop >= m_settings.loops) {
        break;
      }
      if (period > 0.0) {
        loop = (int)std::floor((now / 1e9 + lastMs / 1000.0) / period);
      }
      frame = 0;
      loopStart = now;
    }

    QImage &image = m_buffers[m_next];
    m_next = (m_next + 1) % BUFFERS;

    frameTimer.start();
    renderFrame(image, time);
    lastMs = frameTimer.nsecsElapsed() / 1e6;
    if (m_stop) {
      break; /* The frame was aborted, do not show it */
    }

    if (loop == 0 && !m_settings.dumpDir.isEmpty()) {
      image.save(QDir(m_settings.dumpDir)
                     .filePath(QString::asprintf("anim%02d.png", frame)));
    }
    publish(image, frame, time, lastMs);
    frame++;

    if (m_settings.maxFps > 0) {
      const qint64 due = (qint64)(1e9 / m_settings.maxFps);

      while (!m_stop && frameTimer.nsecsElapsed() < due) {
        QThread::usleep(200);
      }
    }
  }
}
