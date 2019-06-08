#ifndef HEAD_H
#define HEAD_H
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/stat.h>
#include <string.h>
#include <fcntl.h>
#include <time.h>
#include <assert.h>

#include <sys/epoll.h>
#include <signal.h>
#include <sys/wait.h>

#include <stdio.h>

#include <sys/sendfile.h>

//dir
#include <dirent.h>
#include <sys/stat.h>

#include <iostream>
#include <string>
#include <vector>
#include <memory>
#include <mutex>
#include <iomanip>
using std::string;
using std::vector;
using std::cin;
using std::cout;
using std::endl;

#include <openssl/sha.h>
#include <openssl/evp.h>

#include "json.hpp"
using json = nlohmann::json;

#include <fstream> //读取json
#include <sstream>
#include <thread>
extern json __C;


//计时
#include <ctime>
#include <chrono>
#include <ratio>


#include <mysql/mysql.h>

#include <glog/logging.h>

enum MSG_TYPE{
    close_socket,
    
    LOGIN,
    REGISTER,
    UPDATE_INFO,

    GET_COURSE_LIST,
    GET_COURSE_FILES,
    GET_COURSE_NOTICE,

    //文件系统
    QUERY_FILE_OR_DIR,
    DOWNLOAD_FILE,
    UPLOAD_FILE,
    DELETE_FILE,

    //讨论区
    GET_TOPIC,
    INSERT_TOPIC,
    GET_COMMENT,
    INSERT_COMMENT,
    GET_REPLY,
    INSERT_REPLY,
    //邮件系统
    SEND_EMAIL,
    GET_EMAIL,
    GET_DRAFT_EMAIL,
    GET_UNREAD_EMAIL,
    DEL_EMAIL,
    SAVE_DRAFT_EMAIL,
    MARK_EMAIL,
    //教师特权
    GET_STUDENT_LIST,
    PUBLISH_COURSE_NOTICE,
    UPDATE_ABSENT,
    UPDATE_GRADE,
    REGISTER_COURSE




};

#endif