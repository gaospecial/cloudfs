

#ifndef OSS_CONN_H_
#define OSS_CONN_H_

// memory structure for curl write memory callback
struct ReadBuffer {
	char *text;
	size_t size;
};

// memory structure for POST
struct WriteBuffer {
	const char *readptr;
	int sizeleft;
};

typedef std::pair<double, double> progress_t;

CURL *create_curl_handle(void);
void destroy_curl_handle(CURL *curl_handle);
int curl_enhanced_perform(CURL* curl, ReadBuffer* body = NULL, FILE* f = 0);
size_t WriteMemoryCallback(void *ptr, size_t blockSize, size_t numBlocks,
		void *data);
size_t read_callback(void *ptr, size_t size, size_t nmemb, void *userp);
int curl_enhanced_progress(void *clientp, double dltotal, double dlnow,
		double ultotal, double ulnow);
void locate_bundle(void);
size_t header_callback(void *data, size_t blockSize, size_t numBlocks, void *userPtr);
#endif /* OSS_CONN_H_ */
