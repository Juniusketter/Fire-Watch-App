#ifndef DUEDATEALERT_H
#define DUEDATEALERT_H

// ─────────────────────────────────────────────────────────────────────────────
//  DueDateAlert.h
//  Original per-alert logic by Lillian Bowen — Sprint 3
//  Sprint 4 Qt Sync: replaced per-popup QMessageBox spam with a single
//  consolidated summary dialog that lists all due/overdue assignments at once.
// ─────────────────────────────────────────────────────────────────────────────

#include <QDialog>
#include <QLabel>
#include <QScrollArea>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QDialogButtonBox>
#include <QPushButton>
#include <QTableWidget>
#include <QHeaderView>
#include <QTableWidgetItem>
#include <QString>
#include <QList>

// ─────────────────────────────────────────────────────────────────────────────
//  DueDateItem — one alert entry
// ─────────────────────────────────────────────────────────────────────────────
struct DueDateItem {
    int     assignmentId;
    QString status;
    int     daysTillDue;   // negative = overdue
};

// ─────────────────────────────────────────────────────────────────────────────
//  DueDateSummaryDialog — shows ALL due/overdue assignments in one dialog
// ─────────────────────────────────────────────────────────────────────────────
class DueDateSummaryDialog : public QDialog
{
    Q_OBJECT

public:
    explicit DueDateSummaryDialog(const QList<DueDateItem> &items,
                                  QWidget *parent = nullptr)
        : QDialog(parent)
    {
        setWindowTitle("Due Date Alerts");
        setMinimumWidth(520);
        setMinimumHeight(320);

        // Count overdue vs upcoming
        int overdueCount  = 0;
        int upcomingCount = 0;
        for (const auto &item : items) {
            if (item.daysTillDue <= 0) ++overdueCount;
            else                        ++upcomingCount;
        }

        // Header label
        QLabel *header = new QLabel(this);
        QString headerText;
        if (overdueCount > 0 && upcomingCount > 0)
            headerText = QString("<b style='color:#c1121f;'>%1 overdue</b> and "
                                 "<b style='color:#e85d04;'>%2 upcoming</b> assignment(s) need attention.")
                         .arg(overdueCount).arg(upcomingCount);
        else if (overdueCount > 0)
            headerText = QString("<b style='color:#c1121f;'>%1 overdue</b> assignment(s) require immediate attention.")
                         .arg(overdueCount);
        else
            headerText = QString("<b style='color:#e85d04;'>%1 upcoming</b> assignment(s) are due within 7 days.")
                         .arg(upcomingCount);
        header->setText(headerText);
        header->setWordWrap(true);
        header->setMargin(4);

        // Table of alerts
        QTableWidget *table = new QTableWidget(items.size(), 3, this);
        table->setHorizontalHeaderLabels({"Assignment", "Status", "Due In"});
        table->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
        table->verticalHeader()->setVisible(false);
        table->setEditTriggers(QAbstractItemView::NoEditTriggers);
        table->setSelectionBehavior(QAbstractItemView::SelectRows);
        table->setAlternatingRowColors(true);

        for (int row = 0; row < items.size(); ++row) {
            const DueDateItem &item = items[row];

            // Assignment ID
            QTableWidgetItem *idItem = new QTableWidgetItem(
                QString("Assignment #%1").arg(item.assignmentId));
            idItem->setFlags(idItem->flags() & ~Qt::ItemIsEditable);

            // Status
            QTableWidgetItem *statusItem = new QTableWidgetItem(item.status);
            statusItem->setFlags(statusItem->flags() & ~Qt::ItemIsEditable);

            // Days label
            QString daysText;
            QColor  daysColor;
            if (item.daysTillDue < 0) {
                daysText  = QString("OVERDUE (%1 days)").arg(-item.daysTillDue);
                daysColor = QColor("#c1121f");
            } else if (item.daysTillDue == 0) {
                daysText  = "Due TODAY";
                daysColor = QColor("#c1121f");
            } else if (item.daysTillDue <= 2) {
                daysText  = QString("Due in %1 day(s)").arg(item.daysTillDue);
                daysColor = QColor("#e85d04");
            } else {
                daysText  = QString("Due in %1 days").arg(item.daysTillDue);
                daysColor = QColor("#b45309");
            }
            QTableWidgetItem *daysItem = new QTableWidgetItem(daysText);
            daysItem->setFlags(daysItem->flags() & ~Qt::ItemIsEditable);
            daysItem->setForeground(daysColor);
            QFont boldFont = daysItem->font();
            boldFont.setBold(true);
            daysItem->setFont(boldFont);

            table->setItem(row, 0, idItem);
            table->setItem(row, 1, statusItem);
            table->setItem(row, 2, daysItem);
        }

        // Close button
        QDialogButtonBox *buttons = new QDialogButtonBox(QDialogButtonBox::Ok, this);
        buttons->button(QDialogButtonBox::Ok)->setText("Dismiss");
        buttons->button(QDialogButtonBox::Ok)->setStyleSheet(
            "background-color: #1f3864; color: white; padding: 6px 20px; border-radius: 4px;");
        connect(buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);

        QVBoxLayout *main = new QVBoxLayout(this);
        main->addWidget(header);
        main->addSpacing(6);
        main->addWidget(table);
        main->addWidget(buttons);
        setLayout(main);
    }
};

#endif // DUEDATEALERT_H
