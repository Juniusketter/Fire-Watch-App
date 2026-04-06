#include <QtTest/QtTest>
#include "TranslationAudit.h"

class TestTranslationCoverage : public QObject {
    Q_OBJECT
private slots:
    void coverageMustBeComplete();
};

void TestTranslationCoverage::coverageMustBeComplete()
{
    QWidget dummy;
    dummy.setProperty("trKey", "lOut");

    int total = 0, missing = 0;
    int pct = runTranslationAudit(&dummy, total, missing);
    QVERIFY(pct == 100);
}

QTEST_MAIN(TestTranslationCoverage)
#include "TestTranslationCoverage.moc"
