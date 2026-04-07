
#include <QtTest/QtTest>
#include <QFile>
#include <QJsonDocument>

class TestTranslationParity : public QObject {
    Q_OBJECT
private slots:
    void fullParity();
};

void TestTranslationParity::fullParity() {
    QFile en(":/i18n/en.json");
    QFile es(":/i18n/es.json");
    QVERIFY(en.open(QIODevice::ReadOnly));
    QVERIFY(es.open(QIODevice::ReadOnly));
    QCOMPARE(QJsonDocument::fromJson(en.readAll()).object().keys().toSet(), QJsonDocument::fromJson(es.readAll()).object().keys().toSet());
}

QTEST_MAIN(TestTranslationParity)
#include "TestTranslationParity.moc"
