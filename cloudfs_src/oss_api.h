#ifndef _ALIYUN_OSS_API_H_
#define _ALIYUN_OSS_API_H_

#include <string>
#include <map>
#include <vector>
#include <stdint.h>
#include <sys/time.h>
#include "oss_err.h"

using namespace std;

typedef enum { acl_private = 0, acl_public_read, acl_public_read_write } AclType; 

typedef struct oss_address {
    string          hostname;
    string          access_id;
    string          access_key;
} oss_address_t;

// bucket data structures
typedef struct oss_bucket_desc {
    string          name;
    string          creation_date;
} oss_bucket_desc_t;

// bucket operation
oss_result_t list_bucket (
            oss_address_t & address, 
            vector<oss_bucket_desc_t> & output_bucket_list
            );

// object data structures
typedef struct oss_list_object_param {
    string          prefix;
    string          max_keys;
    string          marker;
    string          delimiter;
} oss_list_object_param_t;

typedef struct oss_object_desc {
    string          name;
    string          type;
    uint64_t        size;
    string          cache_control;
    string          expires;
    string          content_encoding;
    string          content_disposition;
    map<string, string> user_meta;
} oss_object_desc_t;

typedef struct oss_get_object_header {
    string          if_modified_since;
    string          if_unmodified_since;
    string          if_match;
    string          if_none_match;
} oss_get_object_header_t;

typedef struct oss_get_object_response_param {
    string          content_type;
    string          content_language;
    string          expires;
    string          cache_control;
    string          content_encoding;
    string			content_disposition;
} oss_get_object_response_param_t;

typedef struct oss_batch_delete {
    bool            is_quiet;
    string          length;
    string          md5;
    vector<string>  object_names;
} oss_batch_delete_t;

typedef unsigned char * oss_data_buffer_t;

// object operation
oss_result_t list_object (
            oss_address_t & address, 
            string & bucket_name, 
            oss_list_object_param_t & params, 
            vector<oss_object_desc_t> & output_object_desc_list
            );

oss_result_t put_object (
            oss_address_t & address, 
            string & bucket_name, 
            oss_object_desc_t & object,
            oss_data_buffer_t & data
            );

oss_result_t get_object (
            oss_address_t & address, 
            string & bucket_name, 
            string & object_name,
            oss_get_object_header_t & header,
            string range,
            oss_get_object_response_param_t & params,
            oss_object_desc_t & output_object,
            oss_data_buffer_t & output_data
            );

oss_result_t copy_object (
            oss_address_t & address, 
            string & bucket_name, 
            string & source_copy,
            string & dest_copy,
            oss_get_object_header_t & header,
            bool replace_meta
            );

oss_result_t head_object (
            oss_address_t & address, 
            string & bucket_name, 
            string & object_name, 
            oss_get_object_header_t & header,
            oss_object_desc_t & output_object_desc
            );

oss_result_t delete_object (
            oss_address_t & address, 
            string & bucket_name, 
            string & object_name
            );

oss_result_t batch_delete_object (
            oss_address_t & address, 
            string & bucket_name, 
            oss_batch_delete_t & batch_delete_request
            );

#endif // _ALIYUN_OSS_API_H
