/**
 ** animview.cpp - See animview.h.
 **/

#include "animview.h"

#include <QKeyEvent>
#include <QMouseEvent>
#include <QPainter>
#include <QPaintEvent>
#include <QWheelEvent>

#include <cmath>

#include <scene.h>

#include "renderthread.h"

/* Degrees of orbit per pixel of mouse movement. */
static const double DEGREES_PER_PIXEL = 0.4;

/* Zoom factor per wheel notch (120 units of angleDelta). */
static const double ZOOM_PER_NOTCH = 1.1;

AnimView::AnimView(RenderThread *renderer, const QString &description,
                   QWidget *parent)
    : QWidget(parent), m_renderer(renderer), m_description(description) {
  setWindowTitle(m_description);
  setMinimumSize(64, 64);
  setFocusPolicy(Qt::StrongFocus);
  setToolTip("Drag to orbit, scroll to zoom, R to reset the view");
  anim_scene_default_view(&m_azimuth, &m_elevation, &m_distance);

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

/*
 * The view is changed here and handed to the renderer, which moves the
 * SIPP camera on its own thread before the next frame.
 */
void AnimView::applyView() {
  m_renderer->setView(m_azimuth, m_elevation, m_distance);
}

void AnimView::resetView() {
  anim_scene_default_view(&m_azimuth, &m_elevation, &m_distance);
  applyView();
}

void AnimView::mousePressEvent(QMouseEvent *event) {
  if (event->button() == Qt::LeftButton) {
    m_dragPos = event->pos();
  }
}

void AnimView::mouseMoveEvent(QMouseEvent *event) {
  if (event->buttons() & Qt::LeftButton) {
    const QPoint delta = event->pos() - m_dragPos;

    m_dragPos = event->pos();
    m_azimuth = std::fmod(m_azimuth - delta.x() * DEGREES_PER_PIXEL, 360.0);
    m_elevation = std::clamp(m_elevation + delta.y() * DEGREES_PER_PIXEL,
                             -10.0, 89.0);
    applyView();
  }
}

void AnimView::wheelEvent(QWheelEvent *event) {
  const double notches = event->angleDelta().y() / 120.0;

  if (notches != 0.0) {
    m_distance =
        std::clamp(m_distance * std::pow(ZOOM_PER_NOTCH, -notches), 2.0, 200.0);
    applyView();
  }
  event->accept();
}

void AnimView::keyPressEvent(QKeyEvent *event) {
  if (event->key() == Qt::Key_R) {
    resetView();
  } else {
    QWidget::keyPressEvent(event);
  }
}

void AnimView::updateTitle() {
  const double seconds = m_rateTimer.restart() / 1000.0;
  const long rendered = m_renderer->framesRendered();
  const double renderFps = (rendered - m_lastRendered) / seconds;
  const double displayFps = (m_displayed - m_lastDisplayed) / seconds;

  m_lastRendered = rendered;
  m_lastDisplayed = m_displayed;

  setWindowTitle(QString("%1 | frame %2 at t=%3, %4 ms | rendered %5 fps, "
                         "shown %6 fps | view %7\u00b0 %8\u00b0 %9")
                     .arg(m_description)
                     .arg(m_frame, 3)
                     .arg(m_time, 0, 'f', 2)
                     .arg(m_renderMs, 0, 'f', 1)
                     .arg(renderFps, 0, 'f', 1)
                     .arg(displayFps, 0, 'f', 1)
                     .arg(m_azimuth, 0, 'f', 0)
                     .arg(m_elevation, 0, 'f', 0)
                     .arg(m_distance, 0, 'f', 1));
}
