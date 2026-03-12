#include "dashboard.h"
#include "ui_dashboard.h"
#include "mainwindow.h"
#include "extdialog.h"
#include "assigndialog.h"
#include "reportdialog.h"
#include "adduserdialog.h"

#include <QHeaderView>
#include <QMessageBox>
#include <QPushButton>
#include <QHBoxLayout>
#include <QWidget>
#include <QCryptographicHash>
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

    ui->labelWelcome->setText("Welcome, " + m_username);
    ui->labelRole->setText("Role: " + m_role);

    QString roleColour = "#b07d2a";
    if (isInv())         roleColour = "#1a6b2e";
    if (isThirdPAdmin()) roleColour = "#1a4a8a";
    if (isThirdPInv())   roleColour = "#5a3080";
    ui->labelRole->setStyleSheet("color: " + roleColour + "; font-weight: bold;");

    connect(ui->btnLogout,  &QPushButton::clicked, this, &Dashboard::onLogoutClicked);
    connect(ui->btnRefresh, &QPushButton::clicked, this, &Dashboard::onRefreshClicked);

    setupTabsForRole();
    updateStats();
}

Dashboard::~Dashboard() { delete ui; }

// ─────────────────────────────────────────────────────────────────────────────
//  Tab setup by role
// ─────────────────────────────────────────────────────────────────────────────
void Dashboard::setupTabsForRole()
{
    if (isAdmin()) {
        setWindowTitle("FireWatch — Admin Dashboard");
        setupExtinguisherToolbar();
        setupAssignmentToolbar();
        setupUserToolbar();
        loadExtinguishers();
        loadAssignments();
        loadReports();
        loadUsers();
        ui->tabWidget->setTabText(3, "All Users");
    }
    else if (isInv()) {
        setWindowTitle("FireWatch — Inspector Dashboard");
        ui->tabWidget->removeTab(3);
        ui->tabWidget->removeTab(0);
        setupReportToolbar();
        loadAssignments();
        loadReports();
        ui->tabWidget->setTabText(0, "My Assignments");
        ui->tabWidget->setTabText(1, "My Reports");
    }
    else if (isThirdPAdmin()) {
        setWindowTitle("FireWatch — Third-Party Admin Dashboard");
        ui->tabWidget->removeTab(2);
        ui->tabWidget->removeTab(0);
        setupAssignmentToolbar();
        loadAssignments();
        loadUsers();
        ui->tabWidget->setTabText(0, "Assignments");
        ui->tabWidget->setTabText(1, "My Team");
    }
    else if (isThirdPInv()) {
        setWindowTitle("FireWatch — Third-Party Inspector Dashboard");
        ui->tabWidget->removeTab(3);
        ui->tabWidget->removeTab(2);
        ui->tabWidget->removeTab(0);
        setupReportToolbar();
        loadAssignments();
        ui->tabWidget->setTabText(0, "My Assignments");
    }
    else {
        setWindowTitle("FireWatch — Dashboard");
        ui->tabWidget->removeTab(3);
        ui->tabWidget->removeTab(2);
        ui->tabWidget->removeTab(0);
        loadAssignments();
    }
}

// ─────────────────────────────────────────────────────────────────────────────
//  Admin extinguisher toolbar — Add / Edit / Delete
// ─────────────────────────────────────────────────────────────────────────────
void Dashboard::setupExtinguisherToolbar()
{
    QWidget     *bar   = new QWidget(this);
    QHBoxLayout *hbox  = new QHBoxLayout(bar);
    hbox->setContentsMargins(0, 4, 0, 4);
    hbox->setSpacing(8);

    QPushButton *btnAdd    = new QPushButton("＋ Add Extinguisher", bar);
    QPushButton *btnEdit   = new QPushButton("✎ Edit Selected",    bar);
    QPushButton *btnDelete = new QPushButton("✕ Delete Selected",  bar);

    QString baseStyle =
        "QPushButton { border-radius: 4px; padding: 5px 14px; font-weight: bold; }"
        "QPushButton:hover { opacity: 0.85; }";

    btnAdd->setStyleSheet(baseStyle +
        "QPushButton { background: #1a6b2e; color: white; }");
    btnEdit->setStyleSheet(baseStyle +
        "QPushButton { background: #1f3864; color: white; }");
    btnDelete->setStyleSheet(baseStyle +
        "QPushButton { background: #8b2222; color: white; }");

    hbox->addWidget(btnAdd);
    hbox->addWidget(btnEdit);
    hbox->addWidget(btnDelete);
    hbox->addStretch();

    QVBoxLayout *tabLayout = qobject_cast<QVBoxLayout*>(
        ui->tabExtinguishers->layout());
    if (tabLayout)
        tabLayout->insertWidget(0, bar);

    connect(btnAdd,    &QPushButton::clicked, this, &Dashboard::onAddExtinguisher);
    connect(btnEdit,   &QPushButton::clicked, this, &Dashboard::onEditExtinguisher);
    connect(btnDelete, &QPushButton::clicked, this, &Dashboard::onDeleteExtinguisher);
}

// ─────────────────────────────────────────────────────────────────────────────
//  Assignment toolbar — Generate Assignment button (Admin / ThirdPAdmin)
// ─────────────────────────────────────────────────────────────────────────────
void Dashboard::setupAssignmentToolbar()
{
    QWidget     *bar  = new QWidget(this);
    QHBoxLayout *hbox = new QHBoxLayout(bar);
    hbox->setContentsMargins(0, 4, 0, 4);
    hbox->setSpacing(8);

    QPushButton *btnGenerate = new QPushButton("＋ Generate Assignment", bar);
    btnGenerate->setStyleSheet(
        "QPushButton { border-radius: 4px; padding: 5px 14px; font-weight: bold;"
        " background: #1a6b2e; color: white; }"
        "QPushButton:hover { opacity: 0.85; }");

    hbox->addWidget(btnGenerate);
    hbox->addStretch();

    // Find the Assignments tab and insert toolbar
    // Tab index depends on role — use tabWidget to find by name
    for (int i = 0; i < ui->tabWidget->count(); ++i) {
        if (ui->tabWidget->tabText(i).contains("Assignment", Qt::CaseInsensitive)) {
            QVBoxLayout *tabLayout = qobject_cast<QVBoxLayout*>(
                ui->tabWidget->widget(i)->layout());
            if (tabLayout)
                tabLayout->insertWidget(0, bar);
            break;
        }
    }

    connect(btnGenerate, &QPushButton::clicked, this, &Dashboard::onGenerateAssignment);
}

// ─────────────────────────────────────────────────────────────────────────────
//  Report toolbar — Generate Report button (Inspector / ThirdPInv)
// ─────────────────────────────────────────────────────────────────────────────
void Dashboard::setupReportToolbar()
{
    QWidget     *bar  = new QWidget(this);
    QHBoxLayout *hbox = new QHBoxLayout(bar);
    hbox->setContentsMargins(0, 4, 0, 4);
    hbox->setSpacing(8);

    QPushButton *btnReport = new QPushButton("＋ Submit Report", bar);
    btnReport->setStyleSheet(
        "QPushButton { border-radius: 4px; padding: 5px 14px; font-weight: bold;"
        " background: #1f3864; color: white; }"
        "QPushButton:hover { opacity: 0.85; }");

    hbox->addWidget(btnReport);
    hbox->addStretch();

    // Insert into Reports tab
    for (int i = 0; i < ui->tabWidget->count(); ++i) {
        if (ui->tabWidget->tabText(i).contains("Report", Qt::CaseInsensitive)) {
            QVBoxLayout *tabLayout = qobject_cast<QVBoxLayout*>(
                ui->tabWidget->widget(i)->layout());
            if (tabLayout)
                tabLayout->insertWidget(0, bar);
            break;
        }
    }

    connect(btnReport, &QPushButton::clicked, this, &Dashboard::onGenerateReport);
}

// ─────────────────────────────────────────────────────────────────────────────
//  Logout / Refresh
// ─────────────────────────────────────────────────────────────────────────────
void Dashboard::onLogoutClicked()
{
    MainWindow *w = new MainWindow();
    w->show();
    this->close();
}

void Dashboard::onRefreshClicked()
{
    updateStats();
    if (isAdmin())            { loadExtinguishers(); loadAssignments(); loadReports(); loadUsers(); }
    else if (isInv())         { loadAssignments(); loadReports(); }
    else if (isThirdPAdmin()) { loadAssignments(); loadUsers(); }
    else if (isThirdPInv())   { loadAssignments(); }
}

// ─────────────────────────────────────────────────────────────────────────────
//  CRUD — Add Extinguisher
// ─────────────────────────────────────────────────────────────────────────────
void Dashboard::onAddExtinguisher()
{
    ExtDialog dlg("Add", this);
    if (dlg.exec() != QDialog::Accepted) return;

    QSqlQuery q(m_db);
    q.prepare(
        "INSERT INTO Extinguishers "
        "(address, building_number, floor_number, room_number, "
        " location_description, inspection_interval_days, next_due_date) "
        "VALUES (:addr, :bldg, :floor, :room, :loc, :interval, :due)"
    );
    q.bindValue(":addr",     dlg.address());
    q.bindValue(":bldg",     dlg.buildingNumber());
    q.bindValue(":floor",    dlg.floorNumber());
    q.bindValue(":room",     dlg.roomNumber());
    q.bindValue(":loc",      dlg.locationDescription());
    q.bindValue(":interval", dlg.inspectionInterval());
    q.bindValue(":due",      dlg.nextDueDate());

    if (q.exec()) {
        QMessageBox::information(this, "Success", "Extinguisher added successfully.");
        loadExtinguishers();
        updateStats();
    } else {
        QMessageBox::critical(this, "Database Error",
            "Failed to add extinguisher:\n" + q.lastError().text());
    }
}

// ─────────────────────────────────────────────────────────────────────────────
//  CRUD — Edit Extinguisher
// ─────────────────────────────────────────────────────────────────────────────
void Dashboard::onEditExtinguisher()
{
    int extId = selectedExtinguisherId();
    if (extId < 0) {
        QMessageBox::warning(this, "No Selection",
            "Please select an extinguisher row to edit.");
        return;
    }

    QSqlQuery fetch(m_db);
    fetch.prepare("SELECT * FROM Extinguishers WHERE extinguisher_id = :id");
    fetch.bindValue(":id", extId);
    fetch.exec();
    if (!fetch.next()) return;

    ExtDialog dlg("Save Changes", this);
    dlg.setAddress(fetch.value("address").toString());
    dlg.setBuildingNumber(fetch.value("building_number").toString());
    dlg.setFloorNumber(fetch.value("floor_number").toInt());
    dlg.setRoomNumber(fetch.value("room_number").toString());
    dlg.setLocationDescription(fetch.value("location_description").toString());
    dlg.setInspectionInterval(fetch.value("inspection_interval_days").toInt());
    dlg.setNextDueDate(fetch.value("next_due_date").toString());

    if (dlg.exec() != QDialog::Accepted) return;

    QSqlQuery q(m_db);
    q.prepare(
        "UPDATE Extinguishers SET "
        "address = :addr, building_number = :bldg, floor_number = :floor, "
        "room_number = :room, location_description = :loc, "
        "inspection_interval_days = :interval, next_due_date = :due "
        "WHERE extinguisher_id = :id"
    );
    q.bindValue(":addr",     dlg.address());
    q.bindValue(":bldg",     dlg.buildingNumber());
    q.bindValue(":floor",    dlg.floorNumber());
    q.bindValue(":room",     dlg.roomNumber());
    q.bindValue(":loc",      dlg.locationDescription());
    q.bindValue(":interval", dlg.inspectionInterval());
    q.bindValue(":due",      dlg.nextDueDate());
    q.bindValue(":id",       extId);

    if (q.exec()) {
        QMessageBox::information(this, "Success", "Extinguisher updated successfully.");
        loadExtinguishers();
    } else {
        QMessageBox::critical(this, "Database Error",
            "Failed to update extinguisher:\n" + q.lastError().text());
    }
}

// ─────────────────────────────────────────────────────────────────────────────
//  CRUD — Delete Extinguisher
// ─────────────────────────────────────────────────────────────────────────────
void Dashboard::onDeleteExtinguisher()
{
    int extId = selectedExtinguisherId();
    if (extId < 0) {
        QMessageBox::warning(this, "No Selection",
            "Please select an extinguisher row to delete.");
        return;
    }

    QSqlQuery fetch(m_db);
    fetch.prepare("SELECT address, building_number, floor_number "
                  "FROM Extinguishers WHERE extinguisher_id = :id");
    fetch.bindValue(":id", extId);
    fetch.exec();
    fetch.next();
    QString desc = fetch.value("address").toString() + " — "
                 + fetch.value("building_number").toString()
                 + " Floor " + fetch.value("floor_number").toString();

    auto reply = QMessageBox::warning(this, "Confirm Delete",
        "Are you sure you want to delete:\n\n" + desc +
        "\n\nThis will also remove all linked assignments and reports.",
        QMessageBox::Yes | QMessageBox::Cancel);

    if (reply != QMessageBox::Yes) return;

    QSqlQuery delReports(m_db);
    delReports.prepare("DELETE FROM Reports WHERE extinguisher_id = :id");
    delReports.bindValue(":id", extId);
    delReports.exec();

    QSqlQuery delAssign(m_db);
    delAssign.prepare("DELETE FROM Assignments WHERE extinguisher_id = :id");
    delAssign.bindValue(":id", extId);
    delAssign.exec();

    QSqlQuery q(m_db);
    q.prepare("DELETE FROM Extinguishers WHERE extinguisher_id = :id");
    q.bindValue(":id", extId);

    if (q.exec()) {
        QMessageBox::information(this, "Deleted", "Extinguisher deleted successfully.");
        loadExtinguishers();
        updateStats();
    } else {
        QMessageBox::critical(this, "Database Error",
            "Failed to delete:\n" + q.lastError().text());
    }
}

// ─────────────────────────────────────────────────────────────────────────────
//  Generate Assignment — DB-backed (Sprint 2)
//  Replaces the cin/cout stub in Admin.h with real DB writes
//  Author: Junius Ketter — Sprint 2
// ─────────────────────────────────────────────────────────────────────────────
void Dashboard::onGenerateAssignment()
{
    AssignDialog dlg(this);

    // Populate extinguishers dropdown
    QSqlQuery extQ(m_db);
    extQ.exec("SELECT extinguisher_id, address, building_number, floor_number "
              "FROM Extinguishers ORDER BY extinguisher_id");
    QList<QPair<int,QString>> extList;
    while (extQ.next()) {
        QString label = QString("#%1 — %2, %3 Fl.%4")
            .arg(extQ.value("extinguisher_id").toInt())
            .arg(extQ.value("address").toString())
            .arg(extQ.value("building_number").toString())
            .arg(extQ.value("floor_number").toInt());
        extList.append({ extQ.value("extinguisher_id").toInt(), label });
    }
    dlg.setExtinguishers(extList);

    // Populate inspectors dropdown (Inspector + 3rd_Party_Inspector roles)
    QSqlQuery invQ(m_db);
    invQ.exec("SELECT user_id, username, role FROM Users "
              "WHERE role IN ('Inspector','3rd_Party_Inspector') "
              "ORDER BY username");
    QList<QPair<int,QString>> invList;
    while (invQ.next()) {
        QString label = invQ.value("username").toString()
                      + "  [" + invQ.value("role").toString() + "]";
        invList.append({ invQ.value("user_id").toInt(), label });
    }
    dlg.setInspectors(invList);

    if (dlg.exec() != QDialog::Accepted) return;

    // Write assignment to DB
    QSqlQuery q(m_db);
    q.prepare(
        "INSERT INTO Assignments (admin_id, inspector_id, extinguisher_id, status) "
        "VALUES (:admin, :inspector, :ext, 'Pending Inspection')"
    );
    q.bindValue(":admin",    m_userId);
    q.bindValue(":inspector", dlg.inspectorId());
    q.bindValue(":ext",       dlg.extinguisherId());

    if (q.exec()) {
        QMessageBox::information(this, "Assignment Generated",
            "Assignment created successfully!\n\n"
            "Inspector has been notified.");
        loadAssignments();
        updateStats();
    } else {
        QMessageBox::critical(this, "Database Error",
            "Failed to create assignment:\n" + q.lastError().text());
    }
}

// ─────────────────────────────────────────────────────────────────────────────
//  Generate Report — DB-backed (Sprint 2)
//  Replaces the stub in Inv.h with real DB writes
//  Author: Junius Ketter — Sprint 2
// ─────────────────────────────────────────────────────────────────────────────
void Dashboard::onGenerateReport()
{
    ReportDialog dlg(this);

    // Populate assignments for this inspector
    QSqlQuery aQ(m_db);
    aQ.prepare(
        "SELECT a.assignment_id, a.extinguisher_id, e.address, e.building_number "
        "FROM Assignments a "
        "LEFT JOIN Extinguishers e ON a.extinguisher_id = e.extinguisher_id "
        "WHERE a.inspector_id = :userId AND a.status != 'Inspection Complete'"
    );
    aQ.bindValue(":userId", m_userId);
    aQ.exec();

    QList<QVariantList> assignList;
    while (aQ.next()) {
        QString label = QString("Assignment #%1 — %2, %3")
            .arg(aQ.value("assignment_id").toInt())
            .arg(aQ.value("address").toString())
            .arg(aQ.value("building_number").toString());
        assignList.append({
            aQ.value("assignment_id"),
            label,
            aQ.value("extinguisher_id")
        });
    }

    if (assignList.isEmpty()) {
        QMessageBox::information(this, "No Assignments",
            "You have no pending assignments to report on.");
        return;
    }
    dlg.setAssignments(assignList);

    // Populate extinguishers dropdown
    QSqlQuery extQ(m_db);
    extQ.exec("SELECT extinguisher_id, address, building_number FROM Extinguishers");
    QList<QPair<int,QString>> extList;
    while (extQ.next()) {
        QString label = QString("#%1 — %2, %3")
            .arg(extQ.value("extinguisher_id").toInt())
            .arg(extQ.value("address").toString())
            .arg(extQ.value("building_number").toString());
        extList.append({ extQ.value("extinguisher_id").toInt(), label });
    }
    dlg.setExtinguishers(extList);

    if (dlg.exec() != QDialog::Accepted) return;

    // Write report to DB
    QSqlQuery q(m_db);
    q.prepare(
        "INSERT INTO Reports (extinguisher_id, inspector_id, inspection_date, notes) "
        "VALUES (:ext, :inspector, :date, :notes)"
    );
    q.bindValue(":ext",       dlg.extinguisherId());
    q.bindValue(":inspector", m_userId);
    q.bindValue(":date",      dlg.inspectionDate());
    q.bindValue(":notes",     dlg.notes());

    if (!q.exec()) {
        QMessageBox::critical(this, "Database Error",
            "Failed to submit report:\n" + q.lastError().text());
        return;
    }

    // Update assignment status to 'Inspection Complete'
    QSqlQuery updateQ(m_db);
    updateQ.prepare(
        "UPDATE Assignments SET status = 'Inspection Complete' "
        "WHERE assignment_id = :id"
    );
    updateQ.bindValue(":id", dlg.assignmentId());
    updateQ.exec();

    QMessageBox::information(this, "Report Submitted",
        "Inspection report submitted successfully!\n\n"
        "Assignment marked as complete.");

    loadReports();
    loadAssignments();
    updateStats();
}

// ─────────────────────────────────────────────────────────────────────────────
//  Helper — get extinguisher_id from selected table row
// ─────────────────────────────────────────────────────────────────────────────
int Dashboard::selectedExtinguisherId() const
{
    int row = ui->tableExtinguishers->currentRow();
    if (row < 0) return -1;
    QTableWidgetItem *item = ui->tableExtinguishers->item(row, 0);
    if (!item) return -1;
    bool ok;
    int id = item->text().toInt(&ok);
    return ok ? id : -1;
}

// ─────────────────────────────────────────────────────────────────────────────
//  Stats
// ─────────────────────────────────────────────────────────────────────────────
void Dashboard::updateStats()
{
    auto count = [&](const QString &sql) -> int {
        QSqlQuery q(m_db);
        q.prepare(sql);
        if (sql.contains(":userId")) q.bindValue(":userId", m_userId);
        q.exec();
        return q.next() ? q.value(0).toInt() : 0;
    };

    if (isAdmin()) {
        ui->statExtinguishers->setText(QString::number(count("SELECT COUNT(*) FROM Extinguishers")));
        ui->statAssignments->setText(QString::number(count("SELECT COUNT(*) FROM Assignments")));
        ui->statReports->setText(QString::number(count("SELECT COUNT(*) FROM Reports")));
        ui->statUsers->setText(QString::number(count("SELECT COUNT(*) FROM Users")));
    }
    else if (isInv()) {
        ui->frameExt->setVisible(false);
        ui->frameUsers->setVisible(false);
        ui->statAssignments->setText(QString::number(count("SELECT COUNT(*) FROM Assignments WHERE inspector_id = :userId")));
        ui->statReports->setText(QString::number(count("SELECT COUNT(*) FROM Reports WHERE inspector_id = :userId")));
        ui->lblAssign->setText("My Assignments");
        ui->lblReports->setText("My Reports");
    }
    else if (isThirdPAdmin()) {
        ui->frameExt->setVisible(false);
        ui->frameReports->setVisible(false);
        ui->statAssignments->setText(QString::number(count("SELECT COUNT(*) FROM Assignments")));
        ui->statUsers->setText(QString::number(count("SELECT COUNT(*) FROM Users WHERE role = '3rd_Party_Inspector'")));
        ui->lblAssign->setText("Assignments");
        ui->lblUsers->setText("Team Members");
    }
    else if (isThirdPInv()) {
        ui->frameExt->setVisible(false);
        ui->frameReports->setVisible(false);
        ui->frameUsers->setVisible(false);
        ui->statAssignments->setText(QString::number(count("SELECT COUNT(*) FROM Assignments WHERE inspector_id = :userId")));
        ui->lblAssign->setText("My Assignments");
    }
}

// ─────────────────────────────────────────────────────────────────────────────
//  Data loaders
// ─────────────────────────────────────────────────────────────────────────────
void Dashboard::loadExtinguishers()
{
    QSqlQuery q(m_db);
    q.exec("SELECT extinguisher_id, address, building_number, floor_number, "
           "room_number, location_description, inspection_interval_days, "
           "next_due_date FROM Extinguishers");
    fillTable(ui->tableExtinguishers, q);
    if (ui->tableExtinguishers->rowCount() == 0)
        showEmptyMessage(ui->tableExtinguishers, "No extinguishers yet. Click '＋ Add Extinguisher' to get started.");
}

void Dashboard::loadAssignments()
{
    QSqlQuery q(m_db);
    if (isAdmin() || isThirdPAdmin()) {
        q.exec("SELECT a.assignment_id, u_admin.username AS assigned_by, "
               "u_insp.username AS inspector, a.extinguisher_id, a.status "
               "FROM Assignments a "
               "LEFT JOIN Users u_admin ON a.admin_id = u_admin.user_id "
               "LEFT JOIN Users u_insp  ON a.inspector_id = u_insp.user_id");
    } else {
        q.prepare("SELECT a.assignment_id, u_admin.username AS assigned_by, "
                  "a.extinguisher_id, a.status "
                  "FROM Assignments a "
                  "LEFT JOIN Users u_admin ON a.admin_id = u_admin.user_id "
                  "WHERE a.inspector_id = :userId");
        q.bindValue(":userId", m_userId);
        q.exec();
    }
    fillTable(ui->tableAssignments, q);
    if (ui->tableAssignments->rowCount() == 0)
        showEmptyMessage(ui->tableAssignments,
            (isAdmin() || isThirdPAdmin()) ? "No assignments yet." : "You have no assignments.");
}

void Dashboard::loadReports()
{
    QSqlQuery q(m_db);
    if (isAdmin()) {
        q.exec("SELECT r.report_id, r.extinguisher_id, u.username AS inspector, "
               "r.inspection_date, r.notes FROM Reports r "
               "LEFT JOIN Users u ON r.inspector_id = u.user_id");
    } else {
        q.prepare("SELECT report_id, extinguisher_id, inspection_date, notes "
                  "FROM Reports WHERE inspector_id = :userId");
        q.bindValue(":userId", m_userId);
        q.exec();
    }
    fillTable(ui->tableReports, q);
    if (ui->tableReports->rowCount() == 0)
        showEmptyMessage(ui->tableReports,
            isAdmin() ? "No reports submitted yet." : "You have not submitted any reports.");
}

void Dashboard::loadUsers()
{
    QSqlQuery q(m_db);
    if (isAdmin())
        q.exec("SELECT user_id, username, role FROM Users");
    else if (isThirdPAdmin())
        q.exec("SELECT user_id, username, role FROM Users WHERE role = '3rd_Party_Inspector'");
    fillTable(ui->tableUsers, q);
    if (ui->tableUsers->rowCount() == 0)
        showEmptyMessage(ui->tableUsers, isAdmin() ? "No users found." : "No team members found.");
}

// ─────────────────────────────────────────────────────────────────────────────
//  User toolbar — Add User button (Admin only)
// ─────────────────────────────────────────────────────────────────────────────
void Dashboard::setupUserToolbar()
{
    QWidget     *bar  = new QWidget(this);
    QHBoxLayout *hbox = new QHBoxLayout(bar);
    hbox->setContentsMargins(0, 4, 0, 4);
    hbox->setSpacing(8);

    QPushButton *btnAddUser = new QPushButton("＋ Add User", bar);
    btnAddUser->setStyleSheet(
        "QPushButton { border-radius: 4px; padding: 5px 14px; font-weight: bold;"
        " background: #15803d; color: white; }"
        "QPushButton:hover { opacity: 0.85; }");

    hbox->addWidget(btnAddUser);
    hbox->addStretch();

    for (int i = 0; i < ui->tabWidget->count(); ++i) {
        if (ui->tabWidget->tabText(i).contains("User", Qt::CaseInsensitive)) {
            QVBoxLayout *tabLayout = qobject_cast<QVBoxLayout*>(
                ui->tabWidget->widget(i)->layout());
            if (tabLayout)
                tabLayout->insertWidget(0, bar);
            break;
        }
    }

    connect(btnAddUser, &QPushButton::clicked, this, &Dashboard::onAddUser);
}

// ─────────────────────────────────────────────────────────────────────────────
//  Add User — DB-backed (Sprint 2)
//  Admin can create new users with hashed passwords
//  Author: Junius Ketter — Sprint 2
// ─────────────────────────────────────────────────────────────────────────────
void Dashboard::onAddUser()
{
    AddUserDialog dlg(this);
    if (dlg.exec() != QDialog::Accepted) return;

    // SHA-256 hash the password (matches Flask's hashlib.sha256)
    QString hashedPw = QString(
        QCryptographicHash::hash(
            dlg.password().toUtf8(),
            QCryptographicHash::Sha256
        ).toHex()
    );

    // Check for duplicate username
    QSqlQuery checkQ(m_db);
    checkQ.prepare("SELECT COUNT(*) FROM Users WHERE username = :u");
    checkQ.bindValue(":u", dlg.username());
    checkQ.exec();
    checkQ.next();
    if (checkQ.value(0).toInt() > 0) {
        QMessageBox::warning(this, "Duplicate Username",
            "A user with that username already exists.");
        return;
    }

    QSqlQuery q(m_db);
    q.prepare(
        "INSERT INTO Users (username, password_hash, role) VALUES (:u, :p, :r)"
    );
    q.bindValue(":u", dlg.username());
    q.bindValue(":p", hashedPw);
    q.bindValue(":r", dlg.role());

    if (q.exec()) {
        QMessageBox::information(this, "User Created",
            QString("User '%1' created successfully with role: %2")
                .arg(dlg.username()).arg(dlg.role()));
        loadUsers();
        updateStats();
    } else {
        QMessageBox::critical(this, "Database Error",
            "Failed to create user:\n" + q.lastError().text());
    }
}

// ─────────────────────────────────────────────────────────────────────────────
//  Helpers
// ─────────────────────────────────────────────────────────────────────────────
void Dashboard::fillTable(QTableWidget *table, QSqlQuery &query)
{
    table->setRowCount(0);
    QSqlRecord rec = query.record();
    int colCount = rec.count();
    table->setColumnCount(colCount);

    QStringList headers;
    for (int i = 0; i < colCount; ++i) headers << rec.fieldName(i);
    table->setHorizontalHeaderLabels(headers);

    int row = 0;
    while (query.next()) {
        table->insertRow(row);
        for (int col = 0; col < colCount; ++col) {
            QString val = query.value(col).isNull() ? "—" : query.value(col).toString();
            QTableWidgetItem *item = new QTableWidgetItem(val);
            item->setFlags(item->flags() & ~Qt::ItemIsEditable);
            table->setItem(row, col, item);
        }
        ++row;
    }
    table->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    table->verticalHeader()->setVisible(false);
}

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
