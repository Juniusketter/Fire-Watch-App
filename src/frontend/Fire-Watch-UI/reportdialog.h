#ifndef REPORTDIALOG_H
#define REPORTDIALOG_H

// ─────────────────────────────────────────────────────────────────────────────
//  ReportDialog — Generate Report form (Inspector only)
//  Allows inspector to submit an inspection report for an assignment.
//  Author: Junius Ketter — Sprint 2
// ─────────────────────────────────────────────────────────────────────────────

#include <QDialog>
#include <QComboBox>
#include <QTextEdit>
#include <QDateEdit>
#include <QFormLayout>
#include <QVBoxLayout>
#include <QDialogButtonBox>
#include <QLabel>
#include <QDate>
#include <QPushButton>

class ReportDialog : public QDialog
{
    Q_OBJECT

public:
    explicit ReportDialog(QWidget *parent = nullptr) : QDialog(parent)
    {
        setWindowTitle("Generate Report");
        setMinimumWidth(420);

        m_assignment    = new QComboBox(this);
        m_extinguisher  = new QComboBox(this);

        m_serviceType = new QComboBox(this);
        m_serviceType->addItem("Routine Inspection", "Routine Inspection");
        m_serviceType->addItem("Annual",             "Annual");
        m_serviceType->addItem("6-Year",             "6-Year");
        m_serviceType->addItem("Hydrostatic",        "Hydrostatic");
        m_serviceType->addItem("Replacement",        "Replacement");

        m_result = new QComboBox(this);
        m_result->addItem("Pass",          "Pass");
        m_result->addItem("Fail",          "Fail");
        m_result->addItem("Needs Repair",  "Needs Repair");
        m_result->addItem("Conditional",   "Conditional");

        m_inspectionDate = new QDateEdit(this);
        m_inspectionDate->setCalendarPopup(true);
        m_inspectionDate->setDate(QDate::currentDate());
        m_inspectionDate->setDisplayFormat("yyyy-MM-dd");

        m_notes = new QTextEdit(this);
        m_notes->setPlaceholderText("Enter inspection notes, observations, or findings...");
        m_notes->setMaximumHeight(100);

        QFormLayout *form = new QFormLayout();
        form->setLabelAlignment(Qt::AlignRight);
        form->addRow("Assignment *",      m_assignment);
        form->addRow("Extinguisher *",    m_extinguisher);
        form->addRow("Service Type *",    m_serviceType);
        form->addRow("Result *",          m_result);
        form->addRow("Inspection Date",   m_inspectionDate);
        form->addRow("Notes",             m_notes);

        QLabel *note = new QLabel("<small><i>* Required field</i></small>", this);

        QDialogButtonBox *buttons = new QDialogButtonBox(
            QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
        buttons->button(QDialogButtonBox::Ok)->setText("Submit Report");
        buttons->button(QDialogButtonBox::Ok)->setStyleSheet(
            "background-color: #1f3864; color: white; padding: 6px 16px; border-radius: 4px;");

        connect(buttons, &QDialogButtonBox::accepted, this, [this]() {
            if (m_extinguisher->currentData().isNull()) return;
            accept();
        });
        connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);

        // When assignment changes, auto-select its extinguisher
        connect(m_assignment, QOverload<int>::of(&QComboBox::currentIndexChanged),
                this, [this](int) {
            int extId = m_assignment->currentData(Qt::UserRole + 1).toInt();
            for (int i = 0; i < m_extinguisher->count(); ++i) {
                if (m_extinguisher->itemData(i).toInt() == extId) {
                    m_extinguisher->setCurrentIndex(i);
                    break;
                }
            }
        });

        QVBoxLayout *main = new QVBoxLayout(this);
        main->addLayout(form);
        main->addWidget(note);
        main->addSpacing(8);
        main->addWidget(buttons);
        setLayout(main);
    }

    // Populate assignments combo: list of (assignment_id, label, extinguisher_id)
    void setAssignments(const QList<QVariantList> &items) {
        m_assignment->clear();
        for (auto &row : items) {
            // row[0]=assignment_id, row[1]=label, row[2]=extinguisher_id
            m_assignment->addItem(row[1].toString(), row[0]);
            m_assignment->setItemData(m_assignment->count() - 1,
                                      row[2], Qt::UserRole + 1);
        }
    }

    // Populate extinguisher combo
    void setExtinguishers(const QList<QPair<int,QString>> &items) {
        m_extinguisher->clear();
        for (auto &p : items)
            m_extinguisher->addItem(p.second, p.first);
    }

    int     assignmentId()    const { return m_assignment->currentData().toInt(); }
    int     extinguisherId()  const { return m_extinguisher->currentData().toInt(); }
    QString serviceType()     const { return m_serviceType->currentData().toString(); }
    QString result()          const { return m_result->currentData().toString(); }
    QString inspectionDate()  const { return m_inspectionDate->date().toString(Qt::ISODate); }
    QString notes()           const { return m_notes->toPlainText().trimmed(); }

private:
    QComboBox  *m_assignment;
    QComboBox  *m_extinguisher;
    QComboBox  *m_serviceType;
    QComboBox  *m_result;
    QDateEdit  *m_inspectionDate;
    QTextEdit  *m_notes;
};

#endif // REPORTDIALOG_H
