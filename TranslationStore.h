#pragma once
#include <QString>
#include <QMap>

class TranslationStore {
public:
    static bool loadLanguage(const QString& langCode);
    static QString translate(const QString& key);

private:
    static QMap<QString, QString> translations;
};
