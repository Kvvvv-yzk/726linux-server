#pragma once // 防止头文件被多次包含

#include "Socket.h"          // 自定义Socket相关头文件（可能包含Buffer定义）
#include "http_parser.h"     // 引入http_parser库头文件
#include <map>               // 使用std::map容器

// HTTP 解析类
class CHttpParser
{
private:
	http_parser m_parser;                    // HTTP解析器实例
	http_parser_settings m_settings;         // HTTP解析器设置（回调函数绑定）
	std::map<Buffer, Buffer> m_HeaderValues; // 保存解析出的HTTP头部字段及其值
	Buffer m_status;                         // 状态行（如HTTP/1.1 200 OK）
	Buffer m_url;                            // 请求URL
	Buffer m_body;                           // 请求体内容
	bool m_complete;                         // 是否解析完成标志
	Buffer m_lastField;                      // 临时保存当前正在解析的字段名
public:
	CHttpParser();                           // 构造函数
	~CHttpParser();                          // 析构函数
	CHttpParser(const CHttpParser& http);    // 拷贝构造函数
	CHttpParser& operator=(const CHttpParser& http); // 赋值操作符重载

public:
	size_t Parser(const Buffer& data);       // 解析输入的数据（完整或部分HTTP请求）

	// 获取HTTP方法编号，参考http_parser.h中的HTTP_METHOD_MAP宏
	unsigned Method() const { return m_parser.method; }

	const std::map<Buffer, Buffer>& Headers() { return m_HeaderValues; } // 获取HTTP头部map
	const Buffer& Status() const { return m_status; }                    // 获取状态行
	const Buffer& Url() const { return m_url; }                          // 获取请求URL
	const Buffer& Body() const { return m_body; }                        // 获取请求体
	unsigned Errno() const { return m_parser.http_errno; }              // 获取解析错误码

protected:
	// 以下为libhttp_parser要求的静态回调函数声明
	static int OnMessageBegin(http_parser* parser);                      // 消息开始
	static int OnUrl(http_parser* parser, const char* at, size_t length); // 解析URL
	static int OnStatus(http_parser* parser, const char* at, size_t length); // 解析状态行
	static int OnHeaderField(http_parser* parser, const char* at, size_t length); // 解析头部字段名
	static int OnHeaderValue(http_parser* parser, const char* at, size_t length); // 解析头部字段值
	static int OnHeadersComplete(http_parser* parser);                  // 头部解析完成
	static int OnBody(http_parser* parser, const char* at, size_t length); // 解析Body
	static int OnMessageComplete(http_parser* parser);                  // 消息解析完成

	// 以下为对应的成员函数实现，供静态函数中回调使用
	int OnMessageBegin();
	int OnUrl(const char* at, size_t length);
	int OnStatus(const char* at, size_t length);
	int OnHeaderField(const char* at, size_t length);
	int OnHeaderValue(const char* at, size_t length);
	int OnHeadersComplete();
	int OnBody(const char* at, size_t length);
	int OnMessageComplete();
};

// URL解析类（解析协议、主机、端口、查询参数等）
class UrlParser
{
public:
	UrlParser(const Buffer& url); // 构造函数，传入待解析的URL
	~UrlParser() {}

	int Parser();                             // 解析URL
	Buffer operator[](const Buffer& name)const; // 获取查询参数中指定字段值

	Buffer Protocol() const { return m_protocol; } // 获取协议（如http/https）
	Buffer Host() const { return m_host; }         // 获取主机名或IP地址
	int Port() const { return m_port; }            // 获取端口号，默认返回80
	void SetUrl(const Buffer& url);                // 设置新的URL并准备重新解析

private:
	Buffer m_url;           // 原始URL
	Buffer m_protocol;      // 协议部分
	Buffer m_host;          // 主机部分
	Buffer m_uri;           // URI路径部分
	int m_port;             // 端口号
	std::map<Buffer, Buffer> m_values; // 存储URL中的查询参数(key-value)
};
