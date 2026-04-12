#pragma once
#include <QString>
#include <QMap>

class TranslationStore {
public:
    static bool loadLanguage(const QString& lang);
    static QString t(const QString& key);

private:
    static QMap<QString, QString> translations;
};
