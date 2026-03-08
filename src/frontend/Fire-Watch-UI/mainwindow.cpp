#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include <QMessageBox>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::on_loginButton_clicked()
{
    // 1. Establish the connection to the SQLite driver
    if (!QSqlDatabase::contains("qt_sql_default_connection")) {
        QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE");
        // Using your absolute path to ensure the file is found
        db.setDatabaseName("C:/Users/juniu/OneDrive/Desktop/Fire-Watch-App/src/database/FireWatch.db");
    }

    QSqlDatabase db = QSqlDatabase::database();

    if (!db.open()) {
        QMessageBox::critical(this, "Connection Error", "Database not found: " + db.lastError().text());
        return;
    }

    // 2. Prepare the secure SQL query to check credentials
    QSqlQuery query;
    query.prepare("SELECT * FROM Users WHERE username = :u AND password = :p");

    // .trimmed() removes any accidental leading or trailing spaces
    query.bindValue(":u", ui->usernameLineEdit->text().trimmed());
    query.bindValue(":p", ui->passwordLineEdit->text().trimmed());

    // 3. Execute and check results
    if (query.exec() && query.next()) {
        QMessageBox::information(this, "Success", "Login Successful! Welcome to Fire Watch.");
    } else {
        QMessageBox::warning(this, "Denied", "Invalid Username or Password.");
    }
}
