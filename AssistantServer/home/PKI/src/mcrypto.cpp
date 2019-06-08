#include "mcrypto.h"

Hash::Hash()
{
}
Hash::Hash(const string &type)
{
    init(type);
}
void Hash::init(const string &type)
{
    const EVP_MD *md = EVP_get_digestbyname(type.c_str());
    if(md == NULL)
    {
        cout<<"Unknown message digest "<<type<<endl;
        cout<<"eg: \"sha256\", \"md5\", etc...\n";
        exit(1);
    }
    ctx = EVP_MD_CTX_new();
    EVP_DigestInit_ex(ctx, md, NULL);

}
void Hash::update(const string &msg)
{
    EVP_DigestUpdate(ctx, msg.c_str(), msg.size());
}
string Hash::hash()
{
    unsigned char md_value[EVP_MAX_MD_SIZE];
    unsigned int md_len;
    EVP_DigestFinal_ex(ctx, md_value, &md_len);
    EVP_MD_CTX_free(ctx);
    std::stringstream ss;
    for(int i = 0; i < md_len; i++)
    {
        ss << std::hex << std::setw(2) << std::setfill('0') << (int)md_value[i];
    }
    return ss.str();
}