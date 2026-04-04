// Admin.h - Full implementation backend for edit/delete users
#pragma once
#include <string>
#include <sqlite3.h>
class Admin{
public:
 std::string userId;
 std::string companyId;
 Admin(std::string u,std::string c):userId(u),companyId(c){}
 bool editUser(sqlite3*db,const std::string&uid,const std::string&email,const std::string&name,const std::string&role,std::string*err);
 bool deleteUser(sqlite3*db,const std::string&uid,std::string*err);
 void audit(sqlite3*db,const std::string&entity,const std::string&id,const std::string&action);
};
#ifndef ADMIN_H
#define ADMIN_H

#include "User.h"
#include "Assignment.h"
#include <iostream>
#include <string>
#include <sqlite3.h>
class Admin{
public:
 std::string userId;
 std::string companyId;
 Admin(std::string u,std::string c):userId(u),companyId(c){}
 bool editUser(sqlite3*db,const std::string&uid,const std::string&email,const std::string&name,const std::string&role,std::string*err);
 bool deleteUser(sqlite3*db,const std::string&uid,std::string*err);
 void audit(sqlite3*db,const std::string&entity,const std::string&id,const std::string&action);
};