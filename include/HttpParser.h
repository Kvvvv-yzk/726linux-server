#pragma once // ��ֹͷ�ļ�����ΰ���

#include "Socket.h"          // �Զ���Socket���ͷ�ļ������ܰ���Buffer���壩
#include "http_parser.h"     // ����http_parser��ͷ�ļ�
#include <map>               // ʹ��std::map����

// HTTP ������
class CHttpParser
{
private:
	http_parser m_parser;                    // HTTP������ʵ��
	http_parser_settings m_settings;         // HTTP���������ã��ص������󶨣�
	std::map<Buffer, Buffer> m_HeaderValues; // �����������HTTPͷ���ֶμ���ֵ
	Buffer m_status;                         // ״̬�У���HTTP/1.1 200 OK��
	Buffer m_url;                            // ����URL
	Buffer m_body;                           // ����������
	bool m_complete;                         // �Ƿ������ɱ�־
	Buffer m_lastField;                      // ��ʱ���浱ǰ���ڽ������ֶ���
public:
	CHttpParser();                           // ���캯��
	~CHttpParser();                          // ��������
	CHttpParser(const CHttpParser& http);    // �������캯��
	CHttpParser& operator=(const CHttpParser& http); // ��ֵ����������

public:
	size_t Parser(const Buffer& data);       // ������������ݣ������򲿷�HTTP����

	// ��ȡHTTP������ţ��ο�http_parser.h�е�HTTP_METHOD_MAP��
	unsigned Method() const { return m_parser.method; }

	const std::map<Buffer, Buffer>& Headers() { return m_HeaderValues; } // ��ȡHTTPͷ��map
	const Buffer& Status() const { return m_status; }                    // ��ȡ״̬��
	const Buffer& Url() const { return m_url; }                          // ��ȡ����URL
	const Buffer& Body() const { return m_body; }                        // ��ȡ������
	unsigned Errno() const { return m_parser.http_errno; }              // ��ȡ����������

protected:
	// ����Ϊlibhttp_parserҪ��ľ�̬�ص���������
	static int OnMessageBegin(http_parser* parser);                      // ��Ϣ��ʼ
	static int OnUrl(http_parser* parser, const char* at, size_t length); // ����URL
	static int OnStatus(http_parser* parser, const char* at, size_t length); // ����״̬��
	static int OnHeaderField(http_parser* parser, const char* at, size_t length); // ����ͷ���ֶ���
	static int OnHeaderValue(http_parser* parser, const char* at, size_t length); // ����ͷ���ֶ�ֵ
	static int OnHeadersComplete(http_parser* parser);                  // ͷ���������
	static int OnBody(http_parser* parser, const char* at, size_t length); // ����Body
	static int OnMessageComplete(http_parser* parser);                  // ��Ϣ�������

	// ����Ϊ��Ӧ�ĳ�Ա����ʵ�֣�����̬�����лص�ʹ��
	int OnMessageBegin();
	int OnUrl(const char* at, size_t length);
	int OnStatus(const char* at, size_t length);
	int OnHeaderField(const char* at, size_t length);
	int OnHeaderValue(const char* at, size_t length);
	int OnHeadersComplete();
	int OnBody(const char* at, size_t length);
	int OnMessageComplete();
};

// URL�����ࣨ����Э�顢�������˿ڡ���ѯ�����ȣ�
class UrlParser
{
public:
	UrlParser(const Buffer& url); // ���캯���������������URL
	~UrlParser() {}

	int Parser();                             // ����URL
	Buffer operator[](const Buffer& name)const; // ��ȡ��ѯ������ָ���ֶ�ֵ

	Buffer Protocol() const { return m_protocol; } // ��ȡЭ�飨��http/https��
	Buffer Host() const { return m_host; }         // ��ȡ��������IP��ַ
	int Port() const { return m_port; }            // ��ȡ�˿ںţ�Ĭ�Ϸ���80
	void SetUrl(const Buffer& url);                // �����µ�URL��׼�����½���

private:
	Buffer m_url;           // ԭʼURL
	Buffer m_protocol;      // Э�鲿��
	Buffer m_host;          // ��������
	Buffer m_uri;           // URI·������
	int m_port;             // �˿ں�
	std::map<Buffer, Buffer> m_values; // �洢URL�еĲ�ѯ����(key-value)
};
