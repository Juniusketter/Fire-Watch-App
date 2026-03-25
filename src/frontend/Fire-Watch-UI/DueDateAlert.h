#ifndef DUEDATEALERT_H
#define DUEDATEALERT_H

#include <QMessageBox>

/*
  original logic by Lillian Bowen
  can be moved to best place, likely its own function instead of a class
  will prompt a warning notification when an assignment is a specified number of days away from the due date
  this is purely a logical solution
  variable names and if/else statements will have to be adjusted to properly integrate
  if you are seeing this on 3/24, i will come back to this tomorrow
*/
class DueDateAlert{
  void assignmentAlert(){
    if(assignment == !complete){
      if(daysTillDue == 7){
        QMessageBox::information (this, "Assignment", "Due in 7 days");
      }
      else if(daysTillDue == 2){
        QMessageBox::warning (this, "Assignment", "Due in 2 days");
      }
      else if(daysTillDue <= 0){
        QMessageBoc::critical (this, "Assignment", "Due today or overdue");
      }
    }
  }

}
