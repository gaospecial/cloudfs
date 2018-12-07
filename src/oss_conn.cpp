
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <dirent.h>
#include <errno.h>
#include <syslog.h>
#include <pthread.h>
#include <curl/curl.h>
#include <iostream>
#include <fstream>
#include <string>
#include <map>

#include "oss_conn.h"
#include "util.h"
#include "log.h"


using namespace std;

long connect_timeout = 10;
time_t readwrite_timeout = 30;
std::string ssl_verify_hostname = "0";
int retries = 2;
std::string program_name;
bool debug = true;


pthread_mutex_t curl_handles_lock;
std::map<CURL*, time_t> curl_times;
std::map<CURL*, progress_t> curl_progress;
std::string curl_ca_bundle;

CURL *create_curl_handle(void) {
	time_t now;
	CURL *curl_handle;

	pthread_mutex_lock(&curl_handles_lock);
	curl_handle = curl_easy_init();
	curl_easy_reset(curl_handle);
	curl_easy_setopt(curl_handle, CURLOPT_NOSIGNAL, 1);
	curl_easy_setopt(curl_handle, CURLOPT_FOLLOWLOCATION, true);
	curl_easy_setopt(curl_handle, CURLOPT_CONNECTTIMEOUT, connect_timeout);
	curl_easy_setopt(curl_handle, CURLOPT_NOPROGRESS, 0);
	curl_easy_setopt(curl_handle, CURLOPT_PROGRESSFUNCTION, curl_enhanced_progress);
	curl_easy_setopt(curl_handle, CURLOPT_PROGRESSDATA, curl_handle);
	curl_easy_setopt(curl_handle, CURLOPT_TIMEOUT, 120);
	// curl_easy_setopt(curl_handle, CURLOPT_FORBID_REUSE, 1);

	if (ssl_verify_hostname.substr(0, 1) == "0")
		curl_easy_setopt(curl_handle, CURLOPT_SSL_VERIFYHOST, 0);
//	if (curl_ca_bundle.size() != 0)
//		curl_easy_setopt(curl_handle, CURLOPT_CAINFO, curl_ca_bundle.c_str());

	now = time(0);
	curl_times[curl_handle] = now;
	curl_progress[curl_handle] = progress_t(-1, -1);
	pthread_mutex_unlock(&curl_handles_lock);

	return curl_handle;
}

void destroy_curl_handle(CURL *curl_handle) {
	if (curl_handle != NULL) {
		pthread_mutex_lock(&curl_handles_lock);
		curl_times.erase(curl_handle);
		curl_progress.erase(curl_handle);
		curl_easy_cleanup(curl_handle);
		pthread_mutex_unlock(&curl_handles_lock);
	}

	return;
}

/**
 * @return fuse return code
 */
int curl_enhanced_perform(CURL* curl, ReadBuffer* body, FILE* f) {
	char url[256];
	time_t now;
	char* ptr_url = url;
	curl_easy_getinfo(curl, CURLINFO_EFFECTIVE_URL, &ptr_url);

	log_debug("ptr_url: %s", ptr_url);
	// curl_easy_setopt(curl, CURLOPT_VERBOSE, true);
	if (ssl_verify_hostname.substr(0, 1) == "0")
		curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0);

//	if (curl_ca_bundle.size() != 0)
//		curl_easy_setopt(curl, CURLOPT_CAINFO, curl_ca_bundle.c_str());

	long responseCode;

	// 1 attempt + retries...
	int t = retries + 1;
	while (t-- > 0) {
		if (f) {
			rewind(f);
		}
		CURLcode curlCode = curl_easy_perform(curl);
		if (curlCode != CURLE_OK) {
			log_error("curlCode=%d", curlCode);
		}
		
		switch (curlCode) {
		case CURLE_OK:
			// Need to look at the HTTP response code

			if (curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &responseCode)
					!= 0) {
				log_error("curl_easy_getinfo failed while trying to retrieve HTTP response code");
				return -EIO;
			}
            log_debug("response code is %ld", responseCode);

			if (responseCode < 400) {
				//log_error("response code:%d", responseCode);
				return 0;
			}

			if (responseCode >= 500) {
				log_error("response code:%d", responseCode);
				sleep(4);
				break;
			}

			// Service response codes which are >= 400 && < 500
			switch (responseCode) {
			case 400:
				if (body) {
					if (body->size && debug) {
						log_error("Body Text: %s", body->text);
					}
				}
				return -EIO;

			case 403:
				if (body) {
					if (body->size && debug) {
						log_error("Body Text: %s", body->text);
					}
				}
				return -EIO;

			case 404:
				if (body) {
					if (body->size && debug) {
						log_error("Body Text: %s", body->text);
					}
				}
				return -ENOENT;

			default:
				if (body) {
					if (body->size) {
						log_error("Body Text %s", body->text);
					}
				}
				return -EIO;
			}
			break;

		case CURLE_WRITE_ERROR:
			log_error("### CURLE_WRITE_ERROR");
			sleep(2);
			break;

		case CURLE_OPERATION_TIMEDOUT:
			log_error("### CURLE_OPERATION_TIMEDOUT");
			sleep(2);
			break;

		case CURLE_COULDNT_RESOLVE_HOST:
			log_error("### CURLE_COULDNT_RESOLVE_HOST");
			sleep(2);
			break;

		case CURLE_COULDNT_CONNECT:
			log_error("### CURLE_COULDNT_CONNECT");
			sleep(4);
			break;

		case CURLE_GOT_NOTHING:
			log_error("### CURLE_GOT_NOTHING");
			sleep(4);
			break;

		case CURLE_ABORTED_BY_CALLBACK:
			log_error("### CURLE_ABORTED_BY_CALLBACK");
			sleep(4);
			now = time(0);
			curl_times[curl] = now;
			break;

		case CURLE_PARTIAL_FILE:
			log_error("### CURLE_PARTIAL_FILE");
			sleep(4);
			break;

		case CURLE_SEND_ERROR:
			log_error("### CURLE_SEND_ERROR");
			sleep(2);
			break;

		case CURLE_RECV_ERROR:
			log_error("### CURLE_RECV_ERROR");
			sleep(2);
			break;

		case CURLE_SSL_CACERT:
			// try to locate cert, if successful, then set the
			// option and continue
			if (curl_ca_bundle.size() == 0) {
				locate_bundle();
				if (curl_ca_bundle.size() != 0) {
					t++;
					curl_easy_setopt(curl, CURLOPT_CAINFO,
							curl_ca_bundle.c_str());
					continue;
				}
			}
			log_error("curlCode: %i  msg: %s", curlCode,
					curl_easy_strerror(curlCode));		
			;
			fprintf(stderr, "%s: curlCode: %i -- %s\n", program_name.c_str(),
					curlCode, curl_easy_strerror(curlCode));
			exit(EXIT_FAILURE);
			break;

#ifdef CURLE_PEER_FAILED_VERIFICATION
			case CURLE_PEER_FAILED_VERIFICATION:
			first_pos = bucket.find_first_of(".");
			if (first_pos != string::npos) {
				fprintf (stderr, "%s: curl returned a CURL_PEER_FAILED_VERIFICATION error\n", program_name.c_str());
				fprintf (stderr, "%s: security issue found: buckets with periods in their name are incompatible with https\n", program_name.c_str());
				fprintf (stderr, "%s: This check can be over-ridden by using the -o ssl_verify_hostname=0\n", program_name.c_str());
				fprintf (stderr, "%s: The certificate will still be checked but the hostname will not be verified.\n", program_name.c_str());
				fprintf (stderr, "%s: A more secure method would be to use a bucket name without periods.\n", program_name.c_str());
			} else {
				fprintf (stderr, "%s: my_curl_easy_perform: curlCode: %i -- %s\n",
						program_name.c_str(),
						curlCode,
						curl_easy_strerror(curlCode));
			}
			exit(EXIT_FAILURE);
			break;
#endif

			// This should be invalid since curl option HTTP FAILONERROR is now off
		case CURLE_HTTP_RETURNED_ERROR:
			log_error("CURLE_HTTP_RETURNED_ERROR");	

			if (curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &responseCode)
					!= 0) {
				return -EIO;
			}
			log_error("###response=%ld", responseCode);	

			// Let's try to retrieve the

			if (responseCode == 404) {
				return -ENOENT;
			}
			if (responseCode < 500) {
				return -EIO;
			}
			break;

			// Unknown CURL return code
		default:
			log_error("###curlCode: %i  msg: %s", curlCode,
					curl_easy_strerror(curlCode));			
			
			exit(EXIT_FAILURE);
			break;
		}
		log_error("###retrying...");
	}

	log_error("###giving up");
	return -EIO;
}

// libcurl callback
size_t WriteMemoryCallback(void *ptr, size_t blockSize, size_t numBlocks,
		void *data) {
	size_t realsize = blockSize * numBlocks;
	struct ReadBuffer *mem = (struct ReadBuffer *) data;

	mem->text = (char *) realloc(mem->text, mem->size + realsize + 1);
	if (mem->text == NULL) {
		/* out of memory! */
		fprintf(stderr, "not enough memory (realloc returned NULL)\n");
		exit(EXIT_FAILURE);
	}

	memcpy(&(mem->text[mem->size]), ptr, realsize);
	mem->size += realsize;
	mem->text[mem->size] = 0;

	return realsize;
}

// read_callback
// http://curl.haxx.se/libcurl/c/post-callback.html
size_t read_callback(void *ptr, size_t size, size_t nmemb, void *userp) {
	struct WriteBuffer *pooh = (struct WriteBuffer *) userp;
	size_t sendSize = size * nmemb;
	
	if (sendSize < (size_t)1)
		return 0;

	if (pooh->sizeleft == 0)
	{
		return 0;	/* no more data left to deliver */
	}

	/* the last peace of data */
	if (pooh->sizeleft <= (int)sendSize)
	{
		size_t tmpRet = pooh->sizeleft;
		memcpy(ptr, pooh->readptr, pooh->sizeleft);
		pooh->readptr += pooh->sizeleft;
		pooh->sizeleft = 0;
		return tmpRet;
	}

	/* still have data to full-fill the buffer */
	memcpy(ptr, pooh->readptr, sendSize);
	pooh->readptr += sendSize;
	pooh->sizeleft -= sendSize;

	return sendSize;	
#if 0

	if (pooh->sizeleft) {
		*(char *) ptr = pooh->readptr[0]; /* copy one single byte */
		pooh->readptr++; /* advance pointer */
		pooh->sizeleft--; /* less data left */
		return 1; /* we return 1 byte at a time! */
	}

	return 0; 	
#endif
}

size_t header_callback(void *data, size_t blockSize, size_t numBlocks, void *userPtr) {
    map<string, string>* headers = reinterpret_cast<map<string, string> *>(userPtr);
    string header(reinterpret_cast<char*>(data), blockSize * numBlocks);
    if (header.find(":") == string::npos)
    {
        return blockSize * numBlocks;
    }
    string key;
    stringstream ss(header);
    if (getline(ss, key, ':')) { 
        if (trim(key)!= "") {

            string value;
            getline(ss, value);
            (*headers)[key] = trim(value);
        }
    }
    return blockSize * numBlocks;
}

// homegrown timeout mechanism
int curl_enhanced_progress(void *clientp, double dltotal, double dlnow,
		double ultotal, double ulnow) {
	CURL* curl = static_cast<CURL*>(clientp);

	time_t now = time(0);
	progress_t p(dlnow, ulnow);

	pthread_mutex_lock(&curl_handles_lock);

	// any progress?
	if (p != curl_progress[curl]) {
		// yes!
		curl_times[curl] = now;
		curl_progress[curl] = p;
	} else {
		// timeout?
		if (now - curl_times[curl] > readwrite_timeout) {
			pthread_mutex_unlock(&curl_handles_lock);

			log_error("timeout  now: %li  curl_times[curl]: %lil  readwrite_timeout: %li",
					   (long int) now, curl_times[curl], (long int) readwrite_timeout);	

			return CURLE_ABORTED_BY_CALLBACK;
		}
	}

	pthread_mutex_unlock(&curl_handles_lock);
	return 0;
}

void locate_bundle(void) {
	// See if environment variable CURL_CA_BUNDLE is set
	// if so, check it, if it is a good path, then set the
	// curl_ca_bundle variable to it
	char *CURL_CA_BUNDLE;
	if (curl_ca_bundle.size() == 0) {
		CURL_CA_BUNDLE = getenv("CURL_CA_BUNDLE");
		if (CURL_CA_BUNDLE != NULL) {
			// check for existance and readability of the file
			ifstream BF(CURL_CA_BUNDLE);
			if (BF.good()) {
				BF.close();
				curl_ca_bundle.assign(CURL_CA_BUNDLE);
			} else {
				fprintf(stderr,
						"%s: file specified by CURL_CA_BUNDLE environment variable is not readable\n",
						program_name.c_str());
				exit(EXIT_FAILURE);
			}

			return;
		}
	}

	// not set via environment variable, look in likely locations

	///////////////////////////////////////////
	// from curl's (7.21.2) acinclude.m4 file
	///////////////////////////////////////////
	// dnl CURL_CHECK_CA_BUNDLE
	// dnl -------------------------------------------------
	// dnl Check if a default ca-bundle should be used
	// dnl
	// dnl regarding the paths this will scan:
	// dnl /etc/ssl/certs/ca-certificates.crt Debian systems
	// dnl /etc/pki/tls/certs/ca-bundle.crt Redhat and Mandriva
	// dnl /usr/share/ssl/certs/ca-bundle.crt old(er) Redhat
	// dnl /usr/local/share/certs/ca-root.crt FreeBSD
	// dnl /etc/ssl/cert.pem OpenBSD
	// dnl /etc/ssl/certs/ (ca path) SUSE
	ifstream BF("/etc/pki/tls/certs/ca-bundle.crt");
	if (BF.good()) {
		BF.close();
		curl_ca_bundle.assign("/etc/pki/tls/certs/ca-bundle.crt");
		return;
	}

	return;
}

