#include "dll.h"
#include "util.h"
User::User(const string &IP, const int PORT)
{
    m_sockfd = unblock_connect(IP.c_str(), PORT, 10);
    assert(m_sockfd >= 0);
    m_epollfd = epoll_create(5);
    addfd(m_epollfd, m_sockfd, true);
    is_login = false;
}
User::~User()
{
    removefd(m_epollfd, m_sockfd);
    close(m_epollfd);
    cout<<"close fd and epollfd\n";
}
bool User::c_login(string _u_id, string _password)
{
    json msg;
    msg["type"] = LOGIN;
    msg["u_id"] = _u_id;
    msg["password"] = _password;
    send_json(m_sockfd, msg);
    epoll_event events[10];
    int res = epoll_wait(m_epollfd, events, 10, -1);
    assert(res == 1);
    json task;
    receive_json(m_epollfd, m_sockfd, task);
    if(task["OK"].get<bool>())
    {
        is_login = true;
        file_manager = new FileManager(_u_id, 1);
        u_id = _u_id;
        password = _password;
        sex = task["info"][0]["SEX"].get<string>();
        //u_id = task["info"][0]["U_ID"].get<string>();
        username = task["info"][0]["USERNAME"].get<string>();
        dept = task["info"][0]["DEPT"].get<string>();
        birth = task["info"][0]["BIRTH"].get<string>();
        role_type = task["info"][0]["ROLE_TYPE"].get<string>();
        return true;
    }
    else
    {
        return false;
    }
}

bool User::c_register()
{
    json msg;
    msg["type"] = REGISTER;
    msg["username"] = username;
    msg["password"] = password;
    msg["u_id"] = u_id;
    msg["role_type"] = role_type;
    msg["sex"] = sex;
    msg["birth"] = birth;
    msg["dept"] = dept;
    send_json(m_sockfd, msg);
    epoll_event events[10];
    int res = epoll_wait(m_epollfd, events, 10, -1);
    assert(res == 1);
    json task;
    receive_json(m_epollfd, m_sockfd, task);
    if(task["OK"].get<bool>())
    {
        return true;
    }
    else
    {
        return false;
    }
    
}
bool User::c_update_info(const string &_password, const string &_sex, 
                         const string &_birth, const string &_dept)
{
    json msg;
    msg["type"] = UPDATE_INFO;
    msg["username"] = username;
    msg["password"] = _password;
    msg["u_id"] = u_id;
    msg["role_type"] = role_type;
    msg["sex"] = _sex;
    msg["birth"] = _birth;
    msg["dept"] = _dept;
    send_json(m_sockfd, msg);
    epoll_event events[10];
    int res = epoll_wait(m_epollfd, events, 10, -1);
    assert(res == 1);
    json task;
    receive_json(m_epollfd, m_sockfd, task);
    if(task["OK"].get<bool>())
    {
        password = _password;
        sex = _sex;
        birth = _birth;
        dept = _dept;
        return true;
    }
    else
    {
        return false;
    }
}
bool User::download(const string &path)
{
    return file_manager->download(path);
}
bool User::upload(const string &path, const string &dst)
{
    return file_manager->upload(path, dst);
}
bool User::upload_ppt(const string &course_name, const string &file_name)
{
    return upload(file_name, course_name + "/ppt");
}
bool User::download_ppt(const string &file_path)
{
    return download(file_path);
}
bool User::upload_homework(const string & course_name, const string &path)
{
    return upload(path, course_name + "/homework");
}
bool User::download_homework(const string & course_name)
{
    string temp = course_name + "/homework";
    return download(temp);
}

bool User::delete_file(const string &file)
{
    return file_manager->delete_file(file);
}
json User::c_get_topic(const string &course_id)
{
    json msg;
    msg["type"] = GET_TOPIC;
    msg["M_ABOUT"] = course_id;
    send_json(m_sockfd, msg);
    epoll_event events[10];
    int res = epoll_wait(m_epollfd, events, 10, -1);
    assert(res == 1);
    json task;
    receive_json(m_epollfd, m_sockfd, task);
    return task;
}
json User::c_get_comment(const string &topic_id)
{
    json msg;
    msg["type"] = GET_COMMENT;
    msg["topic_id"] = topic_id;
    send_json(m_sockfd, msg);
    epoll_event events[10];
    int res = epoll_wait(m_epollfd, events, 10, -1);
    assert(res == 1);
    json task;
    receive_json(m_epollfd, m_sockfd, task);
    return task;
}
json User::c_get_reply(const string &comment_id)
{
    json msg;
    msg["type"] = GET_REPLY;
    msg["comment_id"] = comment_id;
    send_json(m_sockfd, msg);
    epoll_event events[10];
    int res = epoll_wait(m_epollfd, events, 10, -1);
    assert(res == 1);
    json task;
    receive_json(m_epollfd, m_sockfd, task);
    return task;
}
bool User::c_insert_topic(const string &course_id, const string &content)
{
    json msg;
    msg["type"] = INSERT_TOPIC;
    msg["M_AUTHOR"] = u_id;
    msg["M_ABOUT"] = course_id;
    msg["M_CONTENT"] = content;
    send_json(m_sockfd, msg);
    epoll_event events[10];
    int res = epoll_wait(m_epollfd, events, 10, -1);
    assert(res == 1);
    json task;
    receive_json(m_epollfd, m_sockfd, task);
    return task["OK"].get<bool>();
}
bool User::c_insert_comment(const string &topic_id, const string &content)
{
    json msg;
    msg["type"] = INSERT_COMMENT;
    msg["M_AUTHOR"] = u_id;
    msg["topic_id"] = topic_id;
    msg["M_CONTENT"] = content;
    send_json(m_sockfd, msg);
    epoll_event events[10];
    int res = epoll_wait(m_epollfd, events, 10, -1);
    assert(res == 1);
    json task;
    receive_json(m_epollfd, m_sockfd, task);
    return task["OK"].get<bool>();

}

bool User::c_insert_reply(const string &topic_id, const string &to, const string &content)
{
    json msg;
    msg["type"] = INSERT_REPLY;
    msg["M_AUTHOR"] = u_id;
    msg["comment_id"] = topic_id;
    msg["M_TO"] = to;
    msg["M_CONTENT"] = content;
    send_json(m_sockfd, msg);
    epoll_event events[10];
    int res = epoll_wait(m_epollfd, events, 10, -1);
    assert(res == 1);
    json task;
    receive_json(m_epollfd, m_sockfd, task);
    return task["OK"].get<bool>();
}
bool User::c_send_email(const string &to, const string &topic, const string &content)
{
    json msg;
    msg["E_FROM"] = u_id;
    msg["E_TO"] = to;
    msg["E_TOPIC"] = topic;
    msg["E_CONTENT"] = content;
    msg["type"] = SEND_EMAIL;
    send_json(m_sockfd, msg);
    epoll_event events[10];
    int res = epoll_wait(m_epollfd, events, 10, -1);
    assert(res == 1);
    json task;
    receive_json(m_epollfd, m_sockfd, task);
    return task["OK"].get<bool>();
}

json User::c_get_email()
{
    json msg;
    msg["E_FROM"] = u_id;
    msg["type"] = GET_EMAIL;
    send_json(m_sockfd, msg);
    epoll_event events[10];
    int res = epoll_wait(m_epollfd, events, 10, -1);
    assert(res == 1);
    json task;
    receive_json(m_epollfd, m_sockfd, task);
    return task;
}
json User::c_get_draft_email()
{
    json msg;
    msg["E_FROM"] = u_id;
    msg["type"] = GET_DRAFT_EMAIL;
    send_json(m_sockfd, msg);
    epoll_event events[10];
    int res = epoll_wait(m_epollfd, events, 10, -1);
    assert(res == 1);
    json task;
    receive_json(m_epollfd, m_sockfd, task);
    return task;

}
json User::c_get_unread_email()
{
    json msg;
    msg["E_FROM"] = u_id;
    msg["type"] = GET_UNREAD_EMAIL;
    send_json(m_sockfd, msg);
    epoll_event events[10];
    int res = epoll_wait(m_epollfd, events, 10, -1);
    assert(res == 1);
    json task;
    receive_json(m_epollfd, m_sockfd, task);
    return task;

}
bool User::c_mark_email(const string &e_id)
{
    json msg;
    msg["type"] = MARK_EMAIL;
    msg["e_id"] = e_id;
    send_json(m_sockfd, msg);
    epoll_event events[10];
    int res = epoll_wait(m_epollfd, events, 10, -1);
    assert(res == 1);
    json task;
    receive_json(m_epollfd, m_sockfd, task);
    return task["OK"].get<bool>();

}
bool User::c_del_email(const string &e_id)
{
    json msg;
    msg["E_ID"] = e_id;
    msg["type"] = DEL_EMAIL;
    send_json(m_sockfd, msg);
    epoll_event events[10];
    int res = epoll_wait(m_epollfd, events, 10, -1);
    assert(res == 1);
    json task;
    receive_json(m_epollfd, m_sockfd, task);
    return task["OK"].get<bool>();
}
bool User::c_save_draft_email(const string &to, const string &topic, const string &content)
{
    json msg;
    msg["E_FROM"] = u_id;
    msg["E_TO"] = to;
    msg["E_TOPIC"] = topic;
    msg["E_CONTENT"] = content;
    msg["type"] = SAVE_DRAFT_EMAIL;
    send_json(m_sockfd, msg);
    epoll_event events[10];
    int res = epoll_wait(m_epollfd, events, 10, -1);
    assert(res == 1);
    json task;
    receive_json(m_epollfd, m_sockfd, task);
    return task["OK"].get<bool>();
}
json User::c_get_student_list(const string &c_id)
{
    json msg;
    msg["type"] = GET_STUDENT_LIST;
    msg["C_ID"] = c_id;
    send_json(m_sockfd, msg);
    epoll_event events[10];
    int res = epoll_wait(m_epollfd, events, 10, -1);
    assert(res == 1);
    json task;
    receive_json(m_epollfd, m_sockfd, task);
    return task;
}
json User::c_get_course_list()
{
    json msg;
    msg["type"] = GET_COURSE_LIST;
    msg["USERID"] = u_id;
    send_json(m_sockfd, msg);
    epoll_event events[10];
    int res = epoll_wait(m_epollfd, events, 10, -1);
    assert(res == 1);
    json task;
    receive_json(m_epollfd, m_sockfd, task);
    return task;
}
json User::c_get_course_files(const string &course_id)
{
    json msg;
    msg["type"] = GET_COURSE_FILES;
    msg["C_ID"] = course_id;
    msg["role_type"] = role_type;
    send_json(m_sockfd, msg);
    epoll_event events[10];
    int res = epoll_wait(m_epollfd, events, 10, -1);
    assert(res == 1);
    json task;
    receive_json(m_epollfd, m_sockfd, task);
    return task;
}
json User::c_get_course_notice(const string &course_id)
{
    json msg;
    msg["type"] = GET_COURSE_NOTICE;
    msg["C_ID"] = course_id;
    send_json(m_sockfd, msg);
    epoll_event events[10];
    int res = epoll_wait(m_epollfd, events, 10, -1);
    assert(res == 1);
    json task;
    receive_json(m_epollfd, m_sockfd, task);
    return task;

}
bool User::c_publish_course_notice(const string &course_id, const string &content)
{
    json msg;
    msg["type"] = PUBLISH_COURSE_NOTICE;
    msg["C_ID"] = course_id;
    msg["USER_ID"] = u_id;
    msg["CONTENT"] = content;
    send_json(m_sockfd, msg);
    epoll_event events[10];
    int res = epoll_wait(m_epollfd, events, 10, -1);
    assert(res == 1);
    json task;
    receive_json(m_epollfd, m_sockfd, task);
    if(task["OK"].get<bool>())
    {
        return true;
    }
    else
    {
        return false;
    }
}
bool User::c_update_absent(const string &user_id, const int cnt)
{
    json msg;
    msg["type"] = UPDATE_ABSENT;
    msg["USER_ID"] = user_id;
    msg["CNT"] = cnt;
    send_json(m_sockfd, msg);
    epoll_event events[10];
    int res = epoll_wait(m_epollfd, events, 10, -1);
    assert(res == 1);
    json task;
    receive_json(m_epollfd, m_sockfd, task);
    if(task["OK"].get<bool>())
    {
        return true;
    }
    else
    {
        return false;
    }

}
bool User::c_update_grade(const string &user_id, const double grade)
{
    json msg;
    msg["type"] = UPDATE_GRADE;
    msg["USER_ID"] = user_id;
    msg["GRADE"] = grade;
    send_json(m_sockfd, msg);
    epoll_event events[10];
    int res = epoll_wait(m_epollfd, events, 10, -1);
    assert(res == 1);
    json task;
    receive_json(m_epollfd, m_sockfd, task);
    if(task["OK"].get<bool>())
    {
        return true;
    }
    else
    {
        return false;
    }

}
bool User::c_register_course(const string &course_id, const string &course_name)
{
    json msg;
    msg["type"] = REGISTER_COURSE;
    msg["C_ID"] = course_id;
    msg["C_NAME"] = course_name;
    msg["USER_ID"] = u_id;
    send_json(m_sockfd, msg);
    epoll_event events[10];
    int res = epoll_wait(m_epollfd, events, 10, -1);
    assert(res == 1);
    json task;
    receive_json(m_epollfd, m_sockfd, task);
    if(task["OK"].get<bool>())
    {
        return true;
    }
    else
    {
        return false;
    }
}