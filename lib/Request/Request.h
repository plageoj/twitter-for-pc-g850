#ifndef REQUEST_H_INCLUDED
#define REQUEST_H_INCLUDED

#include <Arduino.h>
#include <vector>

class Parameter
{
public:
    void add(String, String);
    void remove(String);
    String get(String, String);
    String get()
    {
        return get("&", "=");
    };

    void concat(Parameter);
    void sort();

private:
    struct KVS
    {
        String key;
        String value;
    };
    std::vector<KVS> param;
    friend bool operator<(KVS &left, KVS &right)
    {
        return left.key < right.key;
    }
};

class Request
{
public:
    Request();
    Request(String method)
    {
        this->method = method;
    };
    Request(String method, String host)
    {
        this->method = method;
        this->host = host;
    };
    Request(String method, String host, String path)
    {
        this->method = method;
        this->host = host;
        this->path = path;
    };

    String getRequestString();
    String getParameterString();
    String getUrl(String);

    String host;
    String method;
    String path;
    Parameter queryParam;
    Parameter bodyParam;
    Parameter authParam;

private:
};

#endif