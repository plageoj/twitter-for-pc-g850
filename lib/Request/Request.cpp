#include "Request.h"
#include <algorithm>

#include "UrlEncode.h"

void Parameter::add(String key, String value)
{
    KVS prm;
    prm.key = urlEncode(key);
    prm.value = urlEncode(value);

    this->param.push_back(prm);
}

void Parameter::remove(String key)
{
    for (auto iter = this->param.begin(); iter != this->param.end(); iter++)
    {
        if (iter->key == key)
        {
            this->param.erase(iter);
            break;
        }
    }
}

String Parameter::get(String key_glue, String value_glue)
{
    String ret = "";
    for (auto &iter : this->param)
    {
        ret += key_glue + iter.key + value_glue + iter.value;
    }
    ret.remove(0, key_glue.length());
    return ret;
}

void Parameter::concat(Parameter param)
{
    this->param.insert(this->param.end(), param.param.begin(), param.param.end());
}

void Parameter::sort()
{
    std::sort(this->param.begin(), this->param.end());
}

Request::Request()
{
}

String Request::getRequestString()
{
    String ret = this->method + " " + this->path;
    String paramStr = this->queryParam.get();
    if (!paramStr.isEmpty())
    {
        ret += "?" + paramStr;
    }
    ret += " HTTP/1.1\r\n";
    return ret;
}

String Request::getParameterString()
{
    Parameter temp;
    temp.concat(this->authParam);
    temp.concat(this->queryParam);
    temp.concat(this->bodyParam);
    temp.sort();
    return temp.get();
}

String Request::getUrl(String protcol)
{
    return protcol + "://" + this->host + this->path;
}