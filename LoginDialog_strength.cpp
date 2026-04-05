#include "LoginDialog.h"
#include "ui_LoginDialog.h"
#include "PasswordPolicy.h"

void LoginDialog::initializePasswordStrength()
{
    connect(ui->passwordLineEdit, &QLineEdit::textChanged, this,
        [this](const QString &text) {
            int score = passwordStrengthScore(text.toStdString());
            if (score <= 2) {
                ui->passwordStrengthLabel->setText("Weak");
                ui->passwordStrengthLabel->setStyleSheet("color:red;");
            } else if (score <= 4) {
                ui->passwordStrengthLabel->setText("Medium");
                ui->passwordStrengthLabel->setStyleSheet("color:orange;");
            } else {
                ui->passwordStrengthLabel->setText("Strong");
                ui->passwordStrengthLabel->setStyleSheet("color:green;");
            }
        }
    );
}
