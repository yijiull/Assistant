#ifndef FILE_MANAGER_H
#define FILE_MANAGER_H
#include "util.h"
class FileManager
{
private:
    json *config;
    int thread_number;

public:
    string username;
    //FileManager();
    FileManager(string _username, int n = 4); 
    ~FileManager();
    bool download(const string &file_path);
    bool upload(const string &file_path);
    void set_thread_number(int _n);
    int get_thread_number();
private:
    bool download_ing();
    static void do_download(int id, json *config);
    bool upload_ing(const string &file_path);
    void do_upload(int id);
    void distribute(json &files, int n); //将待处理的文件分到thread_number个线程中
};

#endif