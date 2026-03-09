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

    // ── Admin CRUD slots ──────────────────────────────────────────────────
    void onAddExtinguisher();
    void onEditExtinguisher();
    void onDeleteExtinguisher();

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

    // ── Tab setup ─────────────────────────────────────────────────────────
    void setupTabsForRole();

    // ── Admin toolbar ─────────────────────────────────────────────────────
    void setupExtinguisherToolbar();

    // ── Data loaders ─────────────────────────────────────────────────────
    void loadExtinguishers();
    void loadAssignments();
    void loadReports();
    void loadUsers();
    void updateStats();

    // ── Helpers ───────────────────────────────────────────────────────────
    void fillTable(QTableWidget *table, QSqlQuery &query);
    void showEmptyMessage(QTableWidget *table, const QString &message);

    // ── Returns extinguisher_id of selected row, -1 if none ──────────────
    int selectedExtinguisherId() const;
};

#endif // DASHBOARD_H
