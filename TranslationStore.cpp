#include "TranslationStore.h"
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>

QMap<QString, QString> TranslationStore::translations;

bool TranslationStore::loadLanguage(const QString& lang)
{
    QFile f(QString(":/i18n/%1.json").arg(lang));
    if (!f.open(QIODevice::ReadOnly)) return false;

    translations.clear();
    QJsonObject obj = QJsonDocument::fromJson(f.readAll()).object();
    for (auto it = obj.begin(); it != obj.end(); ++it)
        translations[it.key()] = it.value().toString();
    return true;
}

QString TranslationStore::t(const QString& key)
{
    return translations.contains(key) ? translations[key] : key;
}
