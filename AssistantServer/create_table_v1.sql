# 用户表
create table User(
    U_ID bigint primary key,
    USERNAME varchar(20) unique,
    PASSWD varchar(20) not null,
    ROLE_TYPE varchar(10),
    SEX varchar(4),
    BIRTH date,
    DEPT varchar(30)
);

# 课程表
create table Course(
    C_ID bigint primary key,
    C_NAME varchar(50),
    T_ID bigint,
    FILE_PATH varchar(80),
    FOREIGN KEY (T_ID) REFERENCES User(U_ID)
);

# 选课表
create table SC(
    U_ID bigint,
    C_ID bigint,
    ABSENT_CNT int default 0,
    GRADE float,
    primary key(U_ID, C_ID),
    FOREIGN KEY (U_ID) REFERENCES User(U_ID),
    FOREIGN KEY (C_ID) REFERENCES Course(C_ID)
);
# 消息列表
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
) DEFAULT CHARSET=utf8;
# 邮件列表
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

# drop table EMAIL;
insert into User values(201630610533, 'yijiull', 'jiang0916', 'student', 'b', '1996-09-16', 'CS');
insert into User values(201630610168, 'Jiang', '123456', 'student', 'b', '1996-10-11', 'CS');
insert into User values(201630610666, 'LP', '123456', 'student', 'b', '1997-9-11', 'CS');
insert into User values(101634566666, "LLXU", "123456", "teacher", "g", "1985-7-20", "CS");
insert into User values(101634566555, "王家兵", "123456", "teacher", "b", "1975-7-20", "CS");
insert into Course values(1404196, 'PKI', 101634566666, 'courses/pki');
insert into Course values(1404198, '密码学', 101634566666, 'courses/密码学');
insert into Course values(1404197, '计算方法', 101634566555, 'courses/计算方法');
insert into SC(U_ID, C_ID) values(201630610533, 1404196);
insert into SC(U_ID, C_ID) values(201630610666, 1404196);
insert into Msg(M_AUTHOR, M_TYPE, M_ABOUT, M_CONTENT, M_TIME) values(201630610533, "TOPIC", 1404196, "about CA: how does CA work?", now());
insert into Msg(M_AUTHOR, M_TYPE, M_ABOUT, M_CONTENT, M_TIME) values(201630610533, "COMMENT", 1, "I don't know either =_=||", now());
insert into Msg(M_AUTHOR, M_TYPE, M_ABOUT, M_TO,  M_CONTENT, M_TIME) values(201630610533, "REPLY", 2, 201630610533, "不要水了...", now());
insert into Msg(M_AUTHOR, M_TYPE, M_ABOUT, M_TO,  M_CONTENT, M_TIME) values(201630610533, "REPLY", 2, 201630610533, "好的...", now());
insert into Msg(M_AUTHOR, M_TYPE, M_ABOUT, M_CONTENT, M_TIME) values(201630610533, "TOPIC", 1404196, "It's raining now =_=||", now());
insert into Msg(M_AUTHOR, M_TYPE, M_ABOUT, M_CONTENT, M_TIME) values(201630610533, "COMMENT", 5, "你也不要水了。。。", now());



#select * from User;
#update User set BIRTH = '1996-09-16' where USERNAME='yijiull';
#
#update Course set FILE_PATH='courses/pki' where C_ID='CS_PKI';
#update Course set TALK_PATH='talks/pki' where C_ID='CS_PKI';