#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include "cJSON.h"
#include "log.h"
#include "OssHeader.h"


OssHeader::OssHeader(const char* headerCfgPath)
{
	m_header_path = headerCfgPath;
}


int OssHeader::ReadFile(string& text)
{
	FILE* f = fopen(m_header_path.c_str(), "rb");
	if (!f)
	{
		log_error("open file[%s] failed", m_header_path.c_str());
		return -1;
	}
	
	fseek(f, 0, SEEK_END);
	long len = ftell(f);
	fseek(f, 0, SEEK_SET);
	
	char* data = (char *)malloc(len + 1);
	if (!data)
	{
		log_error("memory not enough");
		fclose(f);
		return -1;
	}
	
	fread(data, 1, len, f);
	text.assign(data, len);
	
	fclose(f);
	free(data);
	return 0;
}


int OssHeader::init()
{
	string text;
	
	if (ReadFile(text))
	{
		log_error("read file failed.");
		return -1;
	}

	cJSON* json = cJSON_Parse(text.c_str());
	if (!json)
	{
		log_error("Error before: [%s]",cJSON_GetErrorPtr());
		return -1;
	}

	cJSON* item = json->child;
	while (item)
	{
		log_debug("name:%s,value:%s", item->string, item->valuestring);
		if (string(item->string).compare("Cache-Control") == 0) {
			m_cache_control = item->valuestring;
		}
		else if (string(item->string).compare("Content-Disposition") == 0) {
			m_content_disposition = item->valuestring;
		}
		else if (string(item->string).compare("Content-Encoding") == 0) {
			m_content_encoding = item->valuestring;
		}
		else if (string(item->string).compare("Expires") == 0) {
			m_expires = item->valuestring;
		}
		else {
			log_error("Invalid header param.");
		}
		
		item = item->next;
	}

	cJSON_Delete(json);
	return 0;
}


string OssHeader::get_cache_control()
{
	return m_cache_control;
}


string OssHeader::get_content_disposition()
{
	return m_content_disposition;
}


string OssHeader::get_content_encoding()
{
	return m_content_encoding;
}


string OssHeader::get_expires()
{
	return m_expires;
}


