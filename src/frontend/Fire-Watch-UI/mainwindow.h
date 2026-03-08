#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QString>
#include <QtSql/QSqlDatabase>
#include <QtSql/QSqlQuery>
#include <QtSql/QSqlError>
#include "../../backend/User.h"
#include "../../backend/Admin.h"
#include "../../backend/Inv.h"

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void onLoginClicked();

private:
    Ui::MainWindow *ui;

    // Holds the logged-in user's info after successful login
    int    m_userId   = -1;
    QString m_username;
    QString m_role;

    // Opens the SQLite database, returns true on success
    bool openDatabase();
};

#endif // MAINWINDOW_H
