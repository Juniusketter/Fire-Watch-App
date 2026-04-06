#include "LanguageManager.h"
#include <QSettings>

static QString g_lang = "en";

void LanguageManager::setLanguage(const QString& lang)
{
    g_lang = lang;
    QSettings s("Firewatch", "FirewatchApp");
    s.setValue("language", lang);
}

QString LanguageManager::currentLanguage()
{
    QSettings s("Firewatch", "FirewatchApp");
    return s.value("language", g_lang).toString();
}
