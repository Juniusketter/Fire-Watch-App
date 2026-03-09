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
    explicit Dashboard(int userId, const QString &username,
                       const QString &role, QSqlDatabase db,
                       QWidget *parent = nullptr);
    ~Dashboard();

private slots:
    void onLogoutClicked();
    void onRefreshClicked();

private:
    Ui::Dashboard *ui;

    int      m_userId;
    QString  m_username;
    QString  m_role;
    QSqlDatabase m_db;

    // ── Role helpers ──────────────────────────────────────────────────────
    bool isAdmin()       const { return m_role == "Admin"; }
    bool isInv()         const { return m_role == "Inspector"; }
    bool isThirdPAdmin() const { return m_role == "3rd_Party_Admin"; }
    bool isThirdPInv()   const { return m_role == "3rd_Party_Inspector"; }

    // ── Tab setup (called once in constructor) ────────────────────────────
    void setupTabsForRole();

    // ── Data loaders ─────────────────────────────────────────────────────
    void loadExtinguishers();
    void loadAssignments();
    void loadReports();
    void loadUsers();
    void updateStats();

    // ── Helpers ───────────────────────────────────────────────────────────
    void fillTable(QTableWidget *table, QSqlQuery &query);
    void showEmptyMessage(QTableWidget *table, const QString &message);
};

#endif // DASHBOARD_H
