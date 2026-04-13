#include <QtTest/QtTest>
#include "PasswordPolicy.h"

class TestPasswordPolicy : public QObject
{
    Q_OBJECT
private slots:
    void validPasswords();
    void invalidCases();
    void errorMessages();
};

void TestPasswordPolicy::validPasswords()
{
    QVERIFY(isValidPassword("Fire$Check88"));
    QVERIFY(isValidPassword("iNspect!42A"));
}

void TestPasswordPolicy::invalidCases()
{
    QVERIFY(!isValidPassword("short1!"));
    QVERIFY(!isValidPassword("Fire123!Test"));
}

void TestPasswordPolicy::errorMessages()
{
    QCOMPARE(QString::fromStdString(passwordValidationMessage("Fire123!Test")),
             QString("Password may not contain more than 2 consecutive numbers."));
}

QTEST_MAIN(TestPasswordPolicy)
#include "TestPasswordPolicy.moc"
