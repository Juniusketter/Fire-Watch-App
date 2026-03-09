#include "dashboard.h"
#include "ui_dashboard.h"
#include "mainwindow.h"

#include <QHeaderView>
#include <QMessageBox>
#include <QtSql/QSqlQuery>
#include <QtSql/QSqlError>
#include <QtSql/QSqlRecord>

// ─────────────────────────────────────────────────────────────────────────────
//  Constructor
// ─────────────────────────────────────────────────────────────────────────────
Dashboard::Dashboard(int userId, const QString &username,
                     const QString &role, QSqlDatabase db,
                     QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::Dashboard)
    , m_userId(userId)
    , m_username(username)
    , m_role(role)
    , m_db(db)
{
    ui->setupUi(this);
    setWindowTitle("FireWatch — Dashboard");

    // Welcome bar
    ui->labelWelcome->setText("Welcome, " + m_username);
    ui->labelRole->setText("Role: " + m_role);

    // Style the role label colour by role
    QString roleColour = "#b07d2a";  // default amber (Admin)
    if (isInv())         roleColour = "#1a6b2e";  // green
    if (isThirdPAdmin()) roleColour = "#1a4a8a";  // blue
    if (isThirdPInv())   roleColour = "#5a3080";  // purple
    ui->labelRole->setStyleSheet("color: " + roleColour + "; font-weight: bold;");

    // Wire buttons
    connect(ui->btnLogout,  &QPushButton::clicked, this, &Dashboard::onLogoutClicked);
    connect(ui->btnRefresh, &QPushButton::clicked, this, &Dashboard::onRefreshClicked);

    // Build the tab set for this role, then load data
    setupTabsForRole();
    updateStats();
}

Dashboard::~Dashboard()
{
    delete ui;
}

// ─────────────────────────────────────────────────────────────────────────────
//  setupTabsForRole  — show/hide tabs and set window title based on role
//
//  Tab indices in the .ui file:
//    0 = Extinguishers
//    1 = Assignments
//    2 = Reports
//    3 = Users
//
//  Role matrix:
//    Admin           → Extinguishers | Assignments | Reports | Users (all users)
//    Investigator    → Assignments (own) | Reports (own)
//    ThirdPAdmin     → Assignments (all) | Users (team members only)
//    ThirdPInv       → Assignments (own) only
// ─────────────────────────────────────────────────────────────────────────────
void Dashboard::setupTabsForRole()
{
    // Start by loading everything into existing tabs, then hide what
    // this role shouldn't see.  We remove tabs from highest index first
    // so indices don't shift during removal.

    if (isAdmin()) {
        // ── Admin: sees everything ────────────────────────────────────────
        setWindowTitle("FireWatch — Admin Dashboard");
        loadExtinguishers();
        loadAssignments();
        loadReports();
        loadUsers();
        // Rename Users tab to reflect all users
        ui->tabWidget->setTabText(3, "All Users");
    }
    else if (isInv()) {
        // ── Investigator: own assignments + own reports ───────────────────
        setWindowTitle("FireWatch — Inspector Dashboard");
        // Remove Users tab (index 3), then Extinguishers (index 0)
        ui->tabWidget->removeTab(3); // Users
        ui->tabWidget->removeTab(0); // Extinguishers
        // Remaining tabs are now: [0]=Assignments [1]=Reports
        loadAssignments();  // filtered to m_userId
        loadReports();      // filtered to m_userId
        ui->tabWidget->setTabText(0, "My Assignments");
        ui->tabWidget->setTabText(1, "My Reports");
    }
    else if (isThirdPAdmin()) {
        // ── Third-Party Admin: all assignments + their team ───────────────
        setWindowTitle("FireWatch — Third-Party Admin Dashboard");
        // Remove Reports (index 2) and Extinguishers (index 0)
        ui->tabWidget->removeTab(2); // Reports
        ui->tabWidget->removeTab(0); // Extinguishers
        // Remaining tabs: [0]=Assignments [1]=Users
        loadAssignments();  // all assignments
        loadUsers();        // team members (ThirdPInv with same company)
        ui->tabWidget->setTabText(0, "Assignments");
        ui->tabWidget->setTabText(1, "My Team");
    }
    else if (isThirdPInv()) {
        // ── Third-Party Investigator: own assignments only ────────────────
        setWindowTitle("FireWatch — Third-Party Inspector Dashboard");
        // Remove Users (3), Reports (2), Extinguishers (0)
        ui->tabWidget->removeTab(3); // Users
        ui->tabWidget->removeTab(2); // Reports
        ui->tabWidget->removeTab(0); // Extinguishers
        // Remaining tab: [0]=Assignments
        loadAssignments();  // filtered to m_userId
        ui->tabWidget->setTabText(0, "My Assignments");
    }
    else {
        // ── Unknown/fallback: show assignments only (safe default) ────────
        setWindowTitle("FireWatch — Dashboard");
        ui->tabWidget->removeTab(3);
        ui->tabWidget->removeTab(2);
        ui->tabWidget->removeTab(0);
        loadAssignments();
    }
}

// ─────────────────────────────────────────────────────────────────────────────
//  Logout
// ─────────────────────────────────────────────────────────────────────────────
void Dashboard::onLogoutClicked()
{
    MainWindow *loginWindow = new MainWindow();
    loginWindow->show();
    this->close();
}

// ─────────────────────────────────────────────────────────────────────────────
//  Refresh — reload data for this role
// ─────────────────────────────────────────────────────────────────────────────
void Dashboard::onRefreshClicked()
{
    updateStats();

    if (isAdmin()) {
        loadExtinguishers();
        loadAssignments();
        loadReports();
        loadUsers();
    }
    else if (isInv()) {
        loadAssignments();
        loadReports();
    }
    else if (isThirdPAdmin()) {
        loadAssignments();
        loadUsers();
    }
    else if (isThirdPInv()) {
        loadAssignments();
    }
}

// ─────────────────────────────────────────────────────────────────────────────
//  Stats row — counts shown to all roles (scoped where appropriate)
// ─────────────────────────────────────────────────────────────────────────────
void Dashboard::updateStats()
{
    // Helper lambda for COUNT queries
    auto count = [&](const QString &sql) -> int {
        QSqlQuery q(m_db);
        q.prepare(sql);
        if (sql.contains(":userId"))
            q.bindValue(":userId", m_userId);
        q.exec();
        return q.next() ? q.value(0).toInt() : 0;
    };

    if (isAdmin()) {
        ui->statExtinguishers->setText(QString::number(
            count("SELECT COUNT(*) FROM Extinguishers")));
        ui->statAssignments->setText(QString::number(
            count("SELECT COUNT(*) FROM Assignments")));
        ui->statReports->setText(QString::number(
            count("SELECT COUNT(*) FROM Reports")));
        ui->statUsers->setText(QString::number(
            count("SELECT COUNT(*) FROM Users")));
    }
    else if (isInv()) {
        // Hide Extinguishers and Users stat boxes — not relevant
        ui->frameExt->setVisible(false);
        ui->frameUsers->setVisible(false);
        ui->statAssignments->setText(QString::number(
            count("SELECT COUNT(*) FROM Assignments WHERE inspector_id = :userId")));
        ui->statReports->setText(QString::number(
            count("SELECT COUNT(*) FROM Reports WHERE inspector_id = :userId")));
        // Rename stat labels
        ui->lblAssign->setText("My Assignments");
        ui->lblReports->setText("My Reports");
    }
    else if (isThirdPAdmin()) {
        ui->frameExt->setVisible(false);
        ui->frameReports->setVisible(false);
        ui->statAssignments->setText(QString::number(
            count("SELECT COUNT(*) FROM Assignments")));
        ui->statUsers->setText(QString::number(
            count("SELECT COUNT(*) FROM Users WHERE role = '3rd_Party_Inspector'")));
        ui->lblAssign->setText("Assignments");
        ui->lblUsers->setText("Team Members");
    }
    else if (isThirdPInv()) {
        ui->frameExt->setVisible(false);
        ui->frameReports->setVisible(false);
        ui->frameUsers->setVisible(false);
        ui->statAssignments->setText(QString::number(
            count("SELECT COUNT(*) FROM Assignments WHERE inspector_id = :userId")));
        ui->lblAssign->setText("My Assignments");
    }
}

// ─────────────────────────────────────────────────────────────────────────────
//  Data loaders
// ─────────────────────────────────────────────────────────────────────────────

void Dashboard::loadExtinguishers()
{
    // Admin only — full extinguisher list
    QSqlQuery q(m_db);
    q.exec("SELECT extinguisher_id, address, building_number, floor_number, "
           "room_number, location_description, inspection_interval_days, "
           "next_due_date FROM Extinguishers");
    fillTable(ui->tableExtinguishers, q);

    if (ui->tableExtinguishers->rowCount() == 0)
        showEmptyMessage(ui->tableExtinguishers, "No extinguishers in database yet.");
}

void Dashboard::loadAssignments()
{
    QSqlQuery q(m_db);

    if (isAdmin() || isThirdPAdmin()) {
        // Full assignment list with readable names
        q.exec("SELECT a.assignment_id, "
               "u_admin.username AS assigned_by, "
               "u_insp.username  AS inspector, "
               "a.extinguisher_id, a.status "
               "FROM Assignments a "
               "LEFT JOIN Users u_admin ON a.admin_id    = u_admin.user_id "
               "LEFT JOIN Users u_insp  ON a.inspector_id = u_insp.user_id");
    } else {
        // Investigator or ThirdPInv — own assignments only
        q.prepare("SELECT a.assignment_id, "
                  "u_admin.username AS assigned_by, "
                  "a.extinguisher_id, a.status "
                  "FROM Assignments a "
                  "LEFT JOIN Users u_admin ON a.admin_id = u_admin.user_id "
                  "WHERE a.inspector_id = :userId");
        q.bindValue(":userId", m_userId);
        q.exec();
    }

    fillTable(ui->tableAssignments, q);

    if (ui->tableAssignments->rowCount() == 0) {
        QString msg = (isAdmin() || isThirdPAdmin())
            ? "No assignments have been created yet."
            : "You have no assignments right now.";
        showEmptyMessage(ui->tableAssignments, msg);
    }
}

void Dashboard::loadReports()
{
    QSqlQuery q(m_db);

    if (isAdmin()) {
        // All reports with inspector name
        q.exec("SELECT r.report_id, r.extinguisher_id, "
               "u.username AS inspector, "
               "r.inspection_date, r.notes "
               "FROM Reports r "
               "LEFT JOIN Users u ON r.inspector_id = u.user_id");
    } else {
        // Investigator — own reports only
        q.prepare("SELECT report_id, extinguisher_id, "
                  "inspection_date, notes "
                  "FROM Reports "
                  "WHERE inspector_id = :userId");
        q.bindValue(":userId", m_userId);
        q.exec();
    }

    fillTable(ui->tableReports, q);

    if (ui->tableReports->rowCount() == 0) {
        QString msg = isAdmin()
            ? "No reports have been submitted yet."
            : "You have not submitted any reports yet.";
        showEmptyMessage(ui->tableReports, msg);
    }
}

void Dashboard::loadUsers()
{
    QSqlQuery q(m_db);

    if (isAdmin()) {
        // All users — never show password hashes
        q.exec("SELECT user_id, username, role FROM Users");
    } else if (isThirdPAdmin()) {
        // Only ThirdPartyInvestigators (same company — future: filter by company)
        q.exec("SELECT user_id, username, role FROM Users "
               "WHERE role = '3rd_Party_Inspector'");
    }

    fillTable(ui->tableUsers, q);

    if (ui->tableUsers->rowCount() == 0) {
        QString msg = isAdmin()
            ? "No users found."
            : "No team members found.";
        showEmptyMessage(ui->tableUsers, msg);
    }
}

// ─────────────────────────────────────────────────────────────────────────────
//  Generic table filler
// ─────────────────────────────────────────────────────────────────────────────
void Dashboard::fillTable(QTableWidget *table, QSqlQuery &query)
{
    table->setRowCount(0);

    QSqlRecord rec = query.record();
    int colCount = rec.count();
    table->setColumnCount(colCount);

    QStringList headers;
    for (int i = 0; i < colCount; ++i)
        headers << rec.fieldName(i);
    table->setHorizontalHeaderLabels(headers);

    int row = 0;
    while (query.next()) {
        table->insertRow(row);
        for (int col = 0; col < colCount; ++col) {
            QString val = query.value(col).isNull()
                              ? "—"
                              : query.value(col).toString();
            QTableWidgetItem *item = new QTableWidgetItem(val);
            item->setFlags(item->flags() & ~Qt::ItemIsEditable);
            table->setItem(row, col, item);
        }
        ++row;
    }

    table->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    table->verticalHeader()->setVisible(false);
}

// ─────────────────────────────────────────────────────────────────────────────
//  Show a placeholder message when a table has no rows
// ─────────────────────────────────────────────────────────────────────────────
void Dashboard::showEmptyMessage(QTableWidget *table, const QString &message)
{
    table->setRowCount(1);
    table->setColumnCount(1);
    table->setHorizontalHeaderLabels(QStringList() << "Status");
    QTableWidgetItem *item = new QTableWidgetItem(message);
    item->setFlags(item->flags() & ~Qt::ItemIsEditable);
    item->setTextAlignment(Qt::AlignCenter);
    table->setItem(0, 0, item);
    table->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    table->verticalHeader()->setVisible(false);
}
