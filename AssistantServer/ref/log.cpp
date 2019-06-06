#include <glog/logging.h>


int main()
{
    google::InitGoogleLogging("Assistant");
    FLAGS_colorlogtostderr = true;
    google::SetStderrLogging(google::GLOG_INFO);  //大于指定级别的错误都输出到标准输出
    google::SetLogDestination(google::GLOG_INFO,"../log/prefix_");
    //FLAGS_logtostderr =true; //标准输出
    LOG(INFO)<<"info" << 45 <<"sdf";
    LOG(WARNING) << "warning";
    LOG(ERROR) << "Error";
    LOG(FATAL);
    google::ShutdownGoogleLogging();
}