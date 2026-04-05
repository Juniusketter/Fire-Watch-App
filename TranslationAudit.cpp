#include "TranslationAudit.h"
#include "TranslationStore.h"
#include <QSet>

int runTranslationAudit(QObject* root, int& totalKeys, int& missing)
{
    QSet<QString> used;
    std::function<void(QObject*)> walk = [&](QObject* o) {
        QVariant v = o->property("trKey");
        if (v.isValid()) used.insert(v.toString());
        for (QObject* c : o->children()) walk(c);
    };

    walk(root);
    totalKeys = used.size();
    missing = 0;

    for (const QString& k : used)
        if (TranslationStore::translate(k) == k)
            missing++;

    return totalKeys == 0 ? 100 : ((totalKeys - missing) * 100) / totalKeys;
}
