#ifndef SOUNDENGINE_H
#define SOUNDENGINE_H

#include <QObject>
#include <QSoundEffect>
#include <memory>

class SoundEngine : public QObject {
    Q_OBJECT

public:
    explicit SoundEngine(QObject* parent = nullptr);

    bool isEnabled() const { return m_enabled; }
    void setEnabled(bool enabled) { m_enabled = enabled; }

public slots:
    void playCardPickup();
    void playCardPutDown();
    void playCardShuffle();
    void playWin();
    void playLose();

private:
    void loadSound(std::unique_ptr<QSoundEffect>& effect, const QString& filename);
    void loadSoundWithFallback(std::unique_ptr<QSoundEffect>& effect, const QString& basePath, const QString& baseName);

    bool m_enabled;
    std::unique_ptr<QSoundEffect> m_cardPickup;
    std::unique_ptr<QSoundEffect> m_cardPutDown;
    std::unique_ptr<QSoundEffect> m_cardShuffle;
    std::unique_ptr<QSoundEffect> m_win;
    std::unique_ptr<QSoundEffect> m_lose;
};

#endif // SOUNDENGINE_H
