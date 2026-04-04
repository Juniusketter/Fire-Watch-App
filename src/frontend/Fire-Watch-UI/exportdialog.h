#ifndef EXPORTDIALOG_H
#define EXPORTDIALOG_H

// ─────────────────────────────────────────────────────────────────────────────
//  ExportDialog — Export Reports to PDF / Print (Admin only)
//
//  Moved from Admin.h (Lillian Bowen's Sprint 3 export/print functions)
//  into a proper Qt UI dialog. Backend classes should not contain UI code.
//
//  Original author: Lillian Bowen — Sprint 3
//  Refactored by: Junius Ketter — Sprint 3
// ─────────────────────────────────────────────────────────────────────────────

#include <QDialog>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QLabel>
#include <QTextEdit>
#include <QPushButton>
#include <QFileDialog>
#include <QMessageBox>
#include <QPainter>
#include <QImage>
#include <QDebug>
#include <QPrinter>
#include <QPrintDialog>
#include <QDesktopServices>
#include <QUrl>
#include <QFont>
#include <QDateTime>
#include <QTableWidget>
#include <QtSql/QSqlQuery>
#include <QtSql/QSqlDatabase>

class ExportDialog : public QDialog
{
    Q_OBJECT

public:
    explicit ExportDialog(QSqlDatabase db, QWidget *parent = nullptr)
        : QDialog(parent), m_db(db)
    {
        setWindowTitle("Export Reports");
        setMinimumWidth(480);
        setMinimumHeight(300);

        // ── Preview area ─────────────────────────────────────────────────
        m_preview = new QTextEdit(this);
        m_preview->setReadOnly(true);
        m_preview->setPlaceholderText("Report preview will appear here...");

        // ── Buttons ──────────────────────────────────────────────────────
        QPushButton *btnExportPdf = new QPushButton("📄 Export to PDF", this);
        QPushButton *btnPrint     = new QPushButton("🖨 Print", this);
        QPushButton *btnClose     = new QPushButton("Close", this);

        btnExportPdf->setStyleSheet(
            "QPushButton { background: #1f3864; color: white; padding: 8px 18px;"
            " border-radius: 4px; font-weight: bold; }"
            "QPushButton:hover { background: #163050; }");
        btnPrint->setStyleSheet(
            "QPushButton { background: #1a6b2e; color: white; padding: 8px 18px;"
            " border-radius: 4px; font-weight: bold; }"
            "QPushButton:hover { background: #145524; }");
        btnClose->setStyleSheet(
            "QPushButton { padding: 8px 18px; border-radius: 4px; }");

        QHBoxLayout *btnRow = new QHBoxLayout();
        btnRow->addWidget(btnExportPdf);
        btnRow->addWidget(btnPrint);
        btnRow->addStretch();
        btnRow->addWidget(btnClose);

        // ── Layout ───────────────────────────────────────────────────────
        QVBoxLayout *mainLayout = new QVBoxLayout(this);
        mainLayout->addWidget(new QLabel("<b>Report Preview</b>", this));
        mainLayout->addWidget(m_preview, 1);
        mainLayout->addLayout(btnRow);
        setLayout(mainLayout);

        // ── Connections ──────────────────────────────────────────────────
        connect(btnExportPdf, &QPushButton::clicked, this, &ExportDialog::onExportPdf);
        connect(btnPrint,     &QPushButton::clicked, this, &ExportDialog::onPrint);
        connect(btnClose,     &QPushButton::clicked, this, &QDialog::close);

        // Load preview on open
        loadReportPreview();
    }

private slots:

    // ─────────────────────────────────────────────────────────────────────
    //  Export to PDF — based on Lillian's exportReportToPDF()
    //  Enhanced: file picker, multi-report support, header/footer
    // ─────────────────────────────────────────────────────────────────────
    void onExportPdf()
    {
        QString filename = QFileDialog::getSaveFileName(
            this, "Save Report as PDF", "FireWatch_Reports.pdf",
            "PDF Files (*.pdf)");

        if (filename.isEmpty()) return;

        QPrinter printer(QPrinter::HighResolution);
        printer.setOutputFormat(QPrinter::PdfFormat);
        printer.setOutputFileName(filename);
        printer.setPageSize(QPageSize::Letter);

        QPainter painter;
        if (!painter.begin(&printer)) {
            QMessageBox::critical(this, "Export Failed",
                "Could not open file for writing:\n" + filename);
            return;
        }

        // ── Page dimensions ──────────────────────────────────────────
        QRect pageRect = printer.pageRect(QPrinter::DevicePixel).toRect();
        int margin = pageRect.width() / 20;
        int x = margin;
        int y = margin;
        int contentWidth = pageRect.width() - (2 * margin);

        // ── Header ───────────────────────────────────────────────────
        QFont titleFont("Arial", 18, QFont::Bold);
        QFont headerFont("Arial", 11, QFont::Bold);
        QFont bodyFont("Arial", 10);
        QFont smallFont("Arial", 8);

        painter.setFont(titleFont);
        painter.drawText(x, y, contentWidth, 100,
            Qt::AlignLeft | Qt::AlignTop, "FireWatch — Inspection Reports");
        y += 120;

        painter.setFont(smallFont);
        painter.drawText(x, y, contentWidth, 50,
            Qt::AlignLeft | Qt::AlignTop,
            "Generated: " + QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm AP"));
        y += 80;

        // ── Divider ─────────────────────────────────────────────────
        painter.setPen(QPen(QColor("#1f3864"), 3));
        painter.drawLine(x, y, x + contentWidth, y);
        painter.setPen(Qt::black);
        y += 40;

        // ── Report rows ─────────────────────────────────────────────
        QSqlQuery q(m_db);
        q.exec("SELECT r.report_id, r.extinguisher_id, u.username AS inspector, "
               "r.inspection_date, r.notes "
               "FROM Reports r "
               "LEFT JOIN Users u ON r.inspector_id = u.user_id "
               "ORDER BY r.inspection_date DESC");

        int reportNum = 0;
        while (q.next()) {
            reportNum++;

            // Check if we need a new page
            if (y > pageRect.height() - (margin * 3)) {
                printer.newPage();
                y = margin;
            }

            painter.setFont(headerFont);
            QString header = QString("Report #%1 — Extinguisher %2")
                .arg(q.value("report_id").toString())
                .arg(q.value("extinguisher_id").toString());
            painter.drawText(x, y, contentWidth, 50,
                Qt::AlignLeft | Qt::AlignTop, header);
            y += 60;

            painter.setFont(bodyFont);

            QString inspector = q.value("inspector").toString();
            if (!inspector.isEmpty()) {
                painter.drawText(x + 20, y, contentWidth, 40,
                    Qt::AlignLeft | Qt::AlignTop, "Inspector: " + inspector);
                y += 45;
            }

            painter.drawText(x + 20, y, contentWidth, 40,
                Qt::AlignLeft | Qt::AlignTop,
                "Date: " + q.value("inspection_date").toString());
            y += 45;

            QString notes = q.value("notes").toString();
            if (!notes.isEmpty()) {
                QRect notesRect(x + 20, y, contentWidth - 40, 200);
                QRect boundingRect;
                painter.drawText(notesRect,
                    Qt::AlignLeft | Qt::AlignTop | Qt::TextWordWrap,
                    "Notes: " + notes, &boundingRect);
                y += boundingRect.height() + 20;
            }

            // Separator between reports
            y += 20;
            painter.setPen(QPen(QColor("#cccccc"), 1));
            painter.drawLine(x + 20, y, x + contentWidth - 20, y);
            painter.setPen(Qt::black);
            y += 30;
        }

        if (reportNum == 0) {
            painter.setFont(bodyFont);
            painter.drawText(x, y, contentWidth, 50,
                Qt::AlignLeft | Qt::AlignTop, "No reports found.");
        }

        painter.end();

        QMessageBox::information(this, "Export Complete",
            QString("Exported %1 report(s) to:\n%2").arg(reportNum).arg(filename));
    }

    // ─────────────────────────────────────────────────────────────────────
    //  Print — based on Lillian's printPdf()
    //  Enhanced: uses QPrintDialog for direct printing
    // ─────────────────────────────────────────────────────────────────────
    void onPrint()
    {
        QPrinter printer(QPrinter::HighResolution);
        printer.setPageSize(QPageSize::Letter);

        QPrintDialog printDialog(&printer, this);
        printDialog.setWindowTitle("Print Reports");

        if (printDialog.exec() != QDialog::Accepted) return;

        QPainter painter;
        if (!painter.begin(&printer)) {
            QMessageBox::critical(this, "Print Failed",
                "Could not start print job.");
            return;
        }

        // Reuse the same rendering logic as PDF export
        QRect pageRect = printer.pageRect(QPrinter::DevicePixel).toRect();
        int margin = pageRect.width() / 20;
        int x = margin;
        int y = margin;
        int contentWidth = pageRect.width() - (2 * margin);

        QFont titleFont("Arial", 18, QFont::Bold);
        QFont headerFont("Arial", 11, QFont::Bold);
        QFont bodyFont("Arial", 10);

        painter.setFont(titleFont);
        painter.drawText(x, y, contentWidth, 100,
            Qt::AlignLeft | Qt::AlignTop, "FireWatch — Inspection Reports");
        y += 120;

        painter.setPen(QPen(QColor("#1f3864"), 3));
        painter.drawLine(x, y, x + contentWidth, y);
        painter.setPen(Qt::black);
        y += 40;

        QSqlQuery q(m_db);
        q.exec("SELECT r.report_id, r.extinguisher_id, u.username AS inspector, "
               "r.inspection_date, r.notes "
               "FROM Reports r "
               "LEFT JOIN Users u ON r.inspector_id = u.user_id "
               "ORDER BY r.inspection_date DESC");

        while (q.next()) {
            if (y > pageRect.height() - (margin * 3)) {
                printer.newPage();
                y = margin;
            }

            painter.setFont(headerFont);
            painter.drawText(x, y, contentWidth, 50, Qt::AlignLeft | Qt::AlignTop,
                QString("Report #%1 — Extinguisher %2")
                    .arg(q.value("report_id").toString())
                    .arg(q.value("extinguisher_id").toString()));
            y += 60;

            painter.setFont(bodyFont);
            QString inspector = q.value("inspector").toString();
            if (!inspector.isEmpty()) {
                painter.drawText(x + 20, y, contentWidth, 40,
                    Qt::AlignLeft | Qt::AlignTop, "Inspector: " + inspector);
                y += 45;
            }
            painter.drawText(x + 20, y, contentWidth, 40,
                Qt::AlignLeft | Qt::AlignTop,
                "Date: " + q.value("inspection_date").toString());
            y += 45;

            QString notes = q.value("notes").toString();
            if (!notes.isEmpty()) {
                QRect notesRect(x + 20, y, contentWidth - 40, 200);
                QRect boundingRect;
                painter.drawText(notesRect,
                    Qt::AlignLeft | Qt::AlignTop | Qt::TextWordWrap,
                    "Notes: " + notes, &boundingRect);
                y += boundingRect.height() + 20;
            }
            y += 30;
        }

        painter.end();
    }

private:

    // ─────────────────────────────────────────────────────────────────────
    //  Load report summary into preview text area
    // ─────────────────────────────────────────────────────────────────────
    void loadReportPreview()
    {
        QSqlQuery q(m_db);
        q.exec("SELECT r.report_id, r.extinguisher_id, u.username AS inspector, "
               "r.inspection_date, r.notes "
               "FROM Reports r "
               "LEFT JOIN Users u ON r.inspector_id = u.user_id "
               "ORDER BY r.inspection_date DESC");

        QString html = "<h3>FireWatch — Report Summary</h3><hr>";
        int count = 0;

        while (q.next()) {
            count++;
            html += QString(
                "<p><b>Report #%1</b> — Extinguisher %2<br>"
                "Inspector: %3<br>"
                "Date: %4<br>"
                "Notes: %5</p><hr>")
                .arg(q.value("report_id").toString())
                .arg(q.value("extinguisher_id").toString())
                .arg(q.value("inspector").toString())
                .arg(q.value("inspection_date").toString())
                .arg(q.value("notes").toString());
        }

        if (count == 0)
            html += "<p><i>No reports found.</i></p>";
        else
            html += QString("<p><b>Total: %1 report(s)</b></p>").arg(count);

        m_preview->setHtml(html);
    }

    QSqlDatabase m_db;
    QTextEdit   *m_preview;
};

#endif // EXPORTDIALOG_H
