#include "Worker.h"


int Worker::m_epollfd = -1;
Worker::Worker()
{
    conn = nullptr;
}

void Worker::init(int epollfd, int sockfd, const sockaddr_in &client_addr)
{
    m_epollfd = epollfd;
    m_sockfd = sockfd;
    m_address = client_addr;
    //memset(m_buf, 0, sizeof m_buf);

    //string IP = "199.115.231.228";
    string IP = __C["MYSQL"]["IP"].get<string>();
    string USER = __C["MYSQL"]["USERNAME"].get<string>();
    string PASSWORD = __C["MYSQL"]["PASSWORD"].get<string>();
    string DATABASE = __C["MYSQL"]["DATABASE"].get<string>();
    int PORT = __C["MYSQL"]["PORT"].get<int>();
    //conn = std::make_shared<MSQL>(new MSQL(IP.c_str(), USER.c_str(), PASSWORD.c_str(), DATABASE.c_str()));
    //conn = new MSQL(IP.c_str(), USER.c_str(), PASSWORD.c_str(), 9966);
    conn = new MSQL(IP.c_str(), USER.c_str(), PASSWORD.c_str(), DATABASE.c_str(), PORT);
}

void Worker::process()
{
    int res = -1;
    string msg = "";
    /*
    while(true) //ET模式, 读到EAGAIN || EWOULDBLOCK为止
    {
        memset(m_buf, 0, sizeof __C["BUF_SIZE"].get<int>());
        int res = read(m_sockfd, m_buf, __C["BUF_SIZE"].get<int>() - 1);
        if(res < 0)
        {
            if((errno == EWOULDBLOCK) || (errno == EAGAIN))  //对于非阻塞IO，表示已经读完
            {
                printf("read later\n");
                break;
            }
            removefd(m_epollfd, m_sockfd); //关闭连接
            break;
        }
        else if(res == 0)  //检测到客户端关闭了连接
        {
            removefd(m_epollfd, m_sockfd); //关闭连接
            return;   //!!
        }
        else
        {
            //printf("get %d bytes of content: %s\n", res, buf);
            msg += string(m_buf, m_buf + res);
        }
    }
    cout<<msg<<endl;
    std::stringstream ss(msg);
    json cfg;
    ss >> cfg;
    */
    json cfg;
    auto closed = receive_json(m_epollfd, m_sockfd, cfg);
    LOG(INFO) << "MSG: \n" << cfg.dump(4);
    if(closed)
    {
        removefd(m_epollfd, m_sockfd);
        LOG(INFO) << "closed client: " << m_sockfd;
        delete conn;
        return ;
    }
    //cout<<"客户端发来的消息：\n";
    //cout<<cfg.dump(4)<<endl;
    auto type = cfg["type"].get<MSG_TYPE>();
    switch (type)
    {
    case LOGIN:
    {
        //登录
        //参数：用户、密码
        LOG(INFO) << "LOGIN";
        auto username = cfg["username"].get<string>();
        auto password = cfg["password"].get<string>();
        auto res = s_login(conn, username, password);
        if(res["OK"].get<bool>()) //登录成功
        {
            LOG(INFO) << username << " 登录成功";
            send_json(m_sockfd, res);
        }
        else //登录失败,用户名或密码错误
        {
            LOG(WARNING) << username << " 登录失败";
            json msg;
            msg["OK"] = false;
            send_json(m_sockfd, msg);
        }
        break;
    }
    case  REGISTER:
    {
        LOG(INFO) << "REGISTER";
        //注册用户
        //参数：用户名、用户id、密码、用户类别、性别、生日、系
        auto username = cfg["username"].get<string>();
        auto u_id = cfg["u_id"].get<string>();
        auto res = conn->exec("select * from User where U_ID=\'" + u_id + "\'");
        if(res["info"].size() != 0)
        {
            //学号已被注册
            //TODO: 更友好的提示
            LOG(WARNING) << "注册失败： 学号已被注册";
            json msg;
            msg["OK"] = false;
            msg["ERROR"] = "user_id";
            send_json(m_sockfd, msg);
            break;
        } 
        res = conn->exec("select * from User where USERNAME=\'" + username + "\'");
        if(res["info"].size() != 0)
        {
            //用户名已被注册
            //TODO
            LOG(WARNING) << "注册失败： 用户名已被注册";
            json msg;
            msg["OK"] = false;
            msg["ERROR"] = "username";
            send_json(m_sockfd, msg);
            break;
        }
        auto temp = s_register(conn, cfg);
        if(temp) //注册成功
        {
            LOG(INFO) << "注册成功： ?";
            json msg;
            msg["OK"] = true;
            send_json(m_sockfd, msg);
        }
        else //注册失败，
        {
            LOG(WARNING) << "注册失败： ?";
            json msg;
            msg["OK"] = false;
            send_json(m_sockfd, msg);
        }
        break;
    }
    case  UPDATE_INFO:
    {
        LOG(INFO) << "UPDATE_INFO";
        //修改用户信息
        //参数：用户名、用户id、密码、用户类别、性别、生日、系
        //auto username = cfg["username"].get<string>();
        //auto res = conn->exec("select * from User where USERNAME=\'" + username + "\'");
        //if(res.size() != 0)
        //{
        //    //用户名已被注册
        //    //TODO
        //    json msg;
        //    msg["OK"] = false;
        //    send_json(m_sockfd, msg);
        //    break;
        //}
        auto temp = s_update_info(conn, cfg);
        if(temp) //修改成功
        {
            json msg;
            LOG(INFO) << "修改成功";
            msg["OK"] = true;
            send_json(m_sockfd, msg);
        }
        else //修改失败，
        {
            LOG(WARNING) << "修改失败";
            json msg;
            msg["OK"] = false;
            send_json(m_sockfd, msg);
        }
        break;

    }
    case GET_TOPIC: 
    {
        //得到某门课的讨论区的话题
        //需要传参：课程名

        //获取课程id
        LOG(INFO) << "GET_TOPIC";
        //auto res = conn->exec("select C_ID from Course where C_ID=\'" + cfg["M_ABOUT"].get<string>() + '\'');
        string course_id = cfg["M_ABOUT"].get<string>();


        auto res = conn->exec("select M_ID, M_AUTHOR, USERNAME, M_CONTENT, M_TIME from Msg, User \
        where User.U_ID=Msg.M_AUTHOR and M_TYPE=\"TOPIC\" and M_ABOUT=\"" + course_id + '\"');
        //返回id，作者id， 作者用户名, 内容，时间

        send_json(m_sockfd, res);
        break;
    }
    case GET_COMMENT:
    {
        //得到某个话题的评论
        //参数：话题ID
        LOG(INFO) << "GET_COMMENT";
        auto topic_id = cfg["topic_id"].get<string>();
        auto res = conn->exec("select M_ID, M_AUTHOR, USERNAME, M_CONTENT, M_TO, M_TIME from Msg, User \
        where User.U_ID=Msg.M_AUTHOR and M_TYPE=\"COMMENT\" and M_ABOUT=\"" + topic_id + '\"'); 
        //返回id，作者id，作者用户名，内容，回复对象，时间
        send_json(m_sockfd, res);
        break;
    }
    case GET_REPLY:
    {
        //得到某个评论的回复
        //参数：评论ID
        LOG(INFO) << "GET_REPLY";
        auto comment_id = cfg["comment_id"].get<string>();
        auto res = conn->exec("select M_ID, M_AUTHOR, USERNAME, M_CONTENT, M_TO, M_TIME from Msg, User \
        where User.U_ID=Msg.M_AUTHOR and M_TYPE=\'REPLY\' and M_ABOUT=\'" + comment_id + '\''); //注意: 回复内容的M_ABOUT字段表示关于哪个话题
        //返回id，作者id, 作者用户名，内容，回复对象，时间
        send_json(m_sockfd, res);
        break;
    }
    case INSERT_TOPIC:
    {
        //插入话题
        //参数：作者id、课程id、内容

        //获取用户id
        //auto res = conn->exec("select U_ID from User where USERNAME=\'" + cfg["M_AUTHOR"].get<string>() + '\'');
        //string user_id = res["info"][0]["U_ID"].get<string>();
        //cout<<user_id<<"  "<<endl;
        LOG(INFO) << "INSERT_TOPIC";
        string user_id = cfg["M_AUTHOR"].get<string>();//get_user_id(conn, cfg["M_AUTHOR"].get<string>());

        //获取课程id
        //auto res = conn->exec("select C_ID from Course where C_NAME=\'" + cfg["M_ABOUT"].get<string>() + '\'');
        string course_id = cfg["M_ABOUT"].get<string>();//res["info"][0]["C_ID"].get<string>();

        //插入评论
        //char buf[__C["BUF_SIZE"].get<int>()*4];
        //memset(buf, 0, sizeof buf);
        //snprintf(buf, __C["BUF_SIZE"].get<int>()*4 - 1, "insert into Msg(M_AUTHOR, M_TYPE, M_ABOUT, M_CONTENT, M_TIME) \
        //values(%s, \'TOPIC\', %s, \"%s\", now())", 
        //user_id.c_str(), course_id.c_str(), cfg["M_CONTENT"].get<string>().c_str());
        string buf =  "insert into Msg(M_AUTHOR, M_TYPE, M_ABOUT, M_CONTENT, M_TIME) \
        values(" + user_id +  ", \"TOPIC\", " + course_id + ", \"" + cfg["M_CONTENT"].get<string>() + "\", now())";

        auto res = conn->exec(buf);
        send_json(m_sockfd, res);
        if(res["OK"].get<bool>())
        {
            LOG(INFO) << "插入TOPIC成功";
        }
        else
        {
            LOG(WARNING) << "插入TOPIC失败";
        }
        break;
    }
    case INSERT_COMMENT:
    {
        //插入评论
        //参数：作者、话题id、内容
        //auto res = conn->exec("select U_ID from User where USERNAME=\"" + cfg["M_AUTHOR"].get<string>() + '\"');
        //string user_id = res["info"][0]["U_ID"].get<string>();
        LOG(INFO) << "INSERT_COMMENT";
        string user_id = cfg["M_AUTHOR"].get<string>();//get_user_id(conn, cfg["M_AUTHOR"].get<string>());

        //char buf[__C["BUF_SIZE"].get<int>()*4];
        //memset(buf, 0, sizeof buf);
        //snprintf(buf, __C["BUF_SIZE"].get<int>()*4 - 1, "insert into Msg(M_AUTHOR, M_TYPE, M_ABOUT, M_CONTENT, M_TIME) \
        //values(%s, \"COMMENT\", %s, \"%s\", now())", 
        //user_id.c_str(), cfg["topic_id"].get<string>().c_str(), cfg["M_CONTENT"].get<string>().c_str());
        string buf = "insert into Msg(M_AUTHOR, M_TYPE, M_ABOUT, M_CONTENT, M_TIME) \
        values(" + user_id + ", \"COMMENT\", " + cfg["topic_id"].get<string>() + ", \"" + cfg["M_CONTENT"].get<string>() + "\", now())";

        auto res = conn->exec(buf);
        send_json(m_sockfd, res);
        if(res["OK"].get<bool>())
        {
            LOG(INFO) << "插入COMMENT成功";
        }
        else
        {
            LOG(WARNING) << "插入COMMENT失败";
        }
        break;
    }
    case INSERT_REPLY:
    {
        //插入回复
        //参数：作者、评论id、回复对象id
        LOG(INFO) << "INSERT_REPLY";
        //string user_id = get_user_id(conn, cfg["M_AUTHOR"].get<string>());
        //string to_id = get_user_id(conn, cfg["M_TO"].get<string>());
        string user_id = cfg["M_AUTHOR"].get<string>();//get_user_id(conn, cfg["M_AUTHOR"].get<string>());
        string to_id = cfg["M_TO"].get<string>();//get_user_id(conn, cfg["M_AUTHOR"].get<string>());
        //auto res = conn->exec("select U_ID from User where USERNAME=\"" + cfg["M_AUTHOR"].get<string>() + '\"');
        //string user_id = res["info"][0]["U_ID"].get<string>();
        //res = conn->exec("select U_ID from User where USERNAME=\"" + cfg["M_TO"].get<string>() + '\"');
        //string to_id = res["info"][0]["U_ID"].get<string>();

        //char buf[__C["BUF_SIZE"].get<int>()*4];
        //memset(buf, 0, sizeof buf);
        //snprintf(buf, __C["BUF_SIZE"].get<int>()*4 - 1, "insert into Msg(M_AUTHOR, M_TYPE, M_ABOUT, M_TO, M_CONTENT, M_TIME) \
        //values(%s, \"REPLY\", %s, %s, \"%s\", now())", 
        //user_id.c_str(), cfg["comment_id"].get<string>().c_str(), to_id.c_str(), cfg["M_CONTENT"].get<string>().c_str());
        string buf = "insert into Msg(M_AUTHOR, M_TYPE, M_ABOUT, M_TO, M_CONTENT, M_TIME) \
        values(" + user_id + ", \"REPLY\", " + cfg["comment_id"].get<string>() + ", " + to_id + ", \"" + cfg["M_CONTENT"].get<string>() + "\", now())";

        auto res = conn->exec(buf);
        send_json(m_sockfd, res);
        if(res["OK"].get<bool>())
        {
            LOG(INFO) << "插入REPLY成功";
        }
        else
        {
            LOG(WARNING) << "插入REPLY失败";
        }
        break;
    }
    case SEND_EMAIL:
    {
        //发邮件
        //参数：发信人id、收信人用户名、主题、内容
        //auto res = conn->exec("select U_ID from User where USERNAME=\"" + cfg["E_FROM"].get<string>() + '\"');
        //string from_id = res["info"][0]["U_ID"].get<string>();
        //res = conn->exec("select U_ID from User where USERNAME=\"" + cfg["E_TO"].get<string>() + '\"');
        //string to_id = res["info"][0]["U_ID"].get<string>();
        LOG(INFO) << "SEND_EMAIL";
        auto from_id = cfg["E_FROM"].get<string>();//get_user_id(conn, cfg["E_FROM"].get<string>());
        auto to_id = get_user_id(conn, cfg["E_TO"].get<string>());

        //插入邮箱，类型为未读
        char buf[__C["BUF_SIZE"].get<int>() * 8];
        memset(buf, 0, sizeof buf);
        snprintf(buf, __C["BUF_SIZE"].get<int>() * 8 - 1, "insert into EMAIL(E_FROM, E_TO, E_TOPIC, E_CONTENT, E_TIME, E_TYPE)\
        values(%s, %s, \"%s\", \"%s\", now(), 0)", from_id.c_str(), to_id.c_str(), 
        cfg["E_TOPIC"].get<string>().c_str(), cfg["E_CONTENT"].get<string>().c_str());
        auto res = conn->exec(buf);
        send_json(m_sockfd, res);
        if(res["OK"].get<bool>())
        {
            LOG(INFO) << "发送邮件成功";
        }
        else
        {
            LOG(WARNING) << "发送邮件失败";
        }
        break;
    }
    case GET_EMAIL:
    {
        //查看收件箱（包含未读和已读邮件）
        //参数：用户id
        //auto res = conn->exec("select U_ID from User where USERNAME=\"" + cfg["E_FROM"].get<string>() + '\"');
        //string user_id = res["info"][0]["U_ID"].get<string>();
        LOG(INFO) << "GET_EMAIL";
        auto user_id = cfg["E_FROM"].get<string>();//get_user_id(conn, cfg["E_FROM"].get<string>());
        auto res = conn->exec("select * from EMAIL where E_TO=\"" + user_id + "\"");
        send_json(m_sockfd, res);  
        if(res["OK"].get<bool>())
        {
            LOG(INFO) << "请求收件箱成功";
        }
        else
        {
            LOG(WARNING) << "请求收件箱失败";
        }
        break;
    }
    case GET_DRAFT_EMAIL:
    {
        //获取草稿箱内容
        //参数：用户id
        //auto res = conn->exec("select U_ID from User where USERNAME=\"" + cfg["E_FROM"].get<string>() + '\"');
        //string user_id = res["info"][0]["U_ID"].get<string>();
        LOG(INFO) << "GET_DRAFT_EMAIL";
        //auto user_id = get_user_id(conn, cfg["E_FROM"].get<string>());
        auto user_id = cfg["E_FROM"].get<string>();//get_user_id(conn, cfg["E_FROM"].get<string>());
        auto res = conn->exec("select * from EMAIL where E_FROM=\"" + user_id + "\" and E_TYPE=2"); //类型是草稿
        send_json(m_sockfd, res);
        if(res["OK"].get<bool>())
        {
            LOG(INFO) << "请求草稿箱成功";
        }
        else
        {
            LOG(WARNING) << "请求草稿箱失败";
        }
        break;
    }
    case GET_UNREAD_EMAIL:
    {
        //获取未读邮件
        //参数：用户id
        //auto user_id = get_user_id(conn, cfg["E_FROM"].get<string>());
        auto user_id = cfg["E_FROM"].get<string>();//get_user_id(conn, cfg["E_FROM"].get<string>());
        auto res = conn->exec("select * from EMAIL where E_TO=\"" + user_id + "\" and E_TYPE=0"); //类型是草稿
        send_json(m_sockfd, res);
        if(res["OK"].get<bool>())
        {
            LOG(INFO) << "请求未读邮件成功";
        }
        else
        {
            LOG(WARNING) << "请求未读邮件失败";
        }
        break;
    }
    case DEL_EMAIL:
    {
        //删除邮件
        //参数：邮件id
        LOG(INFO) << "DEL_EMAIL";
        auto res = conn->exec("delete from EMAIL where E_ID=\"" + cfg["E_ID"].get<string>() + "\"");
        send_json(m_sockfd, res);
        if(res["OK"].get<bool>())
        {
            LOG(INFO) << "删除邮件成功";
        }
        else
        {
            LOG(WARNING) << "删除邮件失败";
        }
        break;
    }
    case SAVE_DRAFT_EMAIL:
    {
        //发邮件
        //参数：发信人id、收信人用户名、主题、内容
        //如果收信人还没有填就先设为NULL!!
        LOG(INFO) << "SAVE_DRAFT_EMAIL";
        auto from_id = cfg["E_FROM"].get<string>();//get_user_id(conn, cfg["E_FROM"].get<string>());
        auto to_id = get_user_id(conn, cfg["E_TO"].get<string>());

        //插入邮箱，类型为草稿
        char buf[__C["BUF_SIZE"].get<int>() * 8];
        memset(buf, 0, sizeof buf);
        snprintf(buf, __C["BUF_SIZE"].get<int>() * 8 - 1, "insert into EMAIL(E_FROM, E_TO, E_TOPIC, E_CONTENT, E_TIME, E_TYPE)\
        values(%s, %s, \"%s\", \"%s\", now(), 2)", from_id.c_str(), to_id.c_str(), 
        cfg["E_TOPIC"].get<string>().c_str(), cfg["E_CONTENT"].get<string>().c_str());
        auto res = conn->exec(buf);
        send_json(m_sockfd, res);
        if(res["OK"].get<bool>())
        {
            LOG(INFO) << "保存草稿成功";
        }
        else
        {
            LOG(WARNING) << "保存草稿失败";
        }
        break;
    }
    case GET_STUDENT_LIST:
    {
        //获取学生列表
        //参数：课程id
        LOG(INFO) << "GET_STUDENT_LIST";
        auto res = conn->exec("select USERNAME, U_ID from User, SC where SC.U_ID=User.U_ID and SC.C_ID=" + cfg["C_ID"].get<string>());
        send_json(m_sockfd, res);
        if(res["OK"].get<bool>())
        {
            LOG(INFO) << "获取学生列表成功";
        }
        else
        {
            LOG(WARNING) << "获取学生列表失败";
        }
        break;
    }
    case GET_COURSE_LIST:
    {
        //获取教师教课列表
        //参数：用户id
        LOG(INFO) << "GET_COURSE_LIST";
        auto user_id = cfg["USERID"].get<string>();//get_user_id(conn, cfg["USERNAME"].get<string>());
        auto temp = conn->exec("select ROLE_TYPE from User where U_ID=" + user_id);
        json res;
        if(temp["info"][0]["ROLE_TYPE"] == "student")
        {
            res = conn->exec("select C_ID, C_NAME, T_ID from SC, Course where \
            Course.C_ID=SC.C_ID and SC.U_ID=" + user_id);
        }
        else
        {
            res = conn->exec("select C_ID, C_NAME, T_ID, USERNAME from User, Course where \
            Course.T_ID=User.U_ID and User.U_ID=" + user_id);
        }
        
        //int n = res["info"].size();
        //for(int i = 0; i < n; i++)
        //{
        //    res["info"][i]["T_NAME"] = get_user_name(conn, res["info"][i]["T_ID"].get<string>());
        //}
        send_json(m_sockfd, res);
        if(res["OK"].get<bool>())
        {
            LOG(INFO) << "获取课程列表成功";
        }
        else
        {
            LOG(WARNING) << "获取课程列表失败";
        }
        break;
    }
    case GET_COURSE_FILES:
    {
        //获取课程资源列表
        //参数：课程id
        LOG(INFO) << "GET_COURSE_FILES";
        string c_id = cfg["C_ID"].get<string>();
        auto res = conn->exec("select C_NAME from Course where C_ID=" + string("\"") + c_id + "\"");
        string course_name = res["info"][0]["C_NAME"];
        json msg;
        files_to_json("../home/" + course_name, msg);
        send_json(m_sockfd, msg);
        break;
    }
    case GET_COURSE_NOTICE:
    {
        //获取课程通知
        //参数：课程id
        LOG(INFO) << "GET_COURSE_NOTICE";
        string c_id = cfg["C_ID"].get<string>();
        auto res = conn->exec("select * from Msg where M_TYPE=\"NOTICE\" and M_ABOUT=" + c_id);
        send_json(m_sockfd, res);
        break;
    }
    case PUBLISH_COURSE_NOTICE:
    {
        //发布通知
        //参数：课程id，作者id，通知内容
        string c_id = cfg["C_ID"].get<string>();
        string user_id = cfg["USER_ID"].get<string>();
        string content = cfg["CONTENT"].get<string>();
        char buf[__C["BUF_SIZE"].get<int>() * 8];
        memset(buf, 0, sizeof buf);
        snprintf(buf, __C["BUF_SIZE"].get<int>() * 8 - 1, "insert into Msg(M_AUTHOR, M_TYPE, M_CONTENT, M_ABOUT, M_TIME)\
        values(%s, \"NOTICE\", \"%s\", \"%s\", now())", user_id.c_str(), content.c_str(), c_id.c_str());
        auto res = conn->exec(buf);
        send_json(m_sockfd, res);
        if(res["OK"].get<bool>())
        {
            LOG(INFO) << "发布通知成功";
        }
        else
        {
            LOG(WARNING) << "发布通知失败";
        }
        break;
    }
    case UPDATE_ABSENT:
    {
        //更新缺勤情况
        //参数：学生id, 修改情况（+1, -1等）
        LOG(INFO) << "UPDATE_ABSENT";
        string user_id = cfg["USER_ID"].get<string>();
        int cnt = cfg["CNT"].get<int>();
        auto temp = conn->exec("select ABSENT_CNT from SC where U_ID=" + user_id);
        int init = temp["info"][0]["ABSENT_CNT"].get<int>();
        auto res = conn->exec("update SC set ABSENT_CNT=" + std::to_string(init + cnt) + " where U_ID=" + user_id);
        send_json(m_sockfd, res);
        if(res["OK"].get<bool>())
        {
            LOG(INFO) << "修改缺勤次数成功";
        }
        else
        {
            LOG(WARNING) << "修改缺勤次数失败";
        }
        break;
    }
    case UPDATE_GRADE:
    {
        //录入成绩
        //参数：学生id, 分数
        LOG(INFO) << "UPDATE_GRADE";
        string user_id = cfg["USER_ID"].get<string>();
        double cnt = cfg["GRADE"].get<double>();
        auto res = conn->exec("update SC set GRADE=" + std::to_string(cnt) + " where U_ID=" + user_id);
        send_json(m_sockfd, res);
        if(res["OK"].get<bool>())
        {
            LOG(INFO) << "成绩录入成功";
        }
        else
        {
            LOG(WARNING) << "成绩录入失败";
        }
        break;
    }
    case REGISTER_COURSE:
    {
        //注册课程
        //参数：教师用户id,课程id，课程名
        LOG(INFO) << "REGISTER_COURSE";
        string user_id = cfg["USER_ID"].get<string>();
        string c_id = cfg["C_ID"].get<string>();
        string c_name = cfg["C_NAME"].get<string>();
        auto res = conn->exec("insert into Course(C_ID, C_NAME, T_ID) values(" + c_id + ", \"" + c_name + "\", " + user_id + ")");
        send_json(m_sockfd, res);
        if(res["OK"].get<bool>())
        {
            LOG(INFO) << "注册课程成功";
        }
        else
        {
            LOG(WARNING) << "注册课程失败";
        }
        break;
    }
    default:
        break;
    }
}