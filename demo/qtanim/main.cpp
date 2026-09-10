/**
 ** main.cpp - The interactive animation demo: renders the somersaulting
 **            teapot of demo/animation in a loop, into a window.
 **
 ** The rendering runs on a background thread (RenderThread) and the
 ** window (AnimView) shows the frames as they are finished.  This is a
 ** way to see how fast SIPP renders when no image files are involved.
 **/

#include <QApplication>
#include <QCommandLineParser>

#include <cstdio>

#include "animview.h"
#include "renderthread.h"

int main(int argc, char **argv) {
  QApplication app(argc, argv);
  QCoreApplication::setApplicationName("qtanim");

  QCommandLineParser parser;
  parser.setApplicationDescription(
      "Renders the SIPP teapot animation in a loop, into a window.");
  parser.addHelpOption();
  QCommandLineOption sizeOpt({"s", "size"}, "Image size in pixels (256).",
                             "n", "256");
  QCommandLineOption threadsOpt({"j", "threads"},
                                "Rendering threads, 0 = one per core (0).",
                                "n", "0");
  QCommandLineOption overOpt({"o", "oversampling"},
                             "Oversampling factor (2).", "n", "2");
  QCommandLineOption onceOpt({"a", "shade-once"},
                             "Shade each polygon once per pixel.");
  QCommandLineOption phongOpt({"p", "phong"}, "PHONG shading (default).");
  QCommandLineOption gouraudOpt({"g", "gouraud"}, "GOURAUD shading.");
  QCommandLineOption flatOpt({"f", "flat"}, "FLAT shading.");
  QCommandLineOption lineOpt({"l", "line"}, "Line drawing.");
  QCommandLineOption periodOpt(
      {"t", "period"},
      "Seconds one loop of the animation takes; as many frames as can be "
      "rendered in that time are shown.  0 renders the animation's own "
      "frames as fast as possible (1.0).",
      "s", "1.0");
  QCommandLineOption maxFpsOpt("max-fps",
                               "Render at most n frames per second (0 = "
                               "no limit).",
                               "n", "0");
  QCommandLineOption loopsOpt("loops", "Quit after n loops, 0 = never (0).",
                              "n", "0");
  QCommandLineOption dumpOpt("dump", "Write the first loop's frames as PNG "
                                     "files into this directory.",
                             "dir");
  parser.addOptions({sizeOpt, threadsOpt, overOpt, onceOpt, phongOpt,
                     gouraudOpt, flatOpt, lineOpt, periodOpt, maxFpsOpt,
                     loopsOpt, dumpOpt});
  parser.process(app);

  RenderSettings settings;
  settings.size = parser.value(sizeOpt).toInt();
  settings.threads = parser.value(threadsOpt).toInt();
  settings.oversampling = parser.value(overOpt).toInt();
  settings.shadeOnce = parser.isSet(onceOpt);
  settings.period = parser.value(periodOpt).toDouble();
  settings.maxFps = parser.value(maxFpsOpt).toInt();
  settings.loops = parser.value(loopsOpt).toInt();
  settings.dumpDir = parser.value(dumpOpt);
  const char *modeName = "PHONG";
  if (parser.isSet(gouraudOpt)) {
    settings.mode = GOURAUD;
    modeName = "GOURAUD";
  } else if (parser.isSet(flatOpt)) {
    settings.mode = FLAT;
    modeName = "FLAT";
  } else if (parser.isSet(lineOpt)) {
    settings.mode = LINE;
    modeName = "LINE";
  }
  if (settings.size < 8 || settings.oversampling < 1 ||
      settings.period < 0.0 || settings.maxFps < 0) {
    std::fprintf(stderr, "qtanim: bad size, oversampling, period or "
                         "max-fps\n");
    return 1;
  }

  const QString description =
      QString("SIPP %1x%2 %3 x%4 -j %5%6, %7")
          .arg(settings.size)
          .arg(settings.size)
          .arg(modeName)
          .arg(settings.oversampling)
          .arg(settings.threads)
          .arg(settings.shadeOnce ? " -a" : "")
          .arg(settings.period > 0.0
                   ? QString("%1 s per loop").arg(settings.period)
                   : QString("frame stepped"));

  RenderThread renderer(settings);
  AnimView view(&renderer, description);

  QObject::connect(&renderer, &RenderThread::loopDone,
                   [](int loop, int frames, double seconds) {
                     std::printf("loop %d: %d frames in %.3f s, %.1f ms/frame, "
                                 "%.1f fps\n",
                                 loop, frames, seconds,
                                 1000.0 * seconds / frames, frames / seconds);
                     std::fflush(stdout);
                   });
  if (settings.loops > 0) {
    QObject::connect(&renderer, &QThread::finished, &app,
                     &QCoreApplication::quit);
  }

  std::printf("Drag to orbit, scroll to zoom, R to reset the view.\n");
  view.show();
  renderer.start();

  const int status = app.exec();

  renderer.stop();
  renderer.wait();
  return status;
}
