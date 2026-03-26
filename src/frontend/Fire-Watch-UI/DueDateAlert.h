#ifndef DUEDATEALERT_H
#define DUEDATEALERT_H

// DueDateAlert.h
// Original logic by Lillian Bowen — Sprint 3


#include <QMessageBox>
#include <QString>


// -----------------------------------------------------------------------------
inline void checkDueDateAlert(int assignmentId, const QString &status, int daysTillDue)
{
  
    if (status == "Inspection Complete") {
        return;
    }

    QString title = QString("Assignment #%1").arg(assignmentId);

    if (daysTillDue <= 0) {
  
        QMessageBox::critical(
            nullptr,
            title,
            QString("Assignment #%1 is due today or overdue! (%2 days)")
                .arg(assignmentId)
                .arg(daysTillDue)
        );
    }
    else if (daysTillDue <= 2) {
        
        QMessageBox::warning(
            nullptr,
            title,
            QString("Assignment #%1 is due in %2 day(s). Please complete soon.")
                .arg(assignmentId)
                .arg(daysTillDue)
        );
    }
    else if (daysTillDue <= 3) {
        // 3 days left — informational heads-up
        QMessageBox::information(
            nullptr,
            title,
            QString("Assignment #%1 is due in 3 days.")
                .arg(assignmentId)
        );
    }
    else if (daysTillDue <= 7) {
        // 4-7 days left — informational heads-up
        QMessageBox::information(
            nullptr,
            title,
            QString("Assignment #%1 is due in %2 days.")
                .arg(assignmentId)
                .arg(daysTillDue)
        );
    }
    // More than 7 days away — no alert needed
}

#endif // DUEDATEALERT_H
