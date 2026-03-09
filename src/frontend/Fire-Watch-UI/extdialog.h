#ifndef EXTDIALOG_H
#define EXTDIALOG_H

// ─────────────────────────────────────────────────────────────────────────────
//  ExtDialog — Add / Edit Extinguisher form (Admin only)
//  Used by dashboard.cpp for both Add and Edit flows.
// ─────────────────────────────────────────────────────────────────────────────

#include <QDialog>
#include <QLineEdit>
#include <QSpinBox>
#include <QDateEdit>
#include <QFormLayout>
#include <QDialogButtonBox>
#include <QLabel>
#include <QDate>

class ExtDialog : public QDialog
{
    Q_OBJECT

public:
    // mode: "Add" or "Edit"
    explicit ExtDialog(const QString &mode, QWidget *parent = nullptr);

    // ── Setters (used when pre-filling for Edit) ──────────────────────────
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

    // ── Getters (read after dialog accepted) ─────────────────────────────
    QString address()             const { return m_address->text().trimmed(); }
    QString buildingNumber()      const { return m_building->text().trimmed(); }
    int     floorNumber()         const { return m_floor->value(); }
    QString roomNumber()          const { return m_room->text().trimmed(); }
    QString locationDescription() const { return m_locDesc->text().trimmed(); }
    int     inspectionInterval()  const { return m_interval->value(); }
    QString nextDueDate()         const { return m_dueDate->date().toString(Qt::ISODate); }

private:
    QLineEdit *m_address;
    QLineEdit *m_building;
    QSpinBox  *m_floor;
    QLineEdit *m_room;
    QLineEdit *m_locDesc;
    QSpinBox  *m_interval;
    QDateEdit *m_dueDate;
};

#endif // EXTDIALOG_H
