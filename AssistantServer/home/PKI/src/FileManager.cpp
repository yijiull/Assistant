#include "FileManager.h"
#include "head.h"
#include "json.hpp"
#include "util.h"



std::mutex m; //TODO

/*
FileManager::FileManager()
{
    cout<<"dont want\n";
    thread_number = 4;
    config = new json;

}
*/
FileManager::FileManager(string _username, int n)
{
    thread_number = n;
    username = _username;
    config = nullptr;
}
FileManager::~FileManager()
{
    delete config;
}
int FileManager::get_thread_number()
{
    return thread_number;
}
void FileManager::set_thread_number(int _n)
{
    thread_number = _n;
}

bool FileManager::upload(const string &file_path) //单线程上传文件
{
    json files;
    files_to_json(file_path, files);
    distribute(files, 1);
    //cout<<(*config).dump(4)<<endl;
    LOG(INFO) << (*config).dump(4);
    return upload_ing(file_path); //??
}

//向服务端请求待下载的文件信息
bool FileManager::download(const string &file_path)
{
    //if(file_path[file_path.length() - 1] == '/')  //去掉最后的`/`
    //{
    //    file_path = file_path.substr(0, file_path.length() - 1);
    //}
    //std::chrono::high_resolution_clock::time_point ;
    auto st = std::chrono::high_resolution_clock::now();
    string download_dir = "../Downloads";
    int res = -1;
    res = mkdir(download_dir.c_str(), 0755);  //已存在就是返回-1, errno = EEXIST
    
    size_t pos = file_path.find_last_of('/');
    if(pos == string::npos)
    {   
        cout<<"下载路径存在问题，没有找到/\n";
        return 0;
    }
    string file_name = file_path.substr(pos + 1);
    //先找暂存文件`.temp_filename.config`;
    string config_file = download_dir + "/.temp_" + file_name + ".json";
    if(access(config_file.c_str(), F_OK) == 0)
    {
        printf("断点续传\n");
        //断点续传
        config = read_config(config_file.c_str());
        //TODO
        /*
            **暂时默认本地已下载的文件与服务端文件一致**
            可以先去服务端根据config读取数据，计算哈希，再返回与本地哈希值进行对比
            如果一致再进行断点续传，否则重传
        */
        auto cfg = *config;
        thread_number = cfg["info"].size();
        // 创建线程开始断点下载
        download_ing();
    }
    else //从头开始下载
    {
        int fd = unblock_connect(__C["FILESERVER"]["IP"].get<string>().c_str(), __C["FILESERVER"]["PORT"].get<int>(), 10); //连接到服务器
        json msg;
        msg["type"] = QUERY_FILE_OR_DIR;
        msg["file"] = file_path; 
        auto res = send_json(fd, msg);
        assert(res > 0);

        string data_received;
        char buf[__C["BUF_SIZE"].get<int>()];
        //TODO:这里要求对方发完之后关闭socket，这边才能收到EOF，不然会阻塞到这里，后续可以改用epoll
        while(true) //获取文件列表，即config  
        {
            int _n = read(fd, buf, __C["BUF_SIZE"].get<int>());
            if(_n <= 0)
            {
                if(errno == EINTR) //会自动重启，不会运行到这里
                {
                    cout<<"### EINTR ###\n";
                    continue;
                }
                else
                {
                    break;
                }
                
            }
            data_received += string(buf, buf + _n);
        }

        //TODO: 服务器返回错误，没有该文件

        //cout<<data_reveived<<endl;
        /*
        json *files = new json;
        std::stringstream ss(data_received);

        ss >> *files;
        std::cout<<(*files).dump(4)<<std::endl;
        distribute(*files, thread_number);
        delete files;
        */
        json files;
        std::stringstream ss(data_received);

        ss >> files;
        if(files.size() == 0)
        {
            return false;
        }
        std::cout<<files.dump(4)<<std::endl;
        distribute(files, thread_number);
        close(fd);
        res = download_ing();
        //cout<<(*config).dump(4)<<endl;    
        auto ed = std::chrono::high_resolution_clock::now();
        auto time_span = std::chrono::duration_cast<std::chrono::duration<double>>(ed-st);
        cout<<"Time: "<<time_span.count()<<" seconds\n";
        return res;
    }
}
void FileManager::distribute(json &files, int thread_n)
{
    /*
    //json文件格式
        [
            {
                "file-name": user/desire/temp.txt,
                "file-length": size_t,
                "cur-pos": 23
            },
            {
                "file-name": user/desire/yijiiull/temp.txt,
                "file-length": size_t
            }
        ]

    */
    int n = files.size();
    size_t total = 0;
    for(int i = 0; i < n; i++)
    {
        //total += files[i]["file-length"].get<size_t>();
        total += files[i]["file-length"].get<size_t>() - files[i]["cur-pos"].get<size_t>() ;
    }
    cout<<"file cnt: "<<n<<"\ntotal bytes: "<<total<<endl;
    int segment = total / thread_n;  //每个线程需要下载的长度
    //cout<<"total:"<<total<<"  segment:"<<segment<<endl;
    
    //分配任务
    if(config)
    {
        delete config;
    }
    config = new json;
    json *cfg = config;
    int cur = 0;
    for(int i = 0; i < thread_n; i++)
    {
        cout<<"distribute thread "<<i+1<<endl;
        int temp = segment;
        if(i == thread_n - 1)
        {
            temp = total - segment * (thread_n - 1);
        }
        int thread_id = 0;
        while(temp)
        {
            //cout<<files.dump(4)<<endl;
            int toread = files[cur]["file-length"].get<size_t>() - files[cur]["cur-pos"].get<size_t>();
            //cout<<toread<<endl; 
            if(toread <= temp)
            {
                temp -= toread;
                auto res =  files[cur]["file-name"].get<string>();
                cout<<res<<endl;
                (*cfg)["info"][i][thread_id]["file-name"] = files[cur]["file-name"].get<string>();
                (*cfg)["info"][i][thread_id]["st"] = files[cur]["cur-pos"].get<size_t>();
                (*cfg)["info"][i][thread_id]["file-length"] = toread;
                (*cfg)["info"][i][thread_id]["cur-pos"] = 0;
                files[cur]["cur-pos"] = files[cur]["file-length"].get<size_t>();
                cur++;
            }
            else
            {
                (*cfg)["info"][i][thread_id]["file-name"] = files[cur]["file-name"].get<string>();
                (*cfg)["info"][i][thread_id]["st"] = files[cur]["cur-pos"].get<size_t>();
                (*cfg)["info"][i][thread_id]["file-length"] = temp;
                (*cfg)["info"][i][thread_id]["cur-pos"] = 0;
                files[cur]["cur-pos"] = files[cur]["cur-pos"].get<size_t>() + temp;
                temp = 0;
            }
            thread_id++;
        }
    }
    (*cfg)["sha-256"] = "0";
    (*cfg)["total"] = total;
    (*cfg)["download"] = 0;
    //return cfg;
}

void FileManager::do_download(int id, json *config)
{
    cout<<id<<" start\n";
    auto cfg = *config;
    json tasks = cfg["info"][id]; 
    int task_number = tasks.size(); //为了分解任务,每个线程可能传输多个文件的部分片段
    cout<<"task_number: "<<task_number<<endl;
    int sockfd = unblock_connect(__C["FILESERVER"]["IP"].get<string>().c_str(), __C["FILESERVER"]["PORT"].get<int>(), 10); //连接到服务器
    char buf[__C["BUF_SIZE"].get<int>()];
    for(int i = 0; i < task_number; i++)
    {
        json task = tasks[i];
        // cout<<task.dump(4)<<endl;
        task["type"] = DOWNLOAD_FILE;
        send_json(sockfd, task);
        //cout<<"send task done\n";
        string save_file = task["file-name"].get<string>();
        save_file = save_file.substr(save_file.find_first_of('/', 8) + 1); 
        string file = "./Downloads/" + save_file;
        __off64_t file_off = task["st"].get<long>() + task["cur-pos"].get<long>(); // 文件偏移
        size_t length_to_download = task["file-length"].get<size_t>(); //需要下载的文件长度
        int savefd = my_open(file); 
        cout<<"save to file: "<<file<<endl;
        //cout<<"file: "<<savefd<<endl;

        int res_off = lseek(savefd, file_off, SEEK_SET);
        assert(res_off == file_off);

        string local_hash = socket_to_file_client(sockfd, savefd, length_to_download, &task); //BUG?
        int hash_len = rio_readn(sockfd, buf, 32);
        assert(hash_len == 32);
        close(savefd);
        cout<<"hash: "<<local_hash<<"  from " << file_off << " to "<<file_off + length_to_download <<"  "<< file<<endl;
        if(string(buf, buf + hash_len) != local_hash)
        {
            cout<<task[i].dump(4)<<endl;
            cout<<"传输出现错误，请重新传输本段！[y\\n]:";
            char c;
            cin>>c;
            if(c == 'y')
            {
                continue;
            }
        }
        else
        {
            //更新下载信息
            (*config)["info"][id][i] = task;
            //TODO：这里线程共享了，需要加锁，不过感觉不值得为了一个下载进度就加把锁
            //m.lock();
            //(*config)["download"] = (*config)["download"].get<size_t>() + res;  //fixme BUG
            //m.unlock();
        }
    }
    //TODO: send msg to tell server close;  不需要了，服务端直接检测客户端关闭socket后也关闭即可
    close(sockfd);
    // cout<<"read： "<<tot<<" bytes"<<endl;
    std::cout<<id<<" done\n";
}

bool FileManager::download_ing()
{
    cout<<"ing...\n";
    std::thread *threads[thread_number];
    for(int i = 0; i < thread_number; i++)
    {
        threads[i] = new std::thread(do_download, i, config);
    }
    for(int i = 0; i < thread_number; i++)
    {
        threads[i]->join();
    }
    cout<<"download done!\n";
    for(int i = 0; i < thread_number; i++){
        delete threads[i];
    }
    return true;
}
bool FileManager::upload_ing(const string &file_path)
{
    int fd = unblock_connect(__C["FILESERVER"]["IP"].get<string>().c_str(), __C["FILESERVER"]["PORT"].get<int>(), 10);
    auto msg = *config;
    msg["type"] = UPLOAD_FILE;
    msg["username"] = username;
    msg["home_dir"] = file_path;
    send_json(fd, msg);

    char buf[1];
    read(fd, buf, 1);

    json tasks = msg["info"][0]; 
    int task_number = tasks.size(); //为了分解任务,每个线程可能传输多个文件的部分片段
    cout<<"task_number: "<<task_number<<endl;
    for(int i = 0; i < task_number; i++)
    {
        file_to_socket(tasks[i], fd);
    }
    close(fd);
    return true;
}
