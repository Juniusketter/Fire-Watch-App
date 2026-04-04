#ifndef EXTDIALOG_H
#define EXTDIALOG_H

// ─────────────────────────────────────────────────────────────────────────────
//  ExtDialog — Add / Edit Extinguisher form (Admin only)
//  Updated Sprint 4: added NFPA 10 metadata fields
// ─────────────────────────────────────────────────────────────────────────────

#include <QDialog>
#include <QLineEdit>
#include <QSpinBox>
#include <QDoubleSpinBox>
#include <QComboBox>
#include <QDateEdit>
#include <QFormLayout>
#include <QDialogButtonBox>
#include <QLabel>
#include <QDate>
#include <QScrollArea>
#include <QVBoxLayout>
#include <QFrame>

class ExtDialog : public QDialog
{
    Q_OBJECT

public:
    // mode: "Add" or "Edit" (sets button label)
    explicit ExtDialog(const QString &mode, QWidget *parent = nullptr);

    // ── Location setters (pre-fill for Edit) ─────────────────────────────
    void setAddress(const QString &v)             { m_address->setText(v); }
    void setBuildingNumber(const QString &v)      { m_building->setText(v); }
    void setFloorNumber(int v)                    { m_floor->setValue(v); }
    void setRoomNumber(const QString &v)          { m_room->setText(v); }
    void setLocationDescription(const QString &v) { m_locDesc->setText(v); }
    void setInspectionInterval(int v)             { m_interval->setValue(v); }
    void setNextDueDate(const QString &v) {
        QDate d = QDate::fromString(v, Qt::ISODate);
        if (d.isValid()) m_dueDate->setDate(d);
    }

    // ── NFPA setters ─────────────────────────────────────────────────────
    void setExtType(const QString &v) {
        int idx = m_extType->findData(v);
        if (idx >= 0) m_extType->setCurrentIndex(idx);
    }
    void setExtSize(double v)               { m_extSize->setValue(v); }
    void setSerialNumber(const QString &v)  { m_serial->setText(v); }
    void setManufacturer(const QString &v)  { m_manufacturer->setText(v); }
    void setModel(const QString &v)         { m_model->setText(v); }
    void setManufactureDate(const QString &v) { m_mfgDate->setText(v); }
    void setLastAnnualDate(const QString &v)  { m_lastAnnual->setText(v); }
    void setLast6YearDate(const QString &v)   { m_last6Year->setText(v); }
    void setLastHydroDate(const QString &v)   { m_lastHydro->setText(v); }
    void setExtStatus(const QString &v) {
        int idx = m_extStatus->findData(v);
        if (idx >= 0) m_extStatus->setCurrentIndex(idx);
    }

    // ── Location getters ─────────────────────────────────────────────────
    QString address()             const { return m_address->text().trimmed(); }
    QString buildingNumber()      const { return m_building->text().trimmed(); }
    int     floorNumber()         const { return m_floor->value(); }
    QString roomNumber()          const { return m_room->text().trimmed(); }
    QString locationDescription() const { return m_locDesc->text().trimmed(); }
    int     inspectionInterval()  const { return m_interval->value(); }
    QString nextDueDate()         const { return m_dueDate->date().toString(Qt::ISODate); }

    // ── NFPA getters ─────────────────────────────────────────────────────
    QString extType()        const { return m_extType->currentData().toString(); }
    double  extSize()        const { return m_extSize->value(); }
    QString serialNumber()   const { return m_serial->text().trimmed(); }
    QString manufacturer()   const { return m_manufacturer->text().trimmed(); }
    QString model()          const { return m_model->text().trimmed(); }
    QString manufactureDate()const { return m_mfgDate->text().trimmed(); }
    QString lastAnnualDate() const { return m_lastAnnual->text().trimmed(); }
    QString last6YearDate()  const { return m_last6Year->text().trimmed(); }
    QString lastHydroDate()  const { return m_lastHydro->text().trimmed(); }
    QString extStatus()      const { return m_extStatus->currentData().toString(); }

private:
    // Location fields
    QLineEdit *m_address;
    QLineEdit *m_building;
    QSpinBox  *m_floor;
    QLineEdit *m_room;
    QLineEdit *m_locDesc;
    QSpinBox  *m_interval;
    QDateEdit *m_dueDate;

    // NFPA 10 metadata fields
    QComboBox      *m_extType;
    QDoubleSpinBox *m_extSize;
    QLineEdit      *m_serial;
    QLineEdit      *m_manufacturer;
    QLineEdit      *m_model;
    QLineEdit      *m_mfgDate;
    QLineEdit      *m_lastAnnual;
    QLineEdit      *m_last6Year;
    QLineEdit      *m_lastHydro;
    QComboBox      *m_extStatus;
};

#endif // EXTDIALOG_H
