#include "TranslationStore.h"
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>

QMap<QString, QString> TranslationStore::translations;

bool TranslationStore::loadLanguage(const QString& langCode)
{
    QFile file(QString(":/i18n/%1.json").arg(langCode));
    if (!file.open(QIODevice::ReadOnly))
        return false;

    translations.clear();

    QJsonObject obj = QJsonDocument::fromJson(file.readAll()).object();
    for (auto it = obj.begin(); it != obj.end(); ++it)
        translations[it.key()] = it.value().toString();

    return true;
}

QString TranslationStore::translate(const QString& key)
{
    return translations.contains(key) ? translations[key] : key;
}
