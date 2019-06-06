#ifndef DLL_H
#define DLL_H
#include "head.h"
#include "util.h"
#include "FileManager.h"


class User
{
private:
    int m_epollfd;
    int m_sockfd;
public:
    string password;
    string username;
    string u_id;
    string dept;
    string role_type;
    string sex;
    string birth;
    bool is_login;

    FileManager *file_manager;
public:
    User(const string &IP, const int PORT);
    ~User();
    bool c_login(string _username, string _password);
    bool c_register();
    bool c_update_info(const string &password, const string &sex, const string &birth, const string &dept);
    json c_get_course_list();
    //讨论区
    json c_get_topic(const string &course_id);
    json c_get_comment(const string &topic_id);
    json c_get_reply(const string &comment_id);
    bool c_insert_topic(const string &course_id, const string &content);
    bool c_insert_reply(const string &topic_id, const string &to, const string &content);
    bool c_insert_comment(const string &topic_id, const string &content);
    //邮件系统
    bool c_send_email(const string &to, const string &topic, const string &content);
    json c_get_email();
    json c_get_draft_email();
    json c_get_unread_email();
    bool c_del_email(const string &e_id);
    bool c_save_draft_email(const string &to, const string &topic, const string &content);
    bool download(const string &file_path);
    bool upload(const string &file_path);
    bool delete_file(const string &file);

    //教师特权
    json c_get_student_list(const string &c_id);

    json c_get_course_files(const string &course_id);
    json c_get_course_notice(const string &course_id);
    bool c_publish_course_notice(const string &course_id, const string &content);
    bool c_update_absent(const string &user_id, const int cnt);
    bool c_update_grade(const string &user_id, const double grade);
    bool c_register_course(const string &course_id, const string &course_name);

};

#endif 