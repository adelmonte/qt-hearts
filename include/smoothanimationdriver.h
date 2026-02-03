#ifndef SMOOTHANIMATIONDRIVER_H
#define SMOOTHANIMATIONDRIVER_H

#include <QAnimationDriver>
#include <QElapsedTimer>
#include <QTimer>

// Custom animation driver that runs at a higher frame rate than Qt's default 60fps.
// This enables smooth animations on high refresh rate displays (90Hz, 120Hz, 144Hz, etc.)
class SmoothAnimationDriver : public QAnimationDriver {
    Q_OBJECT
public:
    explicit SmoothAnimationDriver(int targetFps = 120, QObject* parent = nullptr);
    ~SmoothAnimationDriver() override;

    void start() override;
    void stop() override;

    qint64 elapsed() const override;

    void setTargetFps(int fps);
    int targetFps() const { return m_targetFps; }

protected:
    void timerEvent(QTimerEvent* event) override;

private:
    int m_targetFps;
    int m_timerId;
    QElapsedTimer m_timer;
    qint64 m_elapsed;
    bool m_running;
};

#endif // SMOOTHANIMATIONDRIVER_H
