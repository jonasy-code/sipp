/**
 ** animview.cpp - See animview.h.
 **/

#include "animview.h"

#include <QPainter>
#include <QPaintEvent>

#include "renderthread.h"

AnimView::AnimView(RenderThread *renderer, const QString &description,
                   QWidget *parent)
    : QWidget(parent), m_renderer(renderer), m_description(description) {
  setWindowTitle(m_description);
  setMinimumSize(64, 64);

  connect(m_renderer, &RenderThread::frameReady, this,
          &AnimView::collectFrame);

  connect(&m_titleTimer, &QTimer::timeout, this, &AnimView::updateTitle);
  m_titleTimer.start(1000);
  m_rateTimer.start();
}

QSize AnimView::sizeHint() const {
  if (m_image.isNull()) {
    return QSize(256, 256);
  }
  return m_image.size();
}

/*
 * The renderer signalled a new frame: fetch it and repaint.  Only the
 * latest frame is fetched, so if this thread falls behind, frames are
 * skipped rather than queued.
 */
void AnimView::collectFrame() {
  const bool first = m_image.isNull();

  m_image = m_renderer->takeFrame(&m_frame, &m_time, &m_renderMs);
  m_displayed++;
  if (first) {
    adjustSize();
  }
  update();
}

void AnimView::paintEvent(QPaintEvent *event) {
  QPainter painter(this);

  Q_UNUSED(event);
  if (m_image.isNull()) {
    painter.fillRect(rect(), Qt::black);
    return;
  }
  painter.setRenderHint(QPainter::SmoothPixmapTransform,
                        m_image.size() != size());
  painter.drawImage(rect(), m_image);
}

void AnimView::updateTitle() {
  const double seconds = m_rateTimer.restart() / 1000.0;
  const long rendered = m_renderer->framesRendered();
  const double renderFps = (rendered - m_lastRendered) / seconds;
  const double displayFps = (m_displayed - m_lastDisplayed) / seconds;

  m_lastRendered = rendered;
  m_lastDisplayed = m_displayed;

  setWindowTitle(QString("%1 | frame %2 at t=%3, %4 ms | rendered %5 fps, "
                         "shown %6 fps")
                     .arg(m_description)
                     .arg(m_frame, 3)
                     .arg(m_time, 0, 'f', 2)
                     .arg(m_renderMs, 0, 'f', 1)
                     .arg(renderFps, 0, 'f', 1)
                     .arg(displayFps, 0, 'f', 1));
}
