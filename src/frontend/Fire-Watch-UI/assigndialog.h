#ifndef ASSIGNDIALOG_H
#define ASSIGNDIALOG_H

// ─────────────────────────────────────────────────────────────────────────────
//  AssignDialog — Generate Assignment form (Admin only)
//  Allows admin to assign an extinguisher inspection to an inspector.
//  Author: Junius Ketter — Sprint 2
// ─────────────────────────────────────────────────────────────────────────────

#include <QDialog>
#include <QComboBox>
#include <QDateEdit>
#include <QFormLayout>
#include <QVBoxLayout>
#include <QDialogButtonBox>
#include <QLabel>
#include <QDate>
#include <QPushButton>

class AssignDialog : public QDialog
{
    Q_OBJECT

public:
    explicit AssignDialog(QWidget *parent = nullptr) : QDialog(parent)
    {
        setWindowTitle("Generate Assignment");
        setMinimumWidth(420);

        m_extinguisher = new QComboBox(this);
        m_inspector    = new QComboBox(this);
        m_dueDate      = new QDateEdit(this);
        m_dueDate->setCalendarPopup(true);
        m_dueDate->setDate(QDate::currentDate().addDays(7));
        m_dueDate->setDisplayFormat("yyyy-MM-dd");

        QFormLayout *form = new QFormLayout();
        form->setLabelAlignment(Qt::AlignRight);
        form->addRow("Extinguisher *", m_extinguisher);
        form->addRow("Inspector *",    m_inspector);
        form->addRow("Due Date",       m_dueDate);

        QDialogButtonBox *buttons = new QDialogButtonBox(
            QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
        buttons->button(QDialogButtonBox::Ok)->setText("Generate");
        buttons->button(QDialogButtonBox::Ok)->setStyleSheet(
            "background-color: #1a6b2e; color: white; padding: 6px 16px; border-radius: 4px;");

        connect(buttons, &QDialogButtonBox::accepted, this, [this]() {
            if (m_extinguisher->currentData().isNull() ||
                m_inspector->currentData().isNull()) {
                return;
            }
            accept();
        });
        connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);

        QVBoxLayout *main = new QVBoxLayout(this);
        main->addLayout(form);
        main->addSpacing(8);
        main->addWidget(buttons);
        setLayout(main);
    }

    // Populate extinguisher combo: list of (id, label) pairs
    void setExtinguishers(const QList<QPair<int,QString>> &items) {
        m_extinguisher->clear();
        for (auto &p : items)
            m_extinguisher->addItem(p.second, p.first);
    }

    // Populate inspector combo: list of (id, username) pairs
    void setInspectors(const QList<QPair<int,QString>> &items) {
        m_inspector->clear();
        for (auto &p : items)
            m_inspector->addItem(p.second, p.first);
    }

    int    extinguisherId() const { return m_extinguisher->currentData().toInt(); }
    int    inspectorId()    const { return m_inspector->currentData().toInt(); }
    QString dueDate()       const { return m_dueDate->date().toString(Qt::ISODate); }

private:
    QComboBox *m_extinguisher;
    QComboBox *m_inspector;
    QDateEdit *m_dueDate;
};

#endif // ASSIGNDIALOG_H
