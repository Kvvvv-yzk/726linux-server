#include "HttpParser.h"

// 构造函数：初始化 http_parser 和回调设置
CHttpParser::CHttpParser()
{
	m_complete = false;  // 标志是否解析完成
	memset(&m_parser, 0, sizeof(m_parser));  // 清空 parser 结构体
	m_parser.data = this;  // 回调函数中可获取当前对象
	http_parser_init(&m_parser, HTTP_REQUEST);  // 初始化为 HTTP 请求模式

	// 设置回调函数指针
	memset(&m_settings, 0, sizeof(m_settings));
	m_settings.on_message_begin = &CHttpParser::OnMessageBegin;
	m_settings.on_url = &CHttpParser::OnUrl;
	m_settings.on_status = &CHttpParser::OnStatus;
	m_settings.on_header_field = &CHttpParser::OnHeaderField;
	m_settings.on_header_value = &CHttpParser::OnHeaderValue;
	m_settings.on_headers_complete = &CHttpParser::OnHeadersComplete;
	m_settings.on_body = &CHttpParser::OnBody;
	m_settings.on_message_complete = &CHttpParser::OnMessageComplete;
}

// 析构函数：无资源释放
CHttpParser::~CHttpParser() {}

// 拷贝构造函数
CHttpParser::CHttpParser(const CHttpParser& http)
{
	memcpy(&m_parser, &http.m_parser, sizeof(m_parser));
	m_parser.data = this;
	memcpy(&m_settings, &http.m_settings, sizeof(m_settings));
	m_status = http.m_status;
	m_url = http.m_url;
	m_body = http.m_body;
	m_complete = http.m_complete;
	m_lastField = http.m_lastField;
}

// 赋值操作符重载
CHttpParser& CHttpParser::operator=(const CHttpParser& http)
{
	if (this != &http) {
		memcpy(&m_parser, &http.m_parser, sizeof(m_parser));
		m_parser.data = this;
		memcpy(&m_settings, &http.m_settings, sizeof(m_settings));
		m_status = http.m_status;
		m_url = http.m_url;
		m_body = http.m_body;
		m_complete = http.m_complete;
		m_lastField = http.m_lastField;
	}
	return *this;
}

// 核心解析函数：调用 http_parser_execute
size_t CHttpParser::Parser(const Buffer& data)
{
	m_complete = false;
	size_t ret = http_parser_execute(
		&m_parser, &m_settings, data, data.size());  // 执行解析器
	if (m_complete == false) {
		m_parser.http_errno = 0x7F;  // 自定义错误码：未完整解析
		return 0;
	}
	return ret;
}

// 所有静态回调都将转发到对象实例方法
int CHttpParser::OnMessageBegin(http_parser* parser)
{ return ((CHttpParser*)parser->data)->OnMessageBegin(); }

int CHttpParser::OnUrl(http_parser* parser, const char* at, size_t length)
{ return ((CHttpParser*)parser->data)->OnUrl(at, length); }

int CHttpParser::OnStatus(http_parser* parser, const char* at, size_t length)
{ return ((CHttpParser*)parser->data)->OnStatus(at, length); }

int CHttpParser::OnHeaderField(http_parser* parser, const char* at, size_t length)
{ return ((CHttpParser*)parser->data)->OnHeaderField(at, length); }

int CHttpParser::OnHeaderValue(http_parser* parser, const char* at, size_t length)
{ return ((CHttpParser*)parser->data)->OnHeaderValue(at, length); }

int CHttpParser::OnHeadersComplete(http_parser* parser)
{ return ((CHttpParser*)parser->data)->OnHeadersComplete(); }

int CHttpParser::OnBody(http_parser* parser, const char* at, size_t length)
{ return ((CHttpParser*)parser->data)->OnBody(at, length); }

int CHttpParser::OnMessageComplete(http_parser* parser)
{ return ((CHttpParser*)parser->data)->OnMessageComplete(); }

// 以下是成员版本的回调实现：
int CHttpParser::OnMessageBegin()
{ return 0; }

int CHttpParser::OnUrl(const char* at, size_t length)
{
	m_url = Buffer(at, length);  // 记录URL
	return 0;
}

int CHttpParser::OnStatus(const char* at, size_t length)
{
	m_status = Buffer(at, length);  // 记录状态行
	return 0;
}

int CHttpParser::OnHeaderField(const char* at, size_t length)
{
	m_lastField = Buffer(at, length);  // 暂存字段名，等待匹配值
	return 0;
}

int CHttpParser::OnHeaderValue(const char* at, size_t length)
{
	m_HeaderValues[m_lastField] = Buffer(at, length);  // 绑定字段值
	return 0;
}

int CHttpParser::OnHeadersComplete()
{ return 0; }

int CHttpParser::OnBody(const char* at, size_t length)
{
	m_body = Buffer(at, length);  // 记录请求体
	return 0;
}

int CHttpParser::OnMessageComplete()
{
	m_complete = true;  // 标志解析成功
	return 0;
}

// 构造函数：保存待解析的URL
UrlParser::UrlParser(const Buffer& url)
{
	m_url = url;
}

// 核心解析函数：将URL解析为协议、主机、端口、路径、查询参数
int UrlParser::Parser()
{
	// 步骤1：解析协议
	const char* pos = m_url;
	const char* target = strstr(pos, "://");
	if (target == NULL) return -1;
	m_protocol = Buffer(pos, target);  // 提取 "http"、"https"

	// 步骤2：解析主机+端口
	pos = target + 3;
	target = strchr(pos, '/');
	if (target == NULL) {
		// 说明没有 URI，仅主机
		if (m_protocol.size() + 3 >= m_url.size())
			return -2;
		m_host = pos;
		return 0;
	}

	Buffer value = Buffer(pos, target);  // 主机 + 端口部分
	if (value.size() == 0) return -3;

	target = strchr(value, ':');  // 查找是否带端口
	if (target != NULL) {
		m_host = Buffer(value, target);  // 提取主机
		m_port = atoi(Buffer(target + 1, (char*)value + value.size()));  // 提取端口号
	}
	else {
		m_host = value;  // 默认无端口，使用80
	}

	// 步骤3：解析 URI 和 查询参数
	pos = strchr(pos, '/');
	target = strchr(pos, '?');
	if (target == NULL) {
		m_uri = pos;  // 没有参数，整个为 URI
		return 0;
	} else {
		m_uri = Buffer(pos, target);  // 分离 URI
		pos = target + 1;  // 定位到参数部分
		const char* t = NULL;

		do {
			target = strchr(pos, '&');  // 查找下一个参数
			if (target == NULL) {
				// 最后一个参数
				t = strchr(pos, '=');
				if (t == NULL) return -4;
				m_values[Buffer(pos, t)] = Buffer(t + 1);  // key=value
			}
			else {
				Buffer kv(pos, target);  // 提取键值对字符串
				t = strchr(kv, '=');
				if (t == NULL) return -5;
				m_values[Buffer(kv, t)] = Buffer(t + 1, kv + kv.size());  // 分隔 key 和 value
				pos = target + 1;  // 下一个参数
			}
		} while (target != NULL);
	}

	return 0;
}

// 获取解析结果中指定key的值
Buffer UrlParser::operator[](const Buffer& name) const
{
	auto it = m_values.find(name);
	if (it == m_values.end()) return Buffer();  // 找不到返回空
	return it->second;
}

// 设置新的URL，并清除旧状态
void UrlParser::SetUrl(const Buffer& url)
{
	m_url = url;
	m_protocol = "";
	m_host = "";
	m_port = 80;
	m_values.clear();
}
