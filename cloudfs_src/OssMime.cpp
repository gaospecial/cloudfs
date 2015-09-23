#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include "OssMime.h"
#include "cJSON.h"
#include "log.h"


OssMime::OssMime(const char* mimeCfgPath)
{
	m_mime_path = mimeCfgPath;
	m_mimes.clear();
}


int OssMime::ReadFile(string& text)
{
	FILE* f = fopen(m_mime_path.c_str(), "rb");
	if (!f)
	{
		log_error("open file[%s] failed", m_mime_path.c_str());
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

int OssMime::init()
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
		log_error("Error before: [%s]\n",cJSON_GetErrorPtr());
		return -1;
	}

	char* out=cJSON_Print(json);
	// log_debug("%s\n", out);
	free(out);

	cJSON* item = json->child;
	while (item)
	{
		m_mimes[item->string] = item->valuestring;

		//log_debug("name:[%s],value:[%s]", item->string, item->valuestring);
		item = item->next;
	}

	log_debug("mime type's size:%d", m_mimes.size());
	cJSON_Delete(json);

	return 0;
}


string OssMime::find(string key)
{
	SS_Map_Iter iter;

	iter = m_mimes.find(key);
	if (iter == m_mimes.end())
	{
		log_debug("Can's find key[%s]'s value", key.c_str());
		return "";
	}

	return iter->second;
}
