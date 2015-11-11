#ifndef _ALIYUN_OSS_ERR_H_
#define _ALIYUN_OSS_ERR_H_

typedef unsigned int oss_result_t;

#define OSS_OK                          0
#define OSS_ACCESS_DENIED               1
#define OSS_BUCKET_ALREADY_EXISTS       2
#define OSS_BUCKET_NOT_EMPTY            3
#define OSS_ENTITY_TOO_LARGE            4
#define OSS_ENTITY_TOO_SMALL            5
#define OSS_FILE_PART_NOT_EXIST         6
#define OSS_FILE_PART_STALE             7
#define OSS_INVALID_ARGUMENT            8
#define OSS_INVALID_ACCESS_KEY_ID       9
#define OSS_INVALID_BUCKET_NAME         10
#define OSS_INVALID_DIGEST              11
#define OSS_INVALID_OBJECT_NAME         12
#define OSS_INVALID_PART                13
#define OSS_INVALID_PART_ORDER          14
#define OSS_INTERNAL_ERROR              15
#define OSS_MALFORMED_XML               16
#define OSS_METHOD_NOT_ALLOWED          17
#define OSS_MISSING_ARGUMENT            18
#define OSS_MISSING_CONTENT_LENGTH      19
#define OSS_NO_SUCH_BUCKET              20
#define OSS_NO_SUCH_OBJECT              21
#define OSS_NO_SUCH_UPLOAD              22
#define OSS_NOT_IMPLEMENTED             23
#define OSS_PRECONDITION_FAILED         24
#define OSS_REQUEST_TIME_TOO_SKEWED     25
#define OSS_REQUEST_TIMEOUT             36
#define OSS_SIGNATURE_DOES_NOT_MATCH    37
#define OSS_TOO_MANY_BUCKETS            38
#define OSS_XML_PARSE_ERROR             101
#define OSS_CURL_ERROR                  201
#define OSS_UNKNOW_ERROR                999

#endif // _ALIYUN_OSS_ERR_H_
