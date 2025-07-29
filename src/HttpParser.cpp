#include "HttpParser.h"

// ���캯������ʼ�� http_parser �ͻص�����
CHttpParser::CHttpParser()
{
	m_complete = false;  // ��־�Ƿ�������
	memset(&m_parser, 0, sizeof(m_parser));  // ��� parser �ṹ��
	m_parser.data = this;  // �ص������пɻ�ȡ��ǰ����
	http_parser_init(&m_parser, HTTP_REQUEST);  // ��ʼ��Ϊ HTTP ����ģʽ

	// ���ûص�����ָ��
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

// ��������������Դ�ͷ�
CHttpParser::~CHttpParser() {}

// �������캯��
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

// ��ֵ����������
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

// ���Ľ������������� http_parser_execute
size_t CHttpParser::Parser(const Buffer& data)
{
	m_complete = false;
	size_t ret = http_parser_execute(
		&m_parser, &m_settings, data, data.size());  // ִ�н�����
	if (m_complete == false) {
		m_parser.http_errno = 0x7F;  // �Զ�������룺δ��������
		return 0;
	}
	return ret;
}

// ���о�̬�ص�����ת��������ʵ������
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

// �����ǳ�Ա�汾�Ļص�ʵ�֣�
int CHttpParser::OnMessageBegin()
{ return 0; }

int CHttpParser::OnUrl(const char* at, size_t length)
{
	m_url = Buffer(at, length);  // ��¼URL
	return 0;
}

int CHttpParser::OnStatus(const char* at, size_t length)
{
	m_status = Buffer(at, length);  // ��¼״̬��
	return 0;
}

int CHttpParser::OnHeaderField(const char* at, size_t length)
{
	m_lastField = Buffer(at, length);  // �ݴ��ֶ������ȴ�ƥ��ֵ
	return 0;
}

int CHttpParser::OnHeaderValue(const char* at, size_t length)
{
	m_HeaderValues[m_lastField] = Buffer(at, length);  // ���ֶ�ֵ
	return 0;
}

int CHttpParser::OnHeadersComplete()
{ return 0; }

int CHttpParser::OnBody(const char* at, size_t length)
{
	m_body = Buffer(at, length);  // ��¼������
	return 0;
}

int CHttpParser::OnMessageComplete()
{
	m_complete = true;  // ��־�����ɹ�
	return 0;
}

// ���캯���������������URL
UrlParser::UrlParser(const Buffer& url)
{
	m_url = url;
}

// ���Ľ�����������URL����ΪЭ�顢�������˿ڡ�·������ѯ����
int UrlParser::Parser()
{
	// ����1������Э��
	const char* pos = m_url;
	const char* target = strstr(pos, "://");
	if (target == NULL) return -1;
	m_protocol = Buffer(pos, target);  // ��ȡ "http"��"https"

	// ����2����������+�˿�
	pos = target + 3;
	target = strchr(pos, '/');
	if (target == NULL) {
		// ˵��û�� URI��������
		if (m_protocol.size() + 3 >= m_url.size())
			return -2;
		m_host = pos;
		return 0;
	}

	Buffer value = Buffer(pos, target);  // ���� + �˿ڲ���
	if (value.size() == 0) return -3;

	target = strchr(value, ':');  // �����Ƿ���˿�
	if (target != NULL) {
		m_host = Buffer(value, target);  // ��ȡ����
		m_port = atoi(Buffer(target + 1, (char*)value + value.size()));  // ��ȡ�˿ں�
	}
	else {
		m_host = value;  // Ĭ���޶˿ڣ�ʹ��80
	}

	// ����3������ URI �� ��ѯ����
	pos = strchr(pos, '/');
	target = strchr(pos, '?');
	if (target == NULL) {
		m_uri = pos;  // û�в���������Ϊ URI
		return 0;
	} else {
		m_uri = Buffer(pos, target);  // ���� URI
		pos = target + 1;  // ��λ����������
		const char* t = NULL;

		do {
			target = strchr(pos, '&');  // ������һ������
			if (target == NULL) {
				// ���һ������
				t = strchr(pos, '=');
				if (t == NULL) return -4;
				m_values[Buffer(pos, t)] = Buffer(t + 1);  // key=value
			}
			else {
				Buffer kv(pos, target);  // ��ȡ��ֵ���ַ���
				t = strchr(kv, '=');
				if (t == NULL) return -5;
				m_values[Buffer(kv, t)] = Buffer(t + 1, kv + kv.size());  // �ָ� key �� value
				pos = target + 1;  // ��һ������
			}
		} while (target != NULL);
	}

	return 0;
}

// ��ȡ���������ָ��key��ֵ
Buffer UrlParser::operator[](const Buffer& name) const
{
	auto it = m_values.find(name);
	if (it == m_values.end()) return Buffer();  // �Ҳ������ؿ�
	return it->second;
}

// �����µ�URL���������״̬
void UrlParser::SetUrl(const Buffer& url)
{
	m_url = url;
	m_protocol = "";
	m_host = "";
	m_port = 80;
	m_values.clear();
}
