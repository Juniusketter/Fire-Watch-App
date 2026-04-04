#include "extdialog.h"
#include <QVBoxLayout>
#include <QPushButton>
#include <QScrollArea>
#include <QFrame>
#include <QLabel>

ExtDialog::ExtDialog(const QString &mode, QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle(mode + " Extinguisher");
    setMinimumWidth(460);
    setMinimumHeight(560);
    resize(460, 640);

    // ── Location fields ───────────────────────────────────────────────────
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
    m_interval->setRange(1, 3650);
    m_interval->setValue(365);
    m_interval->setSuffix(" days");

    m_dueDate  = new QDateEdit(this);
    m_dueDate->setCalendarPopup(true);
    m_dueDate->setDate(QDate::currentDate().addDays(365));
    m_dueDate->setDisplayFormat("yyyy-MM-dd");

    // ── NFPA 10 metadata fields ───────────────────────────────────────────
    m_extType = new QComboBox(this);
    m_extType->addItem("—",              "");
    m_extType->addItem("ABC Dry Chemical",    "ABC");
    m_extType->addItem("BC Dry Chemical",     "BC");
    m_extType->addItem("CO₂",                "CO2");
    m_extType->addItem("Water",              "Water");
    m_extType->addItem("Class K (Wet Chem)", "K");
    m_extType->addItem("Class D (Metal)",    "D");
    m_extType->addItem("Clean Agent",        "CleanAgent");
    m_extType->addItem("Halotron",           "Halotron");

    m_extSize = new QDoubleSpinBox(this);
    m_extSize->setRange(0.0, 999.9);
    m_extSize->setDecimals(1);
    m_extSize->setSuffix(" lbs");
    m_extSize->setValue(0.0);
    m_extSize->setSpecialValueText("—");

    m_serial       = new QLineEdit(this);
    m_serial->setPlaceholderText("Serial number");

    m_manufacturer = new QLineEdit(this);
    m_manufacturer->setPlaceholderText("e.g. Amerex, Kidde, Ansul");

    m_model        = new QLineEdit(this);
    m_model->setPlaceholderText("Model number");

    m_mfgDate      = new QLineEdit(this);
    m_mfgDate->setPlaceholderText("YYYY-MM-DD (optional)");

    m_lastAnnual   = new QLineEdit(this);
    m_lastAnnual->setPlaceholderText("YYYY-MM-DD (optional)");

    m_last6Year    = new QLineEdit(this);
    m_last6Year->setPlaceholderText("YYYY-MM-DD (optional)");

    m_lastHydro    = new QLineEdit(this);
    m_lastHydro->setPlaceholderText("YYYY-MM-DD (optional)");

    m_extStatus = new QComboBox(this);
    m_extStatus->addItem("Active",    "Active");
    m_extStatus->addItem("Needs Service", "Needs_Service");
    m_extStatus->addItem("Condemned", "Condemned");
    m_extStatus->addItem("Retired",   "Retired");

    // ── Build scrollable form ─────────────────────────────────────────────
    QWidget     *formWidget = new QWidget();
    QFormLayout *form       = new QFormLayout(formWidget);
    form->setLabelAlignment(Qt::AlignRight);
    form->setSpacing(8);

    // Section: Location
    QLabel *locHeader = new QLabel("<b>Location</b>");
    form->addRow(locHeader);
    form->addRow("Address *",            m_address);
    form->addRow("Building",             m_building);
    form->addRow("Floor",                m_floor);
    form->addRow("Room",                 m_room);
    form->addRow("Location Description", m_locDesc);
    form->addRow("Inspection Interval",  m_interval);
    form->addRow("Next Due Date",        m_dueDate);

    // Divider
    QFrame *divider = new QFrame();
    divider->setFrameShape(QFrame::HLine);
    divider->setFrameShadow(QFrame::Sunken);
    form->addRow(divider);

    // Section: NFPA 10 Info
    QLabel *nfpaHeader = new QLabel("<b>NFPA 10 Info</b>");
    form->addRow(nfpaHeader);
    form->addRow("Type",                m_extType);
    form->addRow("Size",                m_extSize);
    form->addRow("Serial Number",       m_serial);
    form->addRow("Manufacturer",        m_manufacturer);
    form->addRow("Model",               m_model);
    form->addRow("Manufacture Date",    m_mfgDate);
    form->addRow("Last Annual Date",    m_lastAnnual);
    form->addRow("Last 6-Year Date",    m_last6Year);
    form->addRow("Last Hydro Date",     m_lastHydro);
    form->addRow("Status",              m_extStatus);

    QLabel *note = new QLabel("<small><i>* Required  |  Date fields: YYYY-MM-DD</i></small>");
    form->addRow(note);

    // Scroll area wrapping the form
    QScrollArea *scroll = new QScrollArea(this);
    scroll->setWidget(formWidget);
    scroll->setWidgetResizable(true);
    scroll->setFrameShape(QFrame::NoFrame);

    // ── Buttons ───────────────────────────────────────────────────────────
    QDialogButtonBox *buttons = new QDialogButtonBox(
        QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    buttons->button(QDialogButtonBox::Ok)->setText(mode);
    buttons->button(QDialogButtonBox::Ok)->setStyleSheet(
        "background-color: #1f3864; color: white; padding: 6px 16px; border-radius: 4px;");

    connect(buttons, &QDialogButtonBox::accepted, this, [this]() {
        if (m_address->text().trimmed().isEmpty()) {
            m_address->setPlaceholderText("Address is required!");
            m_address->setStyleSheet("border: 1px solid red;");
            return;
        }
        accept();
    });
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);

    QVBoxLayout *main = new QVBoxLayout(this);
    main->setContentsMargins(8, 8, 8, 8);
    main->addWidget(scroll);
    main->addWidget(buttons);
    setLayout(main);
}
