#ifndef ADDUSERDIALOG_H
#define ADDUSERDIALOG_H

// ─────────────────────────────────────────────────────────────────────────────
//  AddUserDialog — Create new user form (Admin only)
//  Author: Junius Ketter — Sprint 2
// ─────────────────────────────────────────────────────────────────────────────

#include <QDialog>
#include <QLineEdit>
#include <QComboBox>
#include <QFormLayout>
#include <QVBoxLayout>
#include <QDialogButtonBox>
#include <QLabel>
#include <QPushButton>

class AddUserDialog : public QDialog
{
    Q_OBJECT

public:
    explicit AddUserDialog(QWidget *parent = nullptr) : QDialog(parent)
    {
        setWindowTitle("Add New User");
        setMinimumWidth(380);

        m_username = new QLineEdit(this);
        m_username->setPlaceholderText("e.g. JSmith");

        m_password = new QLineEdit(this);
        m_password->setEchoMode(QLineEdit::Password);
        m_password->setPlaceholderText("Enter password");

        m_confirm = new QLineEdit(this);
        m_confirm->setEchoMode(QLineEdit::Password);
        m_confirm->setPlaceholderText("Re-enter password");

        m_role = new QComboBox(this);
        m_role->addItem("Inspector",           "Inspector");
        m_role->addItem("Admin",               "Admin");
        m_role->addItem("3rd Party Admin",     "3rd_Party_Admin");
        m_role->addItem("3rd Party Inspector", "3rd_Party_Inspector");

        m_errorLabel = new QLabel(this);
        m_errorLabel->setStyleSheet("color: #c1121f; font-size: 11px;");
        m_errorLabel->setWordWrap(true);
        m_errorLabel->hide();

        QFormLayout *form = new QFormLayout();
        form->setLabelAlignment(Qt::AlignRight);
        form->addRow("Username *",         m_username);
        form->addRow("Password *",         m_password);
        form->addRow("Confirm Password *", m_confirm);
        form->addRow("Role *",             m_role);

        QDialogButtonBox *buttons = new QDialogButtonBox(
            QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
        buttons->button(QDialogButtonBox::Ok)->setText("Create User");
        buttons->button(QDialogButtonBox::Ok)->setStyleSheet(
            "background-color: #15803d; color: white; padding: 6px 16px; border-radius: 4px;");

        connect(buttons, &QDialogButtonBox::accepted, this, [this]() {
            m_errorLabel->hide();
            if (m_username->text().trimmed().isEmpty()) {
                m_errorLabel->setText("Username is required."); m_errorLabel->show(); return;
            }
            if (m_password->text().isEmpty()) {
                m_errorLabel->setText("Password is required."); m_errorLabel->show(); return;
            }
            if (m_password->text().length() < 6) {
                m_errorLabel->setText("Password must be at least 6 characters."); m_errorLabel->show(); return;
            }
            if (m_password->text() != m_confirm->text()) {
                m_errorLabel->setText("Passwords do not match."); m_errorLabel->show(); return;
            }
            accept();
        });
        connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);

        QVBoxLayout *main = new QVBoxLayout(this);
        main->addLayout(form);
        main->addWidget(m_errorLabel);
        main->addSpacing(8);
        main->addWidget(buttons);
        setLayout(main);
    }

    QString username()    const { return m_username->text().trimmed(); }
    QString password()    const { return m_password->text(); }
    QString role()        const { return m_role->currentData().toString(); }

private:
    QLineEdit *m_username;
    QLineEdit *m_password;
    QLineEdit *m_confirm;
    QComboBox *m_role;
    QLabel    *m_errorLabel;
};

#endif // ADDUSERDIALOG_H
