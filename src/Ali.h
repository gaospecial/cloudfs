

#ifndef ALI_H_
#define ALI_H_
#define DEFAULT_BLOCK_SIZE (512*1024) //4*2^20
#define DEFAULT_ONLINE_SYNC_CYCLE 10
#define DEFAULT_LOG_LEVEL		  1
#define DEFAULT_IMMEDIATE_SYNC	  1


#ifndef CLOUDFS_VERSION_INFO
#define CLOUDFS_VERSION_INFO	"cloudfs V100R001C01BXXXX"
#endif

#define SAFE_DELETE(p) if(p) {\
    delete p;\
    p=0;\
}

#define SAFE_DELETE_ARRAY(p) if(p) {\
    delete[] p;\
    p=0;\
}

#include <string.h>
#include <memory.h>
#include <algorithm>
typedef struct str_comparer {
    bool operator()(const char* s1, const char* s2) const {
        return strcmp(s1, s2) < 0;
    }
} map_str_comparer;
#define FIND_ALIGN_SIZE(_s,_d){\
	size_t _blk_size=AliConf::BLOCK_SIZE;\
    while(_blk_size>=1024){\
    	if(_s>_blk_size){\
            _s=_blk_size*2;\
            _d=_d-1;\
            break;\
    	}\
    	_d+=1;\
    	_blk_size=_blk_size/2;\
    }\
}

//the while here is not that good, if all small files, may dead loop
#define RE_ALOCATE(_p,_size,_end,_fs) if(_end>_size){\
    size_t _new_size=std::min(_size*2,(size_t)(AliConf::BLOCK_SIZE));\
    _new_size=std::max(_new_size,(size_t)_end);\
    char *_np=new char[_new_size];\
    size_t _gap=_new_size-_size;\
    _fs->increase_cache(_gap);\
    log_debug("tttt increase_cache file:%s  path:%s  gap:%zd", m_group->get_file_name(), m_group->get_path_name(), _gap);\
    memcpy(_np,_p,_size);\
    SAFE_DELETE_ARRAY(_p)\
    _p=_np;\
    _size=_new_size;\
}


#define APPNEND_LIST_ITEM(_p,_pos){\
		if(_pos)\
            _pos->next=_p;\
        _p->prev=_pos;\
}
#define INSERT_LIST_ITEM(_p, _pos,_head){\
		_p->next=_pos;\
        if(_pos&&_pos->prev)\
            _pos->prev->next=_p;\
        else\
            _head=_p;\
        if(_pos){\
            _p->prev=_pos->prev;\
            _pos->prev=_p;\
        }\
}
#define INSERT_LIST_ITEM_WITH_ORDER(_p, _pos,_head){\
		if(_pos){\
            while(_pos){\
                if((_pos->addr>_p->addr)){\
                    INSERT_LIST_ITEM(_p,_pos,_head)\
                    break;\
                }\
                else if(_pos->next==NULL){\
                    APPNEND_LIST_ITEM(_p,_pos)\
                    break;\
                }\
                else if(_pos->addr<_p->addr&&_pos->next->addr>_p->addr){\
                    INSERT_LIST_ITEM(_p,_pos->next,_head)\
                    break;\
                }\
                _pos=_pos->next;\
            }\
		}\
		else{\
			INSERT_LIST_ITEM(_p,_pos,_head)\
        }\
}
#define REMOVE_LIST_ITEM(_p,_head){\
		if(_p->prev)\
            _p->prev->next=_p->next;\
        else\
            _head=_p->next;\
        if(_p->next)\
            _p->next->prev=_p->prev;\
		_p->next=NULL;\
		_p->prev=NULL;\
}
typedef enum OSS_OBJ_TYPE{
    OSS_REGULAR=0,
    OSS_DIR=1,
    OSS_LINK=2
} oss_obj_type_t;
#define handle_error_en(en, msg) \
              do { errno = en; perror(msg); exit(EXIT_FAILURE); } while (0)
#endif /* ALI_H_ */
