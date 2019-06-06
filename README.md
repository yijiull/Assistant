[TOC]
# 说明

## 第三方库

使用了一个解析json的库：[json](https://github.com/nlohmann/json#examples)
虽然效率低、耗内存，但好用就够了。
关于怎么使用github上有例子，几分钟入门。

## 数据库

### User
用户表
```mysql
create table User(
    U_ID bigint primary key,
    USERNAME varchar(20) unique,
    PASSWD varchar(20) not null,
    ROLE_TYPE varchar(10),
    SEX varchar(4),
    BIRTH date,
    DEPT varchar(30)
);
```
`U_ID`:用户id  
`USERNAME`:用户名, 唯一  
`PASSWD`：密码  
`ROLE_TYPE`：用户类型(`student`, `teacher`, ...)  
`SEX`:性别    
`BIRTH`：生日  
`DEPT`：系（学院）  

### Course
课程表

```mysql
create table Course(
    C_ID bigint primary key,
    C_NAME varchar(50),
    T_ID bigint,
    FILE_PATH varchar(80),
    FOREIGN KEY (T_ID) REFERENCES User(U_ID)
);
```
`C_ID`:课程id  
`C_NAME`:课程名  
`U_ID`: 授课教师id, 外键（User.U_ID）  
`FILE_PATH`:课程资源路径  

### SC
学生选课表
```mysql
create table SC(
    U_ID bigint,
    C_ID bigint,
    ABSENT_CNT int default 0,
    GRADE float,
    primary key(U_ID, C_ID),
    FOREIGN KEY (U_ID) REFERENCES User(U_ID),
    FOREIGN KEY (C_ID) REFERENCES Course(C_ID)
);
```
`U_ID`:学生id,外键（`User.U_ID`)  
`C_ID`:课程id,外键(`Course.C_ID`)  
`ABSENT`:缺勤次数  
`GRADE`:总成绩  

### Msg
消息表：包含讨论区、作业、通知等
```mysql
create table Msg(
    M_ID bigint AUTO_INCREMENT,
    M_AUTHOR bigint not null,
    M_TYPE varchar(10) not null,
    M_ABOUT bigint,
    M_TO bigint,
    M_CONTENT text,
    M_TIME datetime,
    PRIMARY KEY(M_ID),
    FOREIGN key (M_AUTHOR) REFERENCES User(U_ID),
    FOREIGN key (M_TO) REFERENCES User(U_ID)
);
```
`M_ID`:自增id  
`M_AUTHOR`:消息的作者，外键（`User.U_ID`）  
`M_TYPE`: 消息类型（`TOPIC`、`COMMENT`、`REPLY`、`HOMEWORK`、...）  
`M_ABOUT`: 关于,这个字段的意义取决于`M_TYPE`  
- `TOPIC`对应课程ID（`Course.C_ID`） 
- `COMMENT`对应`TOPIC`的id  
- `REPLY`对应`COMMENT`的id  

`M_TO`: 只对`M_TYPE`是`REPLY`的数据有效，表示回复给谁  
`M_CONTENT`: 回复内容  
`M_TIME`:消息时间  

### EMAIL

```mysql
create table EMAIL(
    E_ID bigint AUTO_INCREMENT,
    E_FROM bigint not null,
    E_TO bigint,
    E_TOPIC TINYTEXT,
    E_CONTENT text,
    E_TIME datetime not null,
    E_TYPE TINYINT not null,  
    PRIMARY KEY(E_ID),
    FOREIGN KEY(E_FROM) REFERENCES User(U_ID),
    FOREIGN KEY(E_TO) REFERENCES User(U_ID)
)
```

- `E_ID`:邮件:id:
- `E_FROM`:发件人，外键(`User.U_ID`)
- `E_TO`:收件人，外键(`User.U_ID`)

- `E_TIME`:发信时间
- `E_TYPE`:信件类型（`0`：未读， `1`：已读， `2`：草稿箱，`3`：已发送）



## 文件服务器

所有用户文件都放在`AssistantServer/home`目录下
- 学生用户拥有自己的主文件夹（用`用户名`命名）
- 教师用户拥有自己的主文件夹（用`用户名`命名）
- 每门课的主文件加放到授课教师的目录下（用`课程名`命名）

客户端下载文件或者上传文件时，输入的路径为相对`home`目录的路径。
例：
```
如果用户`yijiull`要下载`home/yijiull/diary.txt`这个文件
则传入download的参数为`diary.txt`或者`./diary.txt`

下载同样传参。
```


## 主服务器

主服务其连接数据库，根据客户端发来的请求进行增删查改
进程池默认开启了八个进程，每个进程可以处理多个用户。

目前之实现了注册和登录的功能，可以参考这两个功能实现其他功能，其实主要就是数据库的增删查改。

关于讨论区：
- ~~讨论内容不打算存到数据库，每门课建立一个json 文件存放讨论区内容，数据库只存放json的路径即可（因为实现简单，性能先不考虑）~~
- 改为存到数据库里=_=||

### 已实现功能
1. - [x] 登录
2. - [x] 注册
3. - [x] 获取topic
4. - [x] 插入topic
5. - [x] 获取comment
6. - [x] 插入comment
7. - [x] 获取reply
8. - [x] 插入reply
9. - [ ] 获取课程资源列表
10. - [ ] 下载指定资源
11. - [ ] 上传课件（教师用户）
12. - [ ] 分享文件
13. - [ ] 删除指定文件
14. - [ ] 获取通知
15. - [ ] 发通知（教师用户）
16. - [x] 发邮件
17. - [x] 获取所有已收到邮件
18. - [x] 保存邮件到草稿箱 
19. - [x] 获取草稿箱
20. - [x] 获取未读邮件
21. - [x] 删除邮件
22. - [x] 修改个人信息
23. - [x] 获取学生列表（教师用户）
24. - [ ] 记录考勤（教师用户）
25. - [ ] 记录成绩（教师用户）
26. - [ ] 注册课程（教师用户）
27. - [x] 获取学生课程列表

## 客户端

客户端需要先连接主服务器，进行登录，如果登录成功就可以使用自己的资源（文件服务器不需要再次登录，直接连接即可）

下载文件过程：
- 先发送需要下载的文件进行查询，获取文件大小信息（是否存在）
- 根据服务器返回的信息，开始下载（文件存在）/报错（没有该文件）


用户下载的文件默认放到`Downloads`目录下，如果没有会自动创建该目录，下载的文件保持目录结构。

下载可以指定线程数进行多线程下载，每个线程会于文件服务器直接建立一个连接。
下载之后会自动计算哈希值与原文件进行比较，如果不一致说明传输出现了问题，报错。
断点续传还没有实现。

# 编译
## 后台
进入`Assist/AssistantServer/`目录：
```bash
mkdir build
cd build
cmake ..
make -j4
```
这时`build`目录下就生成了两个可执行文件，`FileServer`和`mainServer`,在启动客户端之前启动这两个程序就行


## 客户端

进入`Assist/AssistantClient/`目录：
```bash
mkdir build
cd build
cmake ..
make -j4
```
这时`build`目录下就生成了可执行文件，`mainClient`,运行即可




# 服务器配置
## 主服务器
- IP: 127.0.0.1
- PORT: 9999

## 文件服务器：
- IP: 127.0.0.1
- PORT: 9998

## 数据库
- IP: 
- PORT:
- username:
- password: 
- database: 
	- User
	- Course
	- SC



