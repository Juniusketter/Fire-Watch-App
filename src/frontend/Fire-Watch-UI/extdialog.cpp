#include "extdialog.h"
#include <QVBoxLayout>
#include <QPushButton>

ExtDialog::ExtDialog(const QString &mode, QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle(mode + " Extinguisher");
    setMinimumWidth(420);

    // ── Fields ────────────────────────────────────────────────────────────
    m_address  = new QLineEdit(this);
    m_address->setPlaceholderText("e.g. 123 University Avenue");

    m_building = new QLineEdit(this);
    m_building->setPlaceholderText("e.g. Bldg A");

    m_floor    = new QSpinBox(this);
    m_floor->setRange(0, 200);
    m_floor->setValue(1);

    m_room     = new QLineEdit(this);
    m_room->setPlaceholderText("e.g. 101");

    m_locDesc  = new QLineEdit(this);
    m_locDesc->setPlaceholderText("e.g. Near the elevator");

    m_interval = new QSpinBox(this);
    m_interval->setRange(1, 365);
    m_interval->setValue(30);
    m_interval->setSuffix(" days");

    m_dueDate  = new QDateEdit(this);
    m_dueDate->setCalendarPopup(true);
    m_dueDate->setDate(QDate::currentDate().addDays(30));
    m_dueDate->setDisplayFormat("yyyy-MM-dd");

    // ── Layout ────────────────────────────────────────────────────────────
    QFormLayout *form = new QFormLayout();
    form->setLabelAlignment(Qt::AlignRight);
    form->addRow("Address *",              m_address);
    form->addRow("Building",               m_building);
    form->addRow("Floor",                  m_floor);
    form->addRow("Room",                   m_room);
    form->addRow("Location Description",   m_locDesc);
    form->addRow("Inspection Interval",    m_interval);
    form->addRow("Next Due Date",          m_dueDate);

    // Required field note
    QLabel *note = new QLabel("<small><i>* Required field</i></small>", this);

    // Buttons
    QDialogButtonBox *buttons = new QDialogButtonBox(
        QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    buttons->button(QDialogButtonBox::Ok)->setText(mode);
    buttons->button(QDialogButtonBox::Ok)->setStyleSheet(
        "background-color: #1f3864; color: white; padding: 6px 16px; border-radius: 4px;");

    connect(buttons, &QDialogButtonBox::accepted, this, [this]() {
        if (m_address->text().trimmed().isEmpty()) {
            m_address->setPlaceholderText("⚠ Address is required!");
            m_address->setStyleSheet("border: 1px solid red;");
            return;
        }
        accept();
    });
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);

    QVBoxLayout *main = new QVBoxLayout(this);
    main->addLayout(form);
    main->addWidget(note);
    main->addSpacing(8);
    main->addWidget(buttons);
    setLayout(main);
}
