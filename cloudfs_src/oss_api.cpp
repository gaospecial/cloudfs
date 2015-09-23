#include <string>
#include <map>
#include <vector>
#include <iostream>
#include <sstream>
#include <curl/curl.h>
#include <syslog.h>
#include <errno.h>
#include <time.h>
#include <stdlib.h>
#include <libxml/xpath.h>
#include <libxml/xpathInternals.h>
#include <libxml/parser.h>
#include <libxml/tree.h>
#include "oss_api.h"
#include "oss_err.h"
#include "oss_conn.h"
#include "oss_xml.h"
#include "util.h"
#include "log.h"
#include "client.h"

class auto_curl_slist {
public:
	auto_curl_slist() :
			slist(0) {
	}
	~auto_curl_slist() {
		curl_slist_free_all(slist);
	}

	struct curl_slist* get() const {
		return slist;
	}

	void append(const string& s) {
		slist = curl_slist_append(slist, s.c_str());
	}

private:
	struct curl_slist* slist;
};

static void composite_get_headers(map<string, string> & header_map, string & range, oss_get_object_header_t & headers);

static void composite_head_headers(map<string, string> & header_map, oss_get_object_header_t & headers);

static void composite_put_headers(map<string, string> & header_map, oss_object_desc_t & headers);

static void add_map_to_curl_headers(auto_curl_slist & curl_list, map<string, string> & header_map);

static void composite_get_response_source(string & source, oss_get_object_response_param_t & params);

static void print_map(map<string, string> & header_map);

oss_result_t list_bucket (
            oss_address_t & address, 
            vector<oss_bucket_desc_t> & output_bucket_list
            )
{
	CURL* curl;
	string date_str = get_date();
    string url = address.hostname;
    string method = "GET";
    map<string, string> headers;

    //注意: source内容只需要是utf-8编码即可, 不需要进行http url 转义操作
    string source = "/";
    auto_curl_slist curl_headers;
    curl_headers.append("Date: " + date_str);
    curl_headers.append("ContentType: ");
    curl_headers.append("Authorization: OSS " + address.access_id + ":" + calc_signature("GET", address.access_id, address.access_key,
    		date_str, source, headers));
    ReadBuffer body;

    body.text = (char *) malloc(1);
    body.size = 0;
   
    log_debug("before curl, url is %s\n", url.c_str());
    curl = create_curl_handle();
    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *) &body);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteMemoryCallback);
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, curl_headers.get());

    int result = curl_enhanced_perform(curl, &body);
    destroy_curl_handle(curl);

    if (result == 0) 
    {
        log_debug("body text is %s\n", body.text);
	}
    else
    {
        if (body.text) 
            free(body.text);
        return OSS_CURL_ERROR;
    }
    oss_result_t xml_parse_ret = OSS_OK;

    xml_parse_ret = convert_xml_to_bucket_list(body.text, output_bucket_list);

    if(body.text)
    {
    	free(body.text);
    }

    return xml_parse_ret;
}

oss_result_t list_object (
            oss_address_t & address, 
            string & bucket_name, 
            oss_list_object_param_t & params, 
            vector<oss_object_desc_t> & output_object_desc_list
            )
{
	CURL* curl;
	string date_str = get_date();
    string url = address.hostname;
    string method = "GET";
    map<string, string> headers;

    //注意: source内容只需要是utf-8编码即可, 不需要进行http url 转义操作
    string source = "/" + bucket_name + "/" ;
    string next_marker = "";
    oss_result_t xml_parse_ret;
    ReadBuffer body;
    bool truncated = true;
    string query = "";
    if (params.delimiter != "")
    {
        query = "delimiter=" + url_encode(params.delimiter);
    }
    if (params.prefix != "")
    {
        if (query != "")
            query += "&";
        query += "prefix=" + url_encode(params.prefix);
    }
    if (params.max_keys != "")
    {
        if (query != "")
            query += "&";
        query += "max-keys=" + url_encode(params.max_keys);
    }
    url = prepare_url(url.c_str(), bucket_name);
    while (truncated) 
    {
        auto_curl_slist curl_headers;
        string real_url = url + "/?" + query;
        if (next_marker != "") 
        {
            if (query != "")
                real_url += "&";
            real_url += "marker=" + url_encode(next_marker);
        }
        curl_headers.append("Date: " + date_str);
        curl_headers.append("ContentType: ");
        curl_headers.append("Authorization: OSS " + address.access_id + ":" + calc_signature("GET", address.access_id, address.access_key,
                    date_str, source, headers));

		struct curl_slist* tmp_slist = NULL;
		log_debug("headers : ");
		tmp_slist = curl_headers.get();
		while (tmp_slist)
		{
			log_debug("%s", tmp_slist->data);
			tmp_slist = tmp_slist->next;
		}

        body.text = (char *) malloc(1);
        body.size = 0;
        log_debug("before curl, url is %s", real_url.c_str());

        curl = create_curl_handle();
        curl_easy_setopt(curl, CURLOPT_URL, real_url.c_str());
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *) &body);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteMemoryCallback);
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, curl_headers.get());

        int result = curl_enhanced_perform(curl, &body);
        destroy_curl_handle(curl);
        if (result == 0) 
        {   
        	//此处不能使用log_debug来打印body.text, 因为这部分主要是XML字符串, 超过了log_debug目前的记录能力
            //log_debug("body text is %s\n", body.text);
		}
        else
        {
            xml_parse_ret = OSS_CURL_ERROR;
            break;
        }

        xml_parse_ret= append_xml_to_object_list(body.text, output_object_desc_list);
        if (xml_parse_ret != OSS_OK)
            break;
        truncated = is_truncated(body.text);
        if (truncated)
        {
            next_marker = get_next_marker(body.text);
            //cout << "Next marker is " << next_marker << endl;
        }
        if(body.text)
        {
            free(body.text);
            body.text = NULL;
		}
    }
    if(body.text)
    {
    	free(body.text);
    	body.text = NULL;
    }

    return xml_parse_ret;
}

oss_result_t put_object (
            oss_address_t & address, 
            string & bucket_name, 
            oss_object_desc_t & object,
            oss_data_buffer_t & data
            )
{
	CURL* curl;
	string date_str = get_date();
    string url = address.hostname;
    string method = "PUT";
    map<string, string> headers;
    //注意: source内容只需要是utf-8编码即可, 不需要进行http url 转义操作
    string source = "/" + bucket_name + "/" + object.name;

    oss_result_t ret_val = OSS_OK;

    url = prepare_url(url.c_str(), bucket_name);
    url += url_encode("/" + object.name);
    auto_curl_slist curl_headers;
    curl_headers.append("Date: " + date_str);
    //curl_headers.append("Content-Type: " + object.type);
    //curl_headers.append("Content-Type: ");
    //curl_headers.append("Content-Length: " + str(object.size));
    // x-oss headers: (a) alphabetical order and (b) no spaces after colon
    composite_put_headers(headers, object);

    curl_headers.append("Authorization: OSS " + address.access_id + ":" + calc_signature(method, address.access_id, address.access_key,
                date_str, source, headers));

    add_map_to_curl_headers(curl_headers, headers);

    log_debug("before curl, url is %s.", url.c_str());
    curl = create_curl_handle();

    ReadBuffer body;
    WriteBuffer write_buffer;
    write_buffer.readptr = (const char *)data;
    write_buffer.sizeleft = object.size;
    body.size = 0;
    body.text = (char *) malloc(1);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&body);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteMemoryCallback);
    curl_easy_setopt(curl, CURLOPT_UPLOAD, true); // HTTP PUT
    curl_easy_setopt(curl, CURLOPT_INFILESIZE, object.size); // Content-Length
    if (object.size != 0) {
        curl_easy_setopt(curl, CURLOPT_READFUNCTION, read_callback);
        curl_easy_setopt(curl, CURLOPT_READDATA, &write_buffer);
    }
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, curl_headers.get());
    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());

    int result = curl_enhanced_perform(curl);
    if (result == 0)
    {
    	//cout << "put data successfully" << endl;
    }
    else
    {
        //cout << body.text << endl;
    	ret_val = OSS_CURL_ERROR;
    }

    destroy_curl_handle(curl);

    if(body.text)
    {
    	free(body.text);
    }
    return ret_val;
}

oss_result_t get_object (
            oss_address_t & address, 
            string & bucket_name, 
            string & object_name,
            oss_get_object_header_t & header,
            string range,
            oss_get_object_response_param_t & params,
            oss_object_desc_t & output_object,
            oss_data_buffer_t & output_data
            )
{
	CURL* curl;
	string date_str = get_date();
    string url = address.hostname;
    string method = "GET";
    map<string, string> headers;

    //注意: source内容只需要是utf-8编码即可, 不需要进行http url 转义操作
    string source = "/" + bucket_name + "/" + object_name ;
    string additional_source;
    composite_get_response_source(additional_source, params);
    if (additional_source != "")
        additional_source = "?" + additional_source;
    //source += "?" + additional_source;

    oss_result_t ret_val = OSS_OK;

    url = prepare_url(url.c_str(), bucket_name);
    url += url_encode("/" + object_name + additional_source);
    auto_curl_slist curl_headers;
    curl_headers.append("Date: " + date_str);
    curl_headers.append("Content-Type: ");
    // x-oss headers: (a) alphabetical order and (b) no spaces after colon
    composite_get_headers(headers, range, header);

    curl_headers.append("Authorization: OSS " + address.access_id + ":" + calc_signature(method, address.access_id, address.access_key,
                date_str, source, headers));

    add_map_to_curl_headers(curl_headers, headers);

    log_debug("before curl, url is %s\n", url.c_str());
    curl = create_curl_handle();
    map<string, string> response_headers;

    ReadBuffer body;
    body.size = 0;
    body.text = (char *) malloc(1);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&body);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteMemoryCallback);
    curl_easy_setopt(curl, CURLOPT_FILETIME, true); // Last-Modified
    curl_easy_setopt(curl, CURLOPT_HEADERDATA, &response_headers);
    curl_easy_setopt(curl, CURLOPT_HEADERFUNCTION, header_callback);
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, curl_headers.get());
    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());

    int result = curl_enhanced_perform(curl);
    if (result == 0)
    {
        print_map(response_headers);

        output_object.type = response_headers["Content-Type"];
        output_object.size = atoll(response_headers["Content-Length"].c_str());
        output_object.name = object_name;

        if (response_headers.find("Cache-Control") != response_headers.end())
        	output_object.cache_control = response_headers["Cache-Control"];
        if (response_headers.find("Content-Disposition") != response_headers.end())
        	output_object.content_disposition = response_headers["Content-Disposition"];
        if (response_headers.find("Content-Encoding") != response_headers.end())
        	output_object.content_encoding = response_headers["Content-Encoding"];
        if (response_headers.find("Expires") != response_headers.end())
        	output_object.expires = response_headers["Expires"];

        map<string, string>::iterator iter = response_headers.begin();
        for (; iter != response_headers.end(); iter++)
        {
        	if (iter->first.find("x-oss") == 0) {
        		output_object.user_meta[iter->first] = iter->second;
        	}
        }
        output_data = new unsigned char[output_object.size + 1];
        memcpy((void *) output_data, (const void *) body.text, output_object.size + 1);
    }
    else
    {
    	ret_val = OSS_CURL_ERROR;
    }

    destroy_curl_handle(curl);

    if(body.text)
    {
    	free(body.text);
    }
    return ret_val;
}

oss_result_t copy_object (
            oss_address_t & address,
            string & bucket_name,
            string & source_copy,
            string & dest_copy,
            oss_get_object_header_t & header,
            bool replace_meta)
{
	unsigned short retcode = -1;
	const char *retinfo = NULL;

	if (dest_copy.at(0) == '/')
		dest_copy = dest_copy.substr(1);
	
	oss_client_t *client = client_initialize_with_endpoint(address.access_id.c_str(), 
														   address.access_key.c_str(),
														   address.hostname.c_str());
	if (client == NULL)
	{
		log_error("client initialize failed.");
		return OSS_UNKNOW_ERROR;
	}

	oss_copy_object_result_t *result = client_copy_object_ext(client, 
															  bucket_name.c_str(), 
															  source_copy.c_str(),
															  bucket_name.c_str(), 
															  dest_copy.c_str(), 
															  &retcode);
	if (retcode == OK) 
	{
		if (result != NULL) 
		{
			log_debug("ETag: %s", result->get_etag(result));
			log_debug("LastModified: %s", result->get_last_modified(result));
		}
		log_debug("Copy object successfully.");
	} 
	else {
		retinfo = oss_get_error_message_from_retcode(retcode);
		log_debug("error info:%s", retinfo);
	}

	if (result != NULL) copy_object_result_finalize(result);
	client_finalize(client);

	return OSS_OK;
}

oss_result_t head_object (
            oss_address_t & address,
            string & bucket_name,
            string & object_name,
            oss_get_object_header_t & header,
            oss_object_desc_t & output_object_desc
            )
{
	CURL* curl;
	string date_str = get_date();
    string url = address.hostname;
    string method = "HEAD";
    map<string, string> headers;

    //注意: source内容只需要是utf-8编码即可, 不需要进行http url 转义操作
    string source = "/" + bucket_name + "/" + object_name ;

    oss_result_t ret_val = OSS_OK;
    
    url = prepare_url(url.c_str(), bucket_name);
    url += url_encode("/" + object_name);
    auto_curl_slist curl_headers;
    curl_headers.append("Date: " + date_str);
    curl_headers.append("Content-Type: ");
    // x-oss headers: (a) alphabetical order and (b) no spaces after colon
    composite_head_headers(headers, header);
    curl_headers.append("Authorization: OSS " + address.access_id + ":" + calc_signature(method, address.access_id, address.access_key,
                date_str, source, headers));

    add_map_to_curl_headers(curl_headers, headers);

    log_debug("before curl, url is %s\n", url.c_str());
    curl = create_curl_handle();
    map<string, string> response_headers;
    curl_easy_setopt(curl, CURLOPT_NOBODY, true); // HEAD
    curl_easy_setopt(curl, CURLOPT_FILETIME, true); // Last-Modified
    curl_easy_setopt(curl, CURLOPT_HEADERDATA, &response_headers);
    curl_easy_setopt(curl, CURLOPT_HEADERFUNCTION, header_callback);
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, curl_headers.get());
    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());

    int result = curl_enhanced_perform(curl);
    if (result == 0) 
    {
        print_map(response_headers);

        output_object_desc.type = response_headers["Content-Type"];
        output_object_desc.size = atoll(response_headers["Content-Length"].c_str());
        output_object_desc.name = object_name;

        if (response_headers.find("Cache-Control") != response_headers.end())
        	output_object_desc.cache_control = response_headers["Cache-Control"];
        if (response_headers.find("Content-Disposition") != response_headers.end())
        	output_object_desc.content_disposition = response_headers["Content-Disposition"];
        if (response_headers.find("Content-Encoding") != response_headers.end())
        	output_object_desc.content_encoding = response_headers["Content-Encoding"];
        if (response_headers.find("Expires") != response_headers.end())
        	output_object_desc.expires = response_headers["Expires"];
		
		log_debug("object basic: name[%s], size[%lld]", output_object_desc.name.c_str(), output_object_desc.size);
        map<string, string>::iterator iter = response_headers.begin();
        for (; iter != response_headers.end(); iter++)
        {
        	if (iter->first.find("x-oss") == 0) {
        		output_object_desc.user_meta[iter->first] = iter->second;
        		log_debug("user meta[%s::%s]", iter->first.c_str(), iter->second.c_str());
        	}
        }

        
    }
    else
    {
    	ret_val = OSS_CURL_ERROR;
    }
    
    destroy_curl_handle(curl);

    return ret_val;
}

oss_result_t delete_object (
            oss_address_t & address, 
            string & bucket_name, 
            string & object_name
            )
{
	CURL* curl;
	string date_str = get_date();
    string url = address.hostname;
    string method = "DELETE";
    map<string, string> headers;

    //注意: source内容只需要是utf-8编码即可, 不需要进行http url 转义操作
    string source = "/" + bucket_name + "/" + object_name;

    oss_result_t ret_val = OSS_OK;

    url = prepare_url(url.c_str(), bucket_name);
    url += url_encode("/" + object_name);
    auto_curl_slist curl_headers;
    curl_headers.append("Date: " + date_str);
    curl_headers.append("Content-Type: ");

    curl_headers.append("Authorization: OSS " + address.access_id + ":" + calc_signature(method, address.access_id, address.access_key,
                date_str, source, headers));

    log_debug("before curl, url is %s", url.c_str());
    curl = create_curl_handle();

    ReadBuffer body;
    body.size = 0;
    body.text = (char *) malloc(1);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&body);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteMemoryCallback);
    curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "DELETE");
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, curl_headers.get());
    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());

    int result = curl_enhanced_perform(curl);
    if (result == 0)
    {
    	//cout << "delete data successfully" << endl;
    }
    else
    {
        //cout << body.text << endl;
    	ret_val = OSS_CURL_ERROR;
    }

    destroy_curl_handle(curl);

    if(body.text)
    {
    	free(body.text);
    }
    return ret_val;
}

oss_result_t batch_delete_object (
            oss_address_t & address, 
            string & bucket_name, 
            oss_batch_delete_t & batch_delete_request
            )
{
    return OSS_OK;
}

static void composite_get_headers(map<string, string> & header_map, string & range, oss_get_object_header_t & headers)
{
	if (headers.if_modified_since != "")
		header_map["If-Modifed-Since"] = headers.if_modified_since;
	if (headers.if_unmodified_since != "")
		header_map["If-Unmodified-Since"] = headers.if_unmodified_since;
	if (headers.if_match != "")
		header_map["If-Match"] = headers.if_match;
	if (headers.if_none_match != "")
		header_map["If-None-Match"] = headers.if_none_match;
	if (range != "")
		header_map["Range"] = range;
}

static void composite_head_headers(map<string, string> & header_map, oss_get_object_header_t & headers)
{
	if (headers.if_modified_since != "")
		header_map["If-Modifed-Since"] = headers.if_modified_since;
	if (headers.if_unmodified_since != "")
		header_map["If-Unmodified-Since"] = headers.if_unmodified_since;
	if (headers.if_match != "")
		header_map["If-Match"] = headers.if_match;
	if (headers.if_none_match != "")
		header_map["If-None-Match"] = headers.if_none_match;
}

static void composite_put_headers(map<string, string> & header_map, oss_object_desc_t & headers)
{
	header_map.insert(headers.user_meta.begin(), headers.user_meta.end());
	if (headers.cache_control != "")
		header_map["Cache-Control"] = headers.cache_control;
	if (headers.content_disposition != "")
		header_map["Content-Disposition"] = headers.content_disposition;
	if (headers.content_encoding != "")
		header_map["Content-Encoding"] = headers.content_encoding;
	if (headers.expires != "")
		header_map["Expires"] = headers.expires;
	if (headers.type != "")
		header_map["Content-Type"] = headers.type;
	
}

static void composite_get_response_source(string & source, oss_get_object_response_param_t & params)
{
	source = "";
	if (params.cache_control != "")
		source += "response-cache-control=" + params.cache_control;
	if (params.content_disposition != "")
		source += "&response-content-disposition=" + params.content_disposition;
	if (params.content_encoding != "")
		source += "&response-content-encoding=" + params.content_encoding;
	if (params.content_language != "")
		source += "&response-content-language=" + params.content_language;
	if (params.content_type != "")
		source += "&response-content-type=" + params.content_type;
	if (params.expires != "")
		source += "&response-expires=" + params.expires;

	if (source.find("&") == 0)
		source = source.substr(1, source.size() - 1);
}

static void add_map_to_curl_headers(auto_curl_slist & curl_list, map<string, string> & header_map)
{
	map<string, string>::iterator iter = header_map.begin();

	for (; iter != header_map.end(); iter++)
	{
		curl_list.append(iter->first + ":" + iter->second);
	}
}

static void print_map(map<string, string> & header_map)
{
	map<string, string>::iterator iter = header_map.begin();

	for (; iter != header_map.end(); iter++)
	{
		log_debug("map info[%s::%s]", iter->first.c_str(), iter->second.c_str());
	}
}
