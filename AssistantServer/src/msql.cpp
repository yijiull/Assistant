#include "msql.h"

MSQL::MSQL(string IP, string USER, string password, unsigned int PORT)
{
    conn = new MYSQL;
    mysql_init(conn);
    if(!mysql_real_connect(conn, IP.c_str(), USER.c_str(), password.c_str(), NULL, PORT, NULL, 0))
    {
        LOG(FATAL) << "connect wrong";
    }
    mysql_query(conn, "set names utf8");

}
MSQL::MSQL(string IP, string USER, string password, string database, unsigned int PORT)
{
    conn = new MYSQL;
    mysql_init(conn);
    if(!mysql_real_connect(conn, IP.c_str(), USER.c_str(), password.c_str(), database.c_str(), PORT, NULL, 0))
    {
        LOG(FATAL) << "connect wrong";
    }
    mysql_query(conn, "set names utf8");
}

json MSQL::exec(const string &sql)
{
    LOG(INFO) << "mysql exec: " << sql;
    if(mysql_query(conn, sql.c_str()))
    {
        //error
        LOG(WARNING) << "query error!";
        json ans;
        ans["OK"] = false;
        return ans;
    }
    else
    {
        //success
        MYSQL_RES *res = mysql_store_result(conn);
        if(res) //success
        {
            int m = mysql_num_fields(res);
            int n = mysql_num_rows(res);
            MYSQL_FIELD * fileds = mysql_fetch_fields(res);
            MYSQL_ROW row;
            int i = 0;
            json ans;
            while((row = mysql_fetch_row(res)))
            {
                //cout<<i<<endl;
                for(int j = 0; j < m; j++)
                {
                    string col = fileds[j].name;
                    ans["info"][i][col] = (row[j] ? row[j] : "NULL");  //由于下面直接用了ans["ok"],所以这里需要有个info，否则报错！！！
                }
                ++i;
            }
            mysql_free_result(res); //
            ans["OK"] = true;
            return ans;
        }
        else
        {
            int res = mysql_field_count(conn);
            if(res == 0)
            {
                //执行的是update/insert等操作
                int cnt = mysql_affected_rows(conn);
                LOG(INFO) << "mysql: " << cnt << "rows afected";
                json ans;
                ans["OK"] = true;
                return ans;
            }
            else
            {
                LOG(WARNING) << "mysql wrong!";
                json ans;
                ans["OK"] = false;
                return ans;
            }
        }
    }
}

MSQL::~MSQL()
{
    mysql_close(conn);
    delete conn;
}