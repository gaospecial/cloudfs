#include <string>
#include <iostream>
#include <map>
#include <vector>
#include <curl/curl.h>
#include <errno.h>
#include <time.h>
#include <stdlib.h>
#include <libxml/xpath.h>
#include <libxml/xpathInternals.h>
#include <libxml/parser.h>
#include <libxml/tree.h>
#include <libxml/xmlmemory.h>
#include "oss_xml.h"
#include "util.h"

using namespace std;

oss_result_t convert_error_to_code(string error_code);

bool is_truncated(string xml_string)
{
    if(xml_string.find("<IsTruncated>true</IsTruncated>") != string::npos)
        return true;

    return false;
}

oss_result_t get_xml_error(string xml_string, oss_error_t & error)
{
    xmlDocPtr doc;
    xmlXPathContextPtr ctx;
    xmlXPathObjectPtr err_xp;
    xmlXPathObjectPtr code_xp;
    xmlXPathObjectPtr msg_xp;
    xmlXPathObjectPtr req_xp;
    xmlXPathObjectPtr host_xp;
    xmlNodeSetPtr nodes;
    char *next_marker;

    doc = xmlReadMemory(xml_string.c_str(), strlen(xml_string.c_str()), "", NULL, 0);
    ctx = xmlXPathNewContext(doc);
    err_xp = xmlXPathEvalExpression((xmlChar *) "//Code", ctx);
    
    nodes = err_xp->nodesetval;
    if (nodes->nodeNr < 1)
    {
        xmlXPathFreeObject(err_xp);
        xmlXPathFreeContext(ctx);
        xmlFreeDoc(doc);
        return OSS_OK;
    }
    // OK, we have error xml

    //ctx->node = nodes->nodeTab[0];
    xmlNodeSetPtr tmp_nodes;
    code_xp = xmlXPathEvalExpression((xmlChar *) "//Code", ctx);
    tmp_nodes = code_xp->nodesetval;
    next_marker = (char *) xmlNodeListGetString(doc, tmp_nodes->nodeTab[0]->xmlChildrenNode, 1);
    error.code = string(next_marker);
    error.code = trim(error.code);

    if (next_marker)	
    {
    	xmlFree(next_marker);
    	next_marker = NULL;
	}
    xmlXPathFreeObject(code_xp);

    msg_xp = xmlXPathEvalExpression((xmlChar *) "//Message", ctx);
    tmp_nodes = msg_xp->nodesetval;
    next_marker = (char *) xmlNodeListGetString(doc, tmp_nodes->nodeTab[0]->xmlChildrenNode, 1);
    error.message = string(next_marker);
    error.message = trim(error.message);

    if (next_marker)	
    {
    	xmlFree(next_marker);
    	next_marker = NULL;
	}
    xmlXPathFreeObject(msg_xp);

    req_xp = xmlXPathEvalExpression((xmlChar *) "//RequestId", ctx);
    tmp_nodes = req_xp->nodesetval;
    next_marker = (char *) xmlNodeListGetString(doc, tmp_nodes->nodeTab[0]->xmlChildrenNode, 1);
    error.request_id = string(next_marker);
    error.request_id = trim(error.request_id);

    if (next_marker)	
    {
    	xmlFree(next_marker);
    	next_marker = NULL;
	}    
    xmlXPathFreeObject(req_xp);

    host_xp = xmlXPathEvalExpression((xmlChar *) "//HostId", ctx);
    tmp_nodes = host_xp->nodesetval;
    next_marker = (char *) xmlNodeListGetString(doc, tmp_nodes->nodeTab[0]->xmlChildrenNode, 1);
    error.host = string(next_marker);
    error.host = trim(error.host);

    if (next_marker)	
    {
    	xmlFree(next_marker);
    	next_marker = NULL;
	}    
    xmlXPathFreeObject(host_xp);

	

	xmlXPathFreeObject(err_xp);
    xmlXPathFreeContext(ctx);
    xmlFreeDoc(doc);

    return convert_error_to_code(error.code);
}

string get_next_marker(string xml_string)
{
    xmlDocPtr doc;
    xmlXPathContextPtr ctx;
    xmlXPathObjectPtr marker_xp;
    xmlNodeSetPtr nodes;
    char *next_marker;

    doc = xmlReadMemory(xml_string.c_str(), strlen(xml_string.c_str()), "", NULL, 0);
    ctx = xmlXPathNewContext(doc);
    marker_xp = xmlXPathEvalExpression((xmlChar *) "//NextMarker", ctx);
    nodes = marker_xp->nodesetval;

    if(nodes->nodeNr < 1)
    {
        xmlXPathFreeObject(marker_xp);
        xmlXPathFreeContext(ctx);
        xmlFreeDoc(doc);
        return "";
    }
    next_marker = (char *) xmlNodeListGetString(doc, nodes->nodeTab[0]->xmlChildrenNode, 1);    
    string s_next_marker = string(next_marker);

	if (next_marker != NULL)
	{
		xmlFree(next_marker);
		next_marker = NULL;
	}
    xmlXPathFreeObject(marker_xp);
    xmlXPathFreeContext(ctx);
    xmlFreeDoc(doc);

    return s_next_marker;
}

oss_result_t convert_xml_to_bucket_list(string xml_string, vector<oss_bucket_desc_t> & bucket_list)
{
    xmlDocPtr doc;
    xmlXPathContextPtr ctx;
    xmlXPathObjectPtr contents_xp;
    xmlNodeSetPtr content_nodes;

    doc = xmlReadMemory(xml_string.c_str(), strlen(xml_string.c_str()), "", NULL, 0);
    if(doc == NULL)
        return -1;

    ctx = xmlXPathNewContext(doc);

    contents_xp = xmlXPathEvalExpression((xmlChar *) "//Bucket", ctx);
    content_nodes = contents_xp->nodesetval;

    int i;
    for(i = 0; i < content_nodes->nodeNr; i++) {
        ctx->node = content_nodes->nodeTab[i];
        oss_bucket_desc_t bucket_desc;
        // bucket name
        xmlXPathObjectPtr name_node = xmlXPathEvalExpression((xmlChar *) "Name", ctx);
        xmlNodeSetPtr key_nodes = name_node->nodesetval;
        char * name_string = (char *)xmlNodeListGetString(doc, key_nodes->nodeTab[0]->xmlChildrenNode, 1);
        bucket_desc.name = string(name_string);
        
        // bucket create time
        xmlXPathObjectPtr creation_date_node = xmlXPathEvalExpression((xmlChar *) "CreationDate", ctx);
        key_nodes = creation_date_node->nodesetval;
        char * date_string = (char *)xmlNodeListGetString(doc, key_nodes->nodeTab[0]->xmlChildrenNode, 1);
        bucket_desc.creation_date = string(date_string);
        bucket_list.push_back(bucket_desc);

		//释放XML库函数申请返回的内存资源
		xmlFree(name_string);
		xmlFree(date_string);
        xmlXPathFreeObject(name_node);
        xmlXPathFreeObject(creation_date_node);
    }

    xmlXPathFreeObject(contents_xp);
    xmlXPathFreeContext(ctx);
    xmlFreeDoc(doc);

    return OSS_OK;
}

oss_result_t append_xml_to_object_list(string xml_string, vector<oss_object_desc_t> & object_list)
{
    xmlDocPtr doc;
    xmlXPathContextPtr ctx;
    xmlXPathObjectPtr contents_xp;
    xmlNodeSetPtr content_nodes;

    doc = xmlReadMemory(xml_string.c_str(), strlen(xml_string.c_str()), "", NULL, 0);
    if(doc == NULL)
        return -1;

    ctx = xmlXPathNewContext(doc);

    contents_xp = xmlXPathEvalExpression((xmlChar *) "//Contents", ctx);
    content_nodes = contents_xp->nodesetval;

    int i;
    //获取content节点元素
    for(i = 0; i < content_nodes->nodeNr; i++) {
        ctx->node = content_nodes->nodeTab[i];
        oss_object_desc_t object_desc;
        // object name
        xmlXPathObjectPtr key_object = xmlXPathEvalExpression((xmlChar *) "Key", ctx);
        xmlNodeSetPtr key_nodes = key_object->nodesetval;
        char * key_string = (char *)xmlNodeListGetString(doc, key_nodes->nodeTab[0]->xmlChildrenNode, 1);
        object_desc.name = string(key_string);
        
        // object type
        xmlXPathObjectPtr type_object = xmlXPathEvalExpression((xmlChar *)"Type", ctx);
        key_nodes = type_object->nodesetval;
        char * type_string = (char *)xmlNodeListGetString(doc, key_nodes->nodeTab[0]->xmlChildrenNode, 1);
        object_desc.type = string(type_string);
        
        // object type size
        xmlXPathObjectPtr size_object = xmlXPathEvalExpression((xmlChar *)"Size", ctx);
        key_nodes = size_object->nodesetval;
        char * size_string = (char *)xmlNodeListGetString(doc, key_nodes->nodeTab[0]->xmlChildrenNode, 1);
        object_desc.size = atoll(size_string);

		log_debug("name: %s", key_string);
		log_debug("type: %s", type_string);
		log_debug("size: %s", size_string);
		
        object_list.push_back(object_desc);

        xmlXPathFreeObject(key_object);
        xmlXPathFreeObject(type_object);
        xmlXPathFreeObject(size_object);

        //xmlNodeListGetString返回的字符串的内存需要调用者释放
		if (key_string != NULL)		xmlFree(key_string);
		if (type_string != NULL)	xmlFree(type_string);
        if (size_string != NULL)	xmlFree(size_string);
        
    }

	//获取所有的CommonPrefix元素, 如果存在的话, CommonPrefix出现在当请求有delimiter时, 目录对象以CommonPrefix形式返回
    xmlXPathObjectPtr commonPrefix_xp;
    xmlNodeSetPtr prefix_nodes;
    commonPrefix_xp = xmlXPathEvalExpression((xmlChar *) "//CommonPrefixes", ctx);
    prefix_nodes = commonPrefix_xp->nodesetval;
	for(i = 0; i < prefix_nodes->nodeNr; i++)
	{
		ctx->node = prefix_nodes->nodeTab[i];
		oss_object_desc_t object_desc;
		
		// object name
		xmlXPathObjectPtr Prefix_object = xmlXPathEvalExpression((xmlChar *) "Prefix", ctx);
		xmlNodeSetPtr Prefix_nodes = Prefix_object->nodesetval;
		char * Prefix_string = (char *)xmlNodeListGetString(doc, Prefix_nodes->nodeTab[0]->xmlChildrenNode, 1);
		object_desc.name = string(Prefix_string);

		// object type size 一定是0
		object_desc.size = 0;

		object_list.push_back(object_desc);

		xmlXPathFreeObject(Prefix_object);

		//xmlNodeListGetString返回的字符串的内存需要调用者释放
		if (Prefix_string != NULL)	  xmlFree(Prefix_string);
		
	}

	xmlXPathFreeObject(commonPrefix_xp);
    xmlXPathFreeObject(contents_xp);
    xmlXPathFreeContext(ctx);
    xmlFreeDoc(doc);


    return OSS_OK;
}

oss_result_t convert_error_to_code(string error_code)
{
	if (error_code == "AccessDenied")
		return OSS_ACCESS_DENIED;
	else if (error_code == "BucketAlreadyExists")
		return OSS_BUCKET_ALREADY_EXISTS;
	else if (error_code == "BucketNotEmpty")
        return OSS_BUCKET_NOT_EMPTY;
	else if (error_code == "EntityTooLarge")
        return OSS_ENTITY_TOO_LARGE;
	else if (error_code == "EntityTooSmall")
		return OSS_ENTITY_TOO_SMALL;
	else if (error_code == "FilePartNotExist")
        return OSS_FILE_PART_NOT_EXIST;
	else if (error_code == "FilePartStale")
        return OSS_FILE_PART_STALE;
	else if (error_code == "InvalidArgument")
        return OSS_INVALID_ARGUMENT;
	else if (error_code == "InvalidAccessKeyId")
        return OSS_INVALID_ACCESS_KEY_ID;
	else if (error_code == "InvalidBucketName")
        return OSS_INVALID_BUCKET_NAME;
	else if (error_code == "InvalidDigest")
        return OSS_INVALID_DIGEST;
	else if (error_code == "InvalidObjectName")
        return OSS_INVALID_OBJECT_NAME;
	else if (error_code == "InvalidPart")
        return OSS_INVALID_PART;
	else if (error_code == "InvalidPartOrder")
        return OSS_INVALID_PART_ORDER;
	else if (error_code == "InternalError")
        return OSS_INTERNAL_ERROR;
	else if (error_code == "MalformedXml")
        return OSS_MALFORMED_XML;
	else if (error_code == "MethodNotAllowed")
        return OSS_METHOD_NOT_ALLOWED;
	else if (error_code == "MissingArgument")
        return OSS_MISSING_ARGUMENT;
	else if (error_code == "MissingContentLength")
        return OSS_MISSING_CONTENT_LENGTH;
	else if (error_code == "NoSuchBucket")
        return OSS_NO_SUCH_BUCKET;
	else if (error_code == "NoSuchObject")
        return OSS_NO_SUCH_OBJECT;
	else if (error_code == "NoSuchUpload")
        return OSS_NO_SUCH_UPLOAD;
	else if (error_code == "NotImplemented")
        return OSS_NOT_IMPLEMENTED;
	else if (error_code == "PreconditionFail")
        return OSS_PRECONDITION_FAILED;
	else if (error_code == "RequestTimeTooSkewed")
        return OSS_REQUEST_TIME_TOO_SKEWED;
	else if (error_code == "RequestTimeout")
        return OSS_REQUEST_TIMEOUT;
	else if (error_code == "SignatureDoesNotMatch")
        return OSS_SIGNATURE_DOES_NOT_MATCH;
	else if (error_code == "TooManyBuckets")
        return OSS_TOO_MANY_BUCKETS;
	else
		return OSS_UNKNOW_ERROR;
}
