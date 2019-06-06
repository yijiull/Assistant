#ifndef MSQL_H
#define MSQL_H
#include "head.h"

class MSQL
{
private:
    MYSQL *conn;

public:
    MSQL(string IP, string USER, string password, unsigned int PORT = 3306);
    MSQL(string IP, string USER, string password, string database, unsigned int PORT = 3306);
    ~MSQL();
    json exec(const string &sql);
};
#endif