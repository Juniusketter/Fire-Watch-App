#ifndef DASHBOARD_H
#define DASHBOARD_H

#include <QMainWindow>
#include <QString>
#include <QTableWidget>
#include <QLabel>
#include <QtSql/QSqlDatabase>
#include <QtSql/QSqlQuery>
#include <QtSql/QSqlError>
#include "DueDateAlert.h"

namespace Ui {
class Dashboard;
}

class Dashboard : public QMainWindow
{
    Q_OBJECT

public:
    explicit Dashboard(int userId, const QString &username,
                       const QString &role, int orgId,
                       QSqlDatabase db,
                       QWidget *parent = nullptr);
    ~Dashboard();

private slots:
    void onLogoutClicked();
    void onRefreshClicked();

    // ── Admin CRUD slots ──────────────────────────────────────────────────
    void onAddExtinguisher();
    void onEditExtinguisher();
    void onDeleteExtinguisher();

    // ── Admin: Generate Assignment ────────────────────────────────────────
    void onGenerateAssignment();

    // ── Inspector: Generate Report ────────────────────────────────────────
    void onGenerateReport();

    // ── Admin: Add / Edit / Delete User ──────────────────────────────────
    void onAddUser();
    void onEditUser();
    void onDeleteUser();

    // ── Admin: Export Reports ────────────────────────────────────────────
    void onExportReports();

private:
    Ui::Dashboard *ui;

    int          m_userId;
    QString      m_username;
    QString      m_role;
    int          m_orgId;
    QSqlDatabase m_db;

    // ── Role helpers ──────────────────────────────────────────────────────
    bool isAdmin()       const { return m_role == "Admin"; }
    bool isInv()         const { return m_role == "Inspector"; }
    bool isThirdPAdmin() const { return m_role == "3rd_Party_Admin"; }
    bool isThirdPInv()    const { return m_role == "3rd_Party_Inspector"; }
    bool isClient()       const { return m_role == "Client"; }
    bool isPlatformAdmin() const { return m_role == "Platform_Admin"; }

    // ── Tab setup ─────────────────────────────────────────────────────────
    void setupTabsForRole();

    // ── Admin toolbar ─────────────────────────────────────────────────────
    void setupExtinguisherToolbar();

    // ── Assignment toolbar (Admin) ────────────────────────────────────────
    void setupAssignmentToolbar();

    // ── Report toolbar (Inspector) ────────────────────────────────────────
    void setupReportToolbar();

    // ── Report Export toolbar (Admin) ───────────────────────────────────
    void setupReportExportToolbar();

    // ── User toolbar (Admin) ──────────────────────────────────────────────
    void setupUserToolbar();

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

    // ── Returns user_id of selected row in Users table, -1 if none ───────
    int selectedUserId() const;

    // ── Due date alerts — called after loadAssignments() ─────────────────
    void checkDueDateAlerts();
};

#endif // DASHBOARD_H
