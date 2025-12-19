#include "soundengine.h"
#include <QStandardPaths>
#include <QFileInfo>
#include <QDir>
#include <QCoreApplication>

SoundEngine::SoundEngine(QObject* parent)
    : QObject(parent)
    , m_enabled(true)
{
    // Try to find sounds in various locations
    // Prefer local data/sounds directory first (with WAV files)
    QStringList searchPaths = {
        QCoreApplication::applicationDirPath() + "/data/sounds",
        QCoreApplication::applicationDirPath() + "/sounds",
        QCoreApplication::applicationDirPath() + "/../data/sounds",
        QCoreApplication::applicationDirPath() + "/../share/qt-hearts/sounds",
        "/usr/share/qt-hearts/sounds",
        "/usr/local/share/qt-hearts/sounds",
        QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/sounds"
    };

    QString soundsPath;

    // Look for sounds (prefer WAV format for better compatibility)
    for (const QString& path : searchPaths) {
        if (QFileInfo::exists(path + "/card-pickup.wav") ||
            QFileInfo::exists(path + "/card-down.wav")) {
            soundsPath = path;
            break;
        }
    }

    // Load sounds
    if (!soundsPath.isEmpty()) {
        loadSoundWithFallback(m_cardPickup, soundsPath, "card-pickup");
        loadSoundWithFallback(m_cardPutDown, soundsPath, "card-down");
        loadSoundWithFallback(m_cardShuffle, soundsPath, "card-shuffle");
        loadSoundWithFallback(m_win, soundsPath, "win");
        loadSoundWithFallback(m_lose, soundsPath, "lose");
    }
}

void SoundEngine::loadSound(std::unique_ptr<QSoundEffect>& effect, const QString& filename) {
    if (QFileInfo::exists(filename)) {
        effect = std::make_unique<QSoundEffect>(this);
        effect->setSource(QUrl::fromLocalFile(filename));
        effect->setVolume(1.0);
    }
}

void SoundEngine::loadSoundWithFallback(std::unique_ptr<QSoundEffect>& effect, const QString& basePath, const QString& baseName) {
    // Try different file extensions - prefer WAV for better Qt compatibility
    QStringList extensions = {"wav", "ogg", "mp3", "flac"};
    for (const QString& ext : extensions) {
        QString filename = basePath + "/" + baseName + "." + ext;
        if (QFileInfo::exists(filename)) {
            effect = std::make_unique<QSoundEffect>(this);
            effect->setSource(QUrl::fromLocalFile(filename));
            effect->setVolume(1.0);
            return;
        }
    }
}

void SoundEngine::playCardPickup() {
    if (m_enabled && m_cardPickup) {
        m_cardPickup->play();
    }
}

void SoundEngine::playCardPutDown() {
    if (m_enabled && m_cardPutDown) {
        m_cardPutDown->play();
    }
}

void SoundEngine::playCardShuffle() {
    if (m_enabled && m_cardShuffle) {
        m_cardShuffle->play();
    }
}

void SoundEngine::playWin() {
    if (m_enabled && m_win) {
        m_win->play();
    }
}

void SoundEngine::playLose() {
    if (m_enabled && m_lose) {
        m_lose->play();
    }
}
