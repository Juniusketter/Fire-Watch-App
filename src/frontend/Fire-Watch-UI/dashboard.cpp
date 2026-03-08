#include "dashboard.h"
#include "ui_dashboard.h"
#include "mainwindow.h"

#include <QHeaderView>
#include <QMessageBox>
#include <QtSql/QSqlQuery>
#include <QtSql/QSqlError>
#include <QtSql/QSqlRecord>

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

    // Window title
    setWindowTitle("FireWatch — Dashboard");

    // Set welcome label
    ui->labelWelcome->setText("Welcome, " + m_username);
    ui->labelRole->setText("Role: " + m_role);

    // Wire buttons
    connect(ui->btnLogout,  &QPushButton::clicked, this, &Dashboard::onLogoutClicked);
    connect(ui->btnRefresh, &QPushButton::clicked, this, &Dashboard::onRefreshClicked);

    // Load all data
    updateStats();
    loadExtinguishers();
    loadAssignments();
    loadReports();

    // Only show Users tab to Admins
    if (m_role != "Admin") {
        ui->tabWidget->removeTab(3); // Users tab is index 3
    } else {
        loadUsers();
    }
}

Dashboard::~Dashboard()
{
    delete ui;
}

// ─────────────────────────────────────────────────────────────────────────────
//  Logout — close dashboard, reopen login window
// ─────────────────────────────────────────────────────────────────────────────
void Dashboard::onLogoutClicked()
{
    MainWindow *loginWindow = new MainWindow();
    loginWindow->show();
    this->close();
}

// ─────────────────────────────────────────────────────────────────────────────
//  Refresh — reload all tables from the database
// ─────────────────────────────────────────────────────────────────────────────
void Dashboard::onRefreshClicked()
{
    updateStats();
    loadExtinguishers();
    loadAssignments();
    loadReports();
    if (m_role == "Admin") loadUsers();
}

// ─────────────────────────────────────────────────────────────────────────────
//  Summary stats at the top
// ─────────────────────────────────────────────────────────────────────────────
void Dashboard::updateStats()
{
    auto count = [&](const QString &table) -> int {
        QSqlQuery q(m_db);
        q.exec("SELECT COUNT(*) FROM " + table);
        return q.next() ? q.value(0).toInt() : 0;
    };

    ui->statExtinguishers->setText(QString::number(count("Extinguishers")));
    ui->statAssignments->setText(QString::number(count("Assignments")));
    ui->statReports->setText(QString::number(count("Reports")));
    ui->statUsers->setText(QString::number(count("Users")));
}

// ─────────────────────────────────────────────────────────────────────────────
//  Generic table filler
// ─────────────────────────────────────────────────────────────────────────────
void Dashboard::fillTable(QTableWidget *table, QSqlQuery &query)
{
    table->setRowCount(0);

    // Set column headers from query record
    QSqlRecord rec = query.record();
    int colCount = rec.count();
    table->setColumnCount(colCount);

    QStringList headers;
    for (int i = 0; i < colCount; ++i)
        headers << rec.fieldName(i);
    table->setHorizontalHeaderLabels(headers);

    // Fill rows
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
void Dashboard::loadExtinguishers()
{
    QSqlQuery q(m_db);
    q.exec("SELECT extinguisher_id, address, building_number, floor_number, "
           "room_number, location_description, inspection_interval_days, "
           "next_due_date FROM Extinguishers");
    fillTable(ui->tableExtinguishers, q);
}

void Dashboard::loadAssignments()
{
    QSqlQuery q(m_db);
    q.exec("SELECT assignment_id, admin_id, inspector_id, "
           "extinguisher_id, status FROM Assignments");
    fillTable(ui->tableAssignments, q);
}

void Dashboard::loadReports()
{
    QSqlQuery q(m_db);
    q.exec("SELECT report_id, extinguisher_id, inspector_id, "
           "inspection_date, notes FROM Reports");
    fillTable(ui->tableReports, q);
}

void Dashboard::loadUsers()
{
    QSqlQuery q(m_db);
    // Never show password hashes
    q.exec("SELECT user_id, username, role FROM Users");
    fillTable(ui->tableUsers, q);
}
