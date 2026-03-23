#ifndef ADMIN_H
#define ADMIN_H

#include "User.h"
#include "Assignment.h"
#include <iostream>
#include <string>
#include <vector>

//added for exportReportToPDF() nd pritnPdf(), can take out if that function is moved or changed to not need these
#include <QPainter>
#include <QImage>
#include <QDebug>
#include <QPrinter>
#include <QPrintDialog>
#include <QDesktopServices>
#include <QUrl>

using namespace std;

// Admin class - extends User class
// Created by Lilly Bowen, March 3, 2026
// Updated Sprint 2: Replaced cin/cout console I/O in generateAssignment()
// and changeDB() with parameter-based methods.
// UI input is handled by AssignDialog + ExtDialog in the Qt dashboard
// and by the web UI modals (index.html). DB writes are handled by the
// calling layer (dashboard.cpp / server.py), not this class.

class Admin : public User {
    private:
        string adminID;
        int DBAccess;

    public:
        // Default constructor
        Admin() : User(), adminID(""), DBAccess(0) {}

        // Parameterized constructor
        Admin(string u, string p, string c, string id, int access)
            : User(u, p, c), adminID(id), DBAccess(access) {}

        void   setDBAccess(int access) { DBAccess = access; }
        int    getDBAccess()           { return DBAccess; }
        string getAdminID()            { return adminID; }
        void   setAdminID(string id)   { adminID = id; }

        // Returns true if access code matches
        bool accessDB(int access) {
            if (DBAccess == access) {
                cout << "Access granted. You can now view and edit the database." << endl;
                return true;
            } else {
                cout << "You do not have access to the database. Please check your access code." << endl;
                return false;
            }
        }

        // ─────────────────────────────────────────────────────────────────
        //  changeDB()
        //
        //  Modifies an extinguisher record in the database.
        //  All required data is passed in as parameters — no console I/O.
        //  The Qt dashboard (ExtDialog) handles UI input and calls this
        //  method with validated data, or writes directly via QSqlQuery.
        //
        //  Parameters:
        //    access   - admin access code for authorization
        //    action   - 1=Add, 2=Remove, 3=Update
        //    field    - (for action=3) 1=Type, 2=Location, 3=Interval, 4=Report
        //
        //  Returns: true if the action was authorized and valid.
        // ─────────────────────────────────────────────────────────────────
        bool changeDB(int access, int action, int field = 0) {
            if (!accessDB(access)) return false;

            switch (action) {
                case 1:
                    cout << "The extinguisher has been added successfully!" << endl;
                    return true;
                case 2:
                    cout << "The extinguisher has been removed successfully!" << endl;
                    return true;
                case 3:
                    switch (field) {
                        case 1: cout << "The extinguisher type has been changed successfully!"     << endl; return true;
                        case 2: cout << "The extinguisher location has been changed successfully!" << endl; return true;
                        case 3: cout << "The inspection interval has been changed successfully!"   << endl; return true;
                        case 4: cout << "The extinguisher report has been added successfully!"     << endl; return true;
                        default: cout << "Invalid field option." << endl; return false;
                    }
                default:
                    cout << "Invalid action option." << endl;
                    return false;
            }
        }

        void viewDB(int access) {
            if (!accessDB(access)) return;
            cout << "Here is the database." << endl;
        }

        // ─────────────────────────────────────────────────────────────────
        //  generateAssignment()
        //
        //  Creates and returns an Assignment for an admin.
        //  All required data is passed in as parameters — no console I/O.
        //  Supports two recipient types:
        //    - invsList populated  → assign to internal inspectors
        //    - thirdPCompanyName   → assign to a third-party company
        //
        //  Parameters:
        //    extinguisherLocations - locations of extinguishers to inspect
        //    dueDate               - due date string (YYYY-MM-DD)
        //    invsList              - inspector IDs (empty if third party)
        //    thirdPCompanyName     - third party company name (empty if internal)
        //
        //  Returns: populated Assignment object.
        //  The caller (Qt dashboard or Flask server) saves it to the DB.
        // ─────────────────────────────────────────────────────────────────
        Assignment generateAssignment(
            vector<string> extinguisherLocations,
            string dueDate,
            vector<string> invsList,
            string thirdPCompanyName = "")
        {
            int numExtinguishers = extinguisherLocations.size();
            int numInvs          = invsList.size();
            string id            = getAdminID();

            if (!thirdPCompanyName.empty()) {
                // Assign to third-party company
                Assignment newAssignment(
                    numExtinguishers, numInvs, id,
                    extinguisherLocations, dueDate, thirdPCompanyName
                );
                cout << "Assignment generated for third-party company: "
                     << thirdPCompanyName << ", "
                     << numExtinguishers << " extinguisher(s), due " << dueDate << "." << endl;
                return newAssignment;
            } else {
                // Assign to internal inspectors
                Assignment newAssignment(
                    numExtinguishers, numInvs, id,
                    extinguisherLocations, dueDate, invsList
                );
                cout << "Assignment generated: "
                     << numExtinguishers << " extinguisher(s), "
                     << numInvs << " inspector(s), due " << dueDate << "." << endl;
                return newAssignment;
            }
        }

        // export and print report
        // the placement may be adjusted to whatever file it would be best suited for
        // i am placing it here since the admin users should be the only ones who should be able to export or print the reports
        // uses Qt to export the information from the reports (text and images) into a PDF

        bool exportReportToPDF(QString& filename, QString& text, QImage& image){
            //Create QPrinter instance
            //feel free to adjust any numbers during testing to ensure proper sizes/placement
            QPrinter printer(QPrinter::HighResolution);
            printer.setOutputFormat(QPrinter::PdfFormat);
            printer.setOutputFileName(filename);
            printer.setPageSize(QPrinter::A4);

            //Create QPainter instance
            QPainter painter;
            if(!painter.begin(&printer)){
                qWarning("Failed to open file for painting.");
                return false;
            }

            //draw text at position (100, 100) 
            //position to be changed upon testing to ensure proper placement
            //can also add format options using painter.setFont(QFont("fontStyle", fontSize));
            painter.drawText(100, 100, text);

            //draw image at position (100, 150) at scale 400x300
            //position and size can be changed at testing to ensure proper fit and placement
            QImage scaledImage = image.scaled(400, 300, Qt::KeepAspectRatio);
            painter.drawImage(100, 150, scaledImage);

            //above steps can be repeated as needed
            //if new page is needed add pdfWriter.newPage();

            //end
            painter.end();
            qDebug() << "PDF exported successfully to: " << filename;
            return true;
        }
            
        //print function prompts the user using a print dialog allowing them to choose a printer
        void printPdf(QString& filename){
            //opens PDF using system viewer
            //allows user to print manually
            if(!QDesktopServices::openURL(QUrl::fromLocalFile(filename))){
                qWarning("Could not open the generated PDF file for printing.");
            }
        }


};

#endif // ADMIN_H
