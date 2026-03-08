#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QApplication>
#include <QDir>
#include <QFileInfo>
#include <QMessageBox>
#include <QtSql/QSqlDatabase>
#include <QtSql/QSqlQuery>
#include <QtSql/QSqlError>

// ─────────────────────────────────────────────────────────────────────────────
//  Locate FireWatch.db relative to the executable.
//  The database lives at:  <project_root>/src/database/FireWatch.db
//
//  At runtime the .exe is inside:
//    <project_root>/src/frontend/Fire-Watch-UI/build/.../Fire-Watch-UI.exe
//  So we walk up 5 levels to reach the project root, then descend into
//  src/database/FireWatch.db.
//
//  If that path doesn't exist we also try the working directory as a fallback.
// ─────────────────────────────────────────────────────────────────────────────
static QString findDatabasePath()
{
    // Walk up from the executable directory
    QDir dir(QApplication::applicationDirPath());
    for (int i = 0; i < 6; ++i) {
        QString candidate = dir.filePath("src/database/FireWatch.db");
        if (QFileInfo::exists(candidate))
            return candidate;
        dir.cdUp();
    }

    // Fallback: current working directory
    QString cwd = QDir::currentPath() + "/src/database/FireWatch.db";
    if (QFileInfo::exists(cwd))
        return cwd;

    return QString(); // not found
}

// ─────────────────────────────────────────────────────────────────────────────
MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    connect(ui->pushButtonLogin, &QPushButton::clicked,
            this, &MainWindow::onLoginClicked);

    connect(ui->lineEditPassword, &QLineEdit::returnPressed,
            this, &MainWindow::onLoginClicked);

    connect(ui->lineEditUsername, &QLineEdit::returnPressed,
            this, &MainWindow::onLoginClicked);

    // Open the database once at startup so we're ready for login attempts
    if (!openDatabase()) {
        ui->labelMessage->setStyleSheet("color: red;");
        ui->labelMessage->setText("Warning: could not open FireWatch.db");
    }
}

MainWindow::~MainWindow()
{
    // Close the database connection cleanly
    {
        QSqlDatabase db = QSqlDatabase::database("firewatch");
        if (db.isOpen())
            db.close();
    }
    QSqlDatabase::removeDatabase("firewatch");

    delete ui;
}

// ─────────────────────────────────────────────────────────────────────────────
bool MainWindow::openDatabase()
{
    // Avoid opening twice
    if (QSqlDatabase::contains("firewatch")) {
        QSqlDatabase db = QSqlDatabase::database("firewatch");
        return db.isOpen();
    }

    QString dbPath = findDatabasePath();
    if (dbPath.isEmpty()) {
        qWarning("FireWatch.db not found. Searched from: %s",
                 qPrintable(QApplication::applicationDirPath()));
        return false;
    }

    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", "firewatch");
    db.setDatabaseName(dbPath);

    if (!db.open()) {
        qWarning("Cannot open database: %s",
                 qPrintable(db.lastError().text()));
        return false;
    }

    qInfo("Database opened: %s", qPrintable(dbPath));
    return true;
}

// ─────────────────────────────────────────────────────────────────────────────
void MainWindow::onLoginClicked()
{
    QString username = ui->lineEditUsername->text().trimmed();
    QString password = ui->lineEditPassword->text();

    if (username.isEmpty() || password.isEmpty()) {
        ui->labelMessage->setStyleSheet("color: red;");
        ui->labelMessage->setText("Please enter both username and password.");
        return;
    }

    // ── Query FireWatch.db directly ──────────────────────────────────────────
    QSqlDatabase db = QSqlDatabase::database("firewatch");
    if (!db.isOpen()) {
        // Try to reopen if something went wrong at startup
        if (!openDatabase()) {
            ui->labelMessage->setStyleSheet("color: red;");
            ui->labelMessage->setText("Database unavailable. Check FireWatch.db path.");
            return;
        }
        db = QSqlDatabase::database("firewatch");
    }

    QSqlQuery query(db);
    query.prepare(
        "SELECT user_id, username, role "
        "FROM Users "
        "WHERE username = :username AND password_hash = :password"
    );
    query.bindValue(":username", username);
    query.bindValue(":password", password);

    if (!query.exec()) {
        ui->labelMessage->setStyleSheet("color: red;");
        ui->labelMessage->setText("Database error: " + query.lastError().text());
        return;
    }

    if (query.next()) {
        // ── Login success ────────────────────────────────────────────────────
        m_userId   = query.value("user_id").toInt();
        m_username = query.value("username").toString();
        m_role     = query.value("role").toString();

        ui->labelMessage->setStyleSheet("color: green;");
        ui->labelMessage->setText(
            "Login successful! Welcome, " + m_username +
            "  |  Role: " + m_role
        );
        ui->lineEditPassword->clear();

        // TODO: open the appropriate dashboard window based on m_role
        // e.g. if (m_role == "Admin") { AdminDashboard *dash = new AdminDashboard(...); }

    } else {
        // ── Login failure ────────────────────────────────────────────────────
        ui->labelMessage->setStyleSheet("color: red;");
        ui->labelMessage->setText("Invalid username or password.");
        ui->lineEditPassword->clear();
    }
}
