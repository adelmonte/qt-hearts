#ifndef CARDIMAGEPROVIDER_H
#define CARDIMAGEPROVIDER_H

#include "cardtheme.h"
#include <QQuickImageProvider>

class CardImageProvider : public QQuickImageProvider {
public:
    CardImageProvider(CardTheme* theme);

    QPixmap requestPixmap(const QString& id, QSize* size, const QSize& requestedSize) override;

    void setTheme(CardTheme* theme) { m_theme = theme; }

private:
    CardTheme* m_theme;
};

#endif // CARDIMAGEPROVIDER_H
