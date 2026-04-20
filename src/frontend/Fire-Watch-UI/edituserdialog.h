#ifndef EDITUSERDIALOG_H
#define EDITUSERDIALOG_H

// ─────────────────────────────────────────────────────────────────────────────
//  EditUserDialog — Edit existing user form (Admin only)
//  Sprint 4 Qt Sync — Junius Ketter
//  Supports: role change, name/email/phone edit, optional password reset
// ─────────────────────────────────────────────────────────────────────────────

#include <QDialog>
#include <QLineEdit>
#include <QComboBox>
#include <QCheckBox>
#include <QFormLayout>
#include <QVBoxLayout>
#include <QDialogButtonBox>
#include <QLabel>
#include <QPushButton>
#include <QGroupBox>

class EditUserDialog : public QDialog
{
    Q_OBJECT

public:
    explicit EditUserDialog(QWidget *parent = nullptr) : QDialog(parent)
    {
        setWindowTitle("Edit User");
        setMinimumWidth(420);

        // ── Read-only username display ────────────────────────────────────
        m_usernameDisplay = new QLabel(this);
        m_usernameDisplay->setStyleSheet("font-weight: bold; color: #9ca3af;");

        // ── Editable fields ───────────────────────────────────────────────
        m_role = new QComboBox(this);
        m_role->addItem("Inspector",           "Inspector");
        m_role->addItem("Admin",               "Admin");
        m_role->addItem("3rd Party Admin",     "3rd_Party_Admin");
        m_role->addItem("3rd Party Inspector", "3rd_Party_Inspector");
        m_role->addItem("Client",              "Client");

        m_firstName = new QLineEdit(this);
        m_firstName->setPlaceholderText("First name");

        m_lastName  = new QLineEdit(this);
        m_lastName->setPlaceholderText("Last name");

        m_email     = new QLineEdit(this);
        m_email->setPlaceholderText("email@example.com");

        m_phone     = new QLineEdit(this);
        m_phone->setPlaceholderText("Phone number");

        m_certNumber = new QLineEdit(this);
        m_certNumber->setPlaceholderText("Inspector certification number");

        m_subcontractorCompany = new QLineEdit(this);
        m_subcontractorCompany->setPlaceholderText("Subcontractor company name");

        // ── Optional password reset section ──────────────────────────────
        QGroupBox *pwGroup = new QGroupBox("Reset Password (optional)", this);
        m_resetPassword = new QCheckBox("Set a new password", pwGroup);
        m_newPassword   = new QLineEdit(pwGroup);
        m_newPassword->setEchoMode(QLineEdit::Password);
        m_newPassword->setPlaceholderText("New password (min 6 chars)");
        m_newPassword->setEnabled(false);
        m_confirmPassword = new QLineEdit(pwGroup);
        m_confirmPassword->setEchoMode(QLineEdit::Password);
        m_confirmPassword->setPlaceholderText("Confirm new password");
        m_confirmPassword->setEnabled(false);

        connect(m_resetPassword, &QCheckBox::toggled, this, [this](bool checked) {
            m_newPassword->setEnabled(checked);
            m_confirmPassword->setEnabled(checked);
            if (!checked) { m_newPassword->clear(); m_confirmPassword->clear(); }
        });

        QFormLayout *pwForm = new QFormLayout(pwGroup);
        pwForm->addRow(m_resetPassword);
        pwForm->addRow("New Password",     m_newPassword);
        pwForm->addRow("Confirm Password", m_confirmPassword);

        // ── Error label ───────────────────────────────────────────────────
        m_errorLabel = new QLabel(this);
        m_errorLabel->setStyleSheet("color: #c1121f; font-size: 11px;");
        m_errorLabel->setWordWrap(true);
        m_errorLabel->hide();

        // ── Main form ─────────────────────────────────────────────────────
        QFormLayout *form = new QFormLayout();
        form->setLabelAlignment(Qt::AlignRight);
        form->setSpacing(8);
        form->addRow("Username",   m_usernameDisplay);
        form->addRow("Role",       m_role);
        form->addRow("First Name", m_firstName);
        form->addRow("Last Name",  m_lastName);
        form->addRow("Email",               m_email);
        form->addRow("Phone",               m_phone);
        form->addRow("Cert Number",         m_certNumber);
        form->addRow("Subcontractor Co.",   m_subcontractorCompany);

        QDialogButtonBox *buttons = new QDialogButtonBox(
            QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
        buttons->button(QDialogButtonBox::Ok)->setText("Save Changes");
        buttons->button(QDialogButtonBox::Ok)->setStyleSheet(
            "background-color: #1f3864; color: white; padding: 6px 16px; border-radius: 4px;");

        connect(buttons, &QDialogButtonBox::accepted, this, [this]() {
            m_errorLabel->hide();
            if (m_resetPassword->isChecked()) {
                if (m_newPassword->text().length() < 6) {
                    m_errorLabel->setText("Password must be at least 6 characters.");
                    m_errorLabel->show(); return;
                }
                if (m_newPassword->text() != m_confirmPassword->text()) {
                    m_errorLabel->setText("Passwords do not match.");
                    m_errorLabel->show(); return;
                }
            }
            accept();
        });
        connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);

        QVBoxLayout *main = new QVBoxLayout(this);
        main->addLayout(form);
        main->addWidget(pwGroup);
        main->addWidget(m_errorLabel);
        main->addSpacing(8);
        main->addWidget(buttons);
        setLayout(main);
    }

    // ── Setters (pre-fill from DB) ────────────────────────────────────────
    void setDisplayUsername(const QString &v) { m_usernameDisplay->setText(v); }
    void setRole(const QString &v) {
        int idx = m_role->findData(v);
        if (idx >= 0) m_role->setCurrentIndex(idx);
    }
    void setFirstName(const QString &v) { m_firstName->setText(v); }
    void setLastName(const QString &v)  { m_lastName->setText(v); }
    void setEmail(const QString &v)     { m_email->setText(v); }
    void setPhone(const QString &v)           { m_phone->setText(v); }
    void setCertNumber(const QString &v)      { m_certNumber->setText(v); }
    void setSubcontractorCompany(const QString &v) { m_subcontractorCompany->setText(v); }

    // ── Getters ───────────────────────────────────────────────────────────
    QString role()            const { return m_role->currentData().toString(); }
    QString firstName()       const { return m_firstName->text().trimmed(); }
    QString lastName()        const { return m_lastName->text().trimmed(); }
    QString email()           const { return m_email->text().trimmed(); }
    QString phone()                const { return m_phone->text().trimmed(); }
    QString certNumber()           const { return m_certNumber->text().trimmed(); }
    QString subcontractorCompany() const { return m_subcontractorCompany->text().trimmed(); }
    bool    resetPassword()        const { return m_resetPassword->isChecked(); }
    QString newPassword()          const { return m_newPassword->text(); }

private:
    QLabel    *m_usernameDisplay;
    QComboBox *m_role;
    QLineEdit *m_firstName;
    QLineEdit *m_lastName;
    QLineEdit *m_email;
    QLineEdit *m_phone;
    QLineEdit *m_certNumber;
    QLineEdit *m_subcontractorCompany;
    QCheckBox *m_resetPassword;
    QLineEdit *m_newPassword;
    QLineEdit *m_confirmPassword;
    QLabel    *m_errorLabel;
};

#endif // EDITUSERDIALOG_H
