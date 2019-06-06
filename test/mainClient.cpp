#include "dll.h"

json __C;

int main(int argc, char* argv[])
{
    google::InitGoogleLogging(argv[0]);
    FLAGS_colorlogtostderr = true;
    google::SetStderrLogging(google::GLOG_INFO);  //大于指定级别的错误都输出到标准输出
    google::SetLogDestination(google::GLOG_INFO,"../log/FileServer_");
    __C = read_config();
    User user(__C["MAINSERVER"]["IP"].get<string>(), __C["MAINSERVER"]["PORT"].get<int>());
    while(true)
    {
        string type;
        cout<<"请输入命令：";
        cin>>type;
        if(type == "login")
        {
            string username, password;
            cout<<"请输入用户名：";
            cin>>username;
            cout<<"请输入密码：";
            cin>>password;
            auto res = user.c_login(username, password);
            if(res)
            {
                cout<<"登录成功!\n";
            }
            else
            {
                cout<<"用户名或密码错误!\n";
            }
        }
        else if(type == "register")
        {
            cout<<"请输入用户名：";
            cin>>user.username;
            cout<<"请输入用户id：";
            cin>>user.u_id;
            cout<<"请输入密码：";
            cin>>user.password;
            //两次密码验证
            cout<<"请输入性别[b/g]：";
            cin>>user.sex;
            cout<<"请输入出生年月[XXXX-XX-XX]:";
            cin>>user.birth;
            cout<<"请输入学院：";
            cin>>user.dept;
            cout<<"请输入用户类型[student/teacher]：";
            cin>>user.role_type;
            auto res = user.c_register();
            if(res)
            {
                cout<<"注册成功\n";
            }
            else
            {
                cout<<"注册失败\n用户名或ID已被注册！";
            }
        }
        else if(type == "update_info")
        {
            string password, sex, birth, dept;
            cout<<"请输入密码：";
            cin>>password;
            //两次密码验证
            cout<<"请输入性别[b/g]：";
            cin>>sex;
            cout<<"请输入出生年月[XXXX-XX-XX]:";
            cin>>birth;
            cout<<"请输入学院：";
            cin>>dept;
            auto res = user.c_update_info(password, sex, birth, dept);
            if(res)
            {
                cout<<"修改成功\n";
            }
            else
            {
                cout<<"修改失败\n";
            }
        }
        else if(type == "upload")
        {
            cout<<"请输入需要上传的文件路径: ";
            string path;
            cin>>path;
            auto res = user.upload(path);
            if(res)
            {
                cout<<"上传成功\n";
            }
            else
            {
                cout<<"上传失败\n";
            }

        }
        else if(type == "download")
        {
            cout<<"请输入需要下载的文件路径: ";
            string path;
            cin>>path;
            path = user.username + "/" + path;
            auto res = user.download(path);
            if(res)
            {
                cout<<"下载成功\n";
            }
            else
            {
                cout<<"下载失败\n";
            }
        }
        else if(type == "get_topic")
        {
            //参数：课程名
            cout<<"请输入课程id:";
            string course_id;
            cin>>course_id;
            auto res = user.c_get_topic(course_id);
            if(res["OK"].get<bool>())
            {
                cout<<"获取成功\n";
                cout<<res.dump(4)<<endl;
            }
            else
            {
                cout<<"获取失败\n";
            }
        }
        else if(type == "get_comment")
        {
            cout<<"请输入话题id:";
            string topic_id;
            cin>>topic_id;
            auto res = user.c_get_comment(topic_id);
            if(res["OK"].get<bool>())
            {
                cout<<"获取成功\n";
                cout<<res.dump(4)<<endl;
            }
            else
            {
                cout<<"获取失败\n";
            }
        }
        else if(type == "get_reply")
        {
            cout<<"请输入评论id：";
            string comment_id;
            cin>>comment_id;
            auto res = user.c_get_reply(comment_id);
            if(res["OK"].get<bool>())
            {
                cout<<"获取成功\n";
                cout<<res.dump(4)<<endl;
            }
            else
            {
                cout<<"获取失败\n";
            }
            
        }
        else if(type == "insert_topic")
        {
            cout<<"请输入要发起话题的课程id";
            string course_id;
            cin>>course_id;
            getchar();
            cout<<"请输入话题内容：";
            string content;
            getline(cin, content);
            auto res = user.c_insert_topic(course_id, content);
            if(res)
            {
                cout<<"插入成功\n";
            }
            else
            {
                cout<<"插入失败\n";
            }
        }
        else if(type == "insert_reply")
        {
            cout<<"请输入要回复的评论id：";
            string topic_id, to, content;
            cin>>topic_id;
            cout<<"请输入要回复的用户id";
            cin>>to;
            getchar();
            cout<<"请输入回复内容：";
            getline(cin, content);
            auto res = user.c_insert_reply(topic_id, to, content);
            if(res)
            {
                cout<<"评论成功\n";
            }
            else
            {
                cout<<"评论失败"<<endl;
            }
            

        }
        else if(type == "insert_comment")
        {
            cout<<"请输入要评论的话题id：";
            string topic_id, content;
            cin>>topic_id;
            getchar();
            cout<<"请输入回复内容：";
            getline(cin, content);
            auto res = user.c_insert_comment(topic_id, content);
            if(res)
            {
                cout<<"评论成功\n";
            }
            else
            {
                cout<<"评论失败"<<endl;
            }
            

        }
        else if(type == "send_email")
        {
            cout<<"请输入收件人用户名：";
            string to;
            cin>>to;
            getchar();
            cout<<"请输入邮件主题:";
            string topic;
            getline(cin, topic);
            cout<<"请输入邮件内容，以换行符结束:\n";
            string content;
            getline(cin, content);
            auto res = user.c_send_email(to, topic, content);
            if(res)
            {
                cout<<"邮件发送成功\n";
            }
            else
            {
                cout<<"邮件发送失败\n";
            }
        }
        else if(type == "get_email")
        {
            auto res = user.c_get_email();
            cout<<res.dump(4)<<endl;
        }
        else if(type == "get_draft_email")
        {
            auto res = user.c_get_draft_email();
            cout<<res.dump(4)<<endl;
        }
        else if(type == "get_unread_email")
        {
            auto res = user.c_get_unread_email();
            cout<<res.dump(4)<<endl;
        }
        else if(type == "del_email")
        {
            cout<<"请输入邮件id:";
            string e_id;
            cin>>e_id;
            auto res = user.c_del_email(e_id);
            if(res)
            {
                cout<<"邮件删除成功\n";
            }
            else
            {
                cout<<"邮件删除失败\n";
            }
        }
        else if(type == "save_draft_email")
        {
            cout<<"请输入收件人用户名：";
            string to;
            cin>>to;
            getchar();
            cout<<"请输入邮件主题:";
            string topic;
            getline(cin, topic);
            cout<<"请输入邮件内容，以换行符结束:\n";
            string content;
            getline(cin, content);
            auto res = user.c_save_draft_email(to, topic, content);
            if(res)
            {
                cout<<"草稿保存成功\n";
            }
            else
            {
                cout<<"草稿保存失败\n";
            }
        }
        else if(type == "get_student_list")
        {
            getchar();
            cout<<"请输入课程id:";
            string course_name;
            getline(cin, course_name);
            auto res = user.c_get_student_list(course_name);
            cout<<res.dump(4)<<endl;
        }
        else if(type == "get_course_list")
        {
            auto res = user.c_get_course_list();
            cout<<res.dump(4)<<endl;
        }
        else if(type == "get_course_files")
        {
            string c_id;
            cout<<"请输入课程id：";
            cin>>c_id;
            auto res = user.c_get_course_files(c_id);
            cout<<res.dump(4)<<endl;
        }
        else if(type == "get_course_notice")
        {
            string c_id;
            cout<<"请输入课程id：";
            cin>>c_id;
            auto res = user.c_get_course_notice(c_id);
            cout<<res.dump(4)<<endl;
        }
        else if(type == "publish_course_notice")
        {
            string c_id;
            cout<<"请输入课程id：";
            cin>>c_id;
            getchar();
            string content;
            cout<<"请输入通知内容：";
            getline(cin, content);
            auto res = user.c_publish_course_notice(c_id, content);
            if(res)
            {
                cout<<"发布成功\n";
            }
            else
            {
                cout<<"发布失败\n";
            }
        }
        else if(type == "update_absent")
        {
            string user_id;
            cout<<"请输入学生id：";
            cin>>user_id;
            int cnt;
            cout<<"请输入缺勤情况：";
            cin>>cnt;
            auto res = user.c_update_absent(user_id, cnt);
            if(res)
            {
                cout<<"记录缺勤成功\n";
            }
            else
            {
                cout<<"记录缺勤失败\n";
            }
        }
        else if(type == "register_course")
        {
            string c_id;
            cout<<"请输入课程id：";
            cin>>c_id;
            getchar();
            string c_name;
            cout<<"请输入课程名字: ";
            getline(cin, c_name);
            auto res = user.c_register_course(c_id, c_name);
            if(res)
            {
                cout<<"注册成功\n";
            }
            else
            {
                cout<<"注册失败\n";
            }
        }
        else if(type == "delete_file")
        {
            cout<<"TODO";
        }
    }
    google::ShutdownGoogleLogging();
}