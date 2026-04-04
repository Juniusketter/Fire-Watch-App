#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "dashboard.h"

#include <QApplication>
#include <QDir>
#include <QFileInfo>
#include <QCryptographicHash>
#include <QtSql/QSqlDatabase>
#include <QtSql/QSqlQuery>
#include <QtSql/QSqlError>

// ─────────────────────────────────────────────────────────────────────────────
static QString findDatabasePath()
{
    QString direct = QDir::homePath() +
                     "/OneDrive/Desktop/Fire-Watch-App/src/database/FireWatch.db";
    if (QFileInfo::exists(direct)) return direct;

    QDir dir(QApplication::applicationDirPath());
    for (int i = 0; i < 6; ++i) {
        QString candidate = dir.filePath("src/database/FireWatch.db");
        if (QFileInfo::exists(candidate))
            return candidate;
        dir.cdUp();
    }
    return QString();
}

// ─────────────────────────────────────────────────────────────────────────────
MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    setWindowTitle("FireWatch");

    connect(ui->pushButtonLogin,  &QPushButton::clicked,
            this, &MainWindow::onLoginClicked);
    connect(ui->lineEditPassword, &QLineEdit::returnPressed,
            this, &MainWindow::onLoginClicked);
    connect(ui->lineEditUsername, &QLineEdit::returnPressed,
            this, &MainWindow::onLoginClicked);

    if (!openDatabase()) {
        ui->labelMessage->setStyleSheet("color: red;");
        ui->labelMessage->setText("Warning: could not open FireWatch.db");
    }
}

MainWindow::~MainWindow()
{
    {
        QSqlDatabase db = QSqlDatabase::database("firewatch");
        if (db.isOpen()) db.close();
    }
    QSqlDatabase::removeDatabase("firewatch");
    delete ui;
}

// ─────────────────────────────────────────────────────────────────────────────
bool MainWindow::openDatabase()
{
    if (QSqlDatabase::contains("firewatch")) {
        return QSqlDatabase::database("firewatch").isOpen();
    }

    QString dbPath = findDatabasePath();
    if (dbPath.isEmpty()) return false;

    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", "firewatch");
    db.setDatabaseName(dbPath);
    return db.open();
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

    QSqlDatabase db = QSqlDatabase::database("firewatch");
    if (!db.isOpen() && !openDatabase()) {
        ui->labelMessage->setStyleSheet("color: red;");
        ui->labelMessage->setText("Database unavailable. Check FireWatch.db path.");
        return;
    }

    // Hash the password with SHA-256 before comparing against stored hash
    QString hashedPassword = QString(
        QCryptographicHash::hash(password.toUtf8(), QCryptographicHash::Sha256).toHex()
    );

    QSqlQuery query(db);
    query.prepare(
        "SELECT user_id, username, role, org_id "
        "FROM Users "
        "WHERE username = :username AND password_hash = :password"
    );
    query.bindValue(":username", username);
    query.bindValue(":password", hashedPassword);

    if (!query.exec()) {
        ui->labelMessage->setStyleSheet("color: red;");
        ui->labelMessage->setText("Database error: " + query.lastError().text());
        return;
    }

    if (query.next()) {
        int     userId   = query.value("user_id").toInt();
        QString uname    = query.value("username").toString();
        QString role     = query.value("role").toString();
        int     orgId    = query.value("org_id").toInt();

        // Open the Dashboard window and close the login window
        Dashboard *dash = new Dashboard(userId, uname, role, orgId, db);
        dash->show();
        this->close();

    } else {
        ui->labelMessage->setStyleSheet("color: red;");
        ui->labelMessage->setText("Invalid username or password.");
        ui->lineEditPassword->clear();
    }
}
