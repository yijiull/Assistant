# API

## FileManager
是用来管理上传、下载文件的类。
目前实现了多线程下载、单线程上传。TODO：加入一些提醒，比如覆盖已下载文件等...
断点续传还没有实现。

使用可以参考client.cpp。

`FileManager(int thread_number)`: 指定下载线程

`download(const string &file)`: 下载文件或目录
`upload(const string &file)`:  上传文件或目录
必须是绝对路径,目前还没有针对其他类型文件的处理，后续可以加上
普通文件： `/yjiull/temp/code.cpp`
目录： `/yijiull/temp`  注意最后不能加`/`



## Hash

mcrypto.h
mcrypto.cpp

Hash类是对openssl计算哈希部分的一个简单封装
每次计算都要重新初始化`init(const string &type)`, 参数可以选openssl实现的各种哈希函数，如`sha256`,`md5`等。
初始化后，使用`update(const string &msg)`可以分多次更新文件的哈希值，用于`read()`读取后分多次计算，不用一次存取文件内容。（内部如何实现的?）
对文件的全部内容计算完成之后，调用`hash()`即可返回加密后的哈希值。


## processpoll

参考书上实现了一个进程池，还没有用进去，打算用到服务端


## json

使用了解析json的一个包： [json](https://github.com/nlohmann/json#examples)

虽然效率低、耗内存，但好用！