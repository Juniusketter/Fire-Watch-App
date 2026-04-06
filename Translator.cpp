#include "Translator.h"
#include "TranslationStore.h"
#include <QWidget>
#include <QLabel>
#include <QAction>
#include <QAbstractButton>

void applyTranslations(QObject* root)
{
    if (!root) return;

    QVariant v = root->property("trKey");
    if (v.isValid()) {
        QString text = TranslationStore::translate(v.toString());
        if (auto* l = qobject_cast<QLabel*>(root)) l->setText(text);
        if (auto* b = qobject_cast<QAbstractButton*>(root)) b->setText(text);
        if (auto* a = qobject_cast<QAction*>(root)) a->setText(text);
        if (auto* w = qobject_cast<QWidget*>(root)) w->setWindowTitle(text);
    }

    for (QObject* c : root->children())
        applyTranslations(c);
}
