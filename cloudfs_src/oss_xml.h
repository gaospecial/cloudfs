#ifndef OSS_XML_H_
#define OSS_XNL_H_

#include <vector>
#include <string>
#include "oss_api.h"

using namespace std;

typedef struct oss_error
{
	string code;
	string message;
	string request_id;
	string host;
} oss_error_t;

oss_result_t convert_xml_to_bucket_list(string xml_string, vector<oss_bucket_desc_t> & bucket_list);

oss_result_t append_xml_to_object_list(string xml_string, vector<oss_object_desc_t> & object_list);

oss_result_t get_xml_error(string xml_string, oss_error_t & error);

bool is_truncated(string xml_string);

string get_next_marker(string xml_string);

#endif // OSS_XML_H_
