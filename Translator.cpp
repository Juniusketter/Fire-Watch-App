#include "Translator.h"
#include "TranslationStore.h"
#include <QLabel>
#include <QAbstractButton>
#include <QAction>
#include <QWidget>

void applyTranslations(QObject* root)
{
    if (!root) return;

    QVariant key = root->property("trKey");
    if (key.isValid()) {
        QString text = TranslationStore::t(key.toString());
        if (auto* l = qobject_cast<QLabel*>(root)) l->setText(text);
        else if (auto* b = qobject_cast<QAbstractButton*>(root)) b->setText(text);
        else if (auto* a = qobject_cast<QAction*>(root)) a->setText(text);
        else if (auto* w = qobject_cast<QWidget*>(root)) w->setWindowTitle(text);
    }
    for (QObject* c : root->children()) applyTranslations(c);
}
