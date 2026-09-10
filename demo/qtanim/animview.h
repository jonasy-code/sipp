/**
 ** animview.h - The window showing the animation, with rendering and
 **              display rates in its title.
 **/

#ifndef ANIMVIEW_H
#define ANIMVIEW_H

#include <QElapsedTimer>
#include <QImage>
#include <QPoint>
#include <QString>
#include <QTimer>
#include <QWidget>

class RenderThread;

class AnimView : public QWidget {
  Q_OBJECT

public:
  AnimView(RenderThread *renderer, const QString &description,
           QWidget *parent = nullptr);

  QSize sizeHint() const override;

protected:
  void paintEvent(QPaintEvent *event) override;
  void mousePressEvent(QMouseEvent *event) override;
  void mouseMoveEvent(QMouseEvent *event) override;
  void wheelEvent(QWheelEvent *event) override;
  void keyPressEvent(QKeyEvent *event) override;

private slots:
  void collectFrame();
  void updateTitle();

private:
  void resetView();
  void applyView();

  RenderThread *m_renderer;
  double m_azimuth = 0.0; /* The view, see anim_scene_view() */
  double m_elevation = 0.0;
  double m_distance = 0.0;
  QPoint m_dragPos; /* Where the mouse was last seen while dragging */
  QString m_description;
  QImage m_image; /* The frame on screen */
  int m_frame = 0;
  double m_time = 0.0;
  double m_renderMs = 0.0;

  QTimer m_titleTimer;
  QElapsedTimer m_rateTimer;
  long m_displayed = 0; /* Frames put on screen ... */
  long m_lastDisplayed = 0;
  long m_lastRendered = 0; /* ... and rendered, at the last title update */
};

#endif /* ANIMVIEW_H */
