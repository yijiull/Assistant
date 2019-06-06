#include "head.h"
//通用的哈希函数
class Hash 
{
private:
    EVP_MD_CTX *ctx;
public:
    Hash();
    Hash(const string &type);
    void init(const string &type);
    void update(const string &msg);
    string hash();
};