#ifndef DASHBOARD_H
#define DASHBOARD_H

#include <QMainWindow>
#include <QString>
#include <QTableWidget>
#include <QLabel>
#include <QtSql/QSqlDatabase>
#include <QtSql/QSqlQuery>
#include <QtSql/QSqlError>

namespace Ui {
class Dashboard;
}

class Dashboard : public QMainWindow
{
    Q_OBJECT

public:
    // Called from MainWindow after successful login
    explicit Dashboard(int userId, const QString &username,
                       const QString &role, QSqlDatabase db,
                       QWidget *parent = nullptr);
    ~Dashboard();

private slots:
    void onLogoutClicked();
    void onRefreshClicked();

private:
    Ui::Dashboard *ui;

    int     m_userId;
    QString m_username;
    QString m_role;
    QSqlDatabase m_db;

    void loadExtinguishers();
    void loadAssignments();
    void loadReports();
    void loadUsers();
    void updateStats();

    // Helper: populate a QTableWidget from a query
    void fillTable(QTableWidget *table, QSqlQuery &query);
};

#endif // DASHBOARD_H
