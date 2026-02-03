#include "smoothanimationdriver.h"
#include <QCoreApplication>

SmoothAnimationDriver::SmoothAnimationDriver(int targetFps, QObject* parent)
    : QAnimationDriver(parent)
    , m_targetFps(targetFps)
    , m_timerId(0)
    , m_elapsed(0)
    , m_running(false)
{
}

SmoothAnimationDriver::~SmoothAnimationDriver() {
    if (m_timerId) {
        killTimer(m_timerId);
    }
}

void SmoothAnimationDriver::start() {
    if (m_running) return;

    m_running = true;
    m_elapsed = 0;
    m_timer.start();

    // Use high-precision timer at target fps interval
    int intervalMs = 1000 / m_targetFps;
    if (intervalMs < 1) intervalMs = 1;
    m_timerId = startTimer(intervalMs, Qt::PreciseTimer);

    QAnimationDriver::start();
}

void SmoothAnimationDriver::stop() {
    if (!m_running) return;

    m_running = false;
    if (m_timerId) {
        killTimer(m_timerId);
        m_timerId = 0;
    }

    QAnimationDriver::stop();
}

qint64 SmoothAnimationDriver::elapsed() const {
    if (!m_running) return m_elapsed;
    return m_timer.elapsed();
}

void SmoothAnimationDriver::setTargetFps(int fps) {
    m_targetFps = qBound(30, fps, 240);

    // If running, restart timer with new interval
    if (m_running && m_timerId) {
        killTimer(m_timerId);
        int intervalMs = 1000 / m_targetFps;
        if (intervalMs < 1) intervalMs = 1;
        m_timerId = startTimer(intervalMs, Qt::PreciseTimer);
    }
}

void SmoothAnimationDriver::timerEvent(QTimerEvent* event) {
    if (event->timerId() == m_timerId && m_running) {
        // Advance all animations
        advance();
    }
}
