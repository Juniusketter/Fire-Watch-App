#pragma once
#include <QString>

class LanguageManager {
public:
    static void setLanguage(const QString& lang);
    static QString currentLanguage();
};
