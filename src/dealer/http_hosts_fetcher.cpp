//
// Copyright (C) 2011-2012 Rim Zaidullin <creator@bash.org.ru>
//
// Licensed under the BSD 2-Clause License (the "License");
// you may not use this file except in compliance with the License.
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//

#include <stdexcept>
#include <iostream>

#include <boost/current_function.hpp>
#include <boost/tokenizer.hpp>

#include "cocaine/dealer/heartbeats/http_hosts_fetcher.hpp"

namespace cocaine {
namespace dealer {

http_hosts_fetcher::http_hosts_fetcher(service_info_t service_info) :
	curl_(NULL),
	service_info_(service_info)
{
	curl_ = curl_easy_init();
}

http_hosts_fetcher::~http_hosts_fetcher() {
	curl_easy_cleanup(curl_);
}

int
http_hosts_fetcher::curl_writer(char* data, size_t size, size_t nmemb, std::string* buffer_in) {
	if (buffer_in != NULL) {
		buffer_in->append(data, size * nmemb);
		return size * nmemb;
	}

	return 0;
}

void
http_hosts_fetcher::get_hosts(std::vector<host_info_t>& hosts, service_info_t& service_info) {
	CURLcode result = CURLE_OK;
	char error_buffer[CURL_ERROR_SIZE];
	std::string buffer;
	
	if (curl_) {
		curl_easy_setopt(curl_, CURLOPT_ERRORBUFFER, error_buffer);
		curl_easy_setopt(curl_, CURLOPT_URL, service_info_.hosts_url_.c_str());
		curl_easy_setopt(curl_, CURLOPT_HEADER, 0);
		curl_easy_setopt(curl_, CURLOPT_FOLLOWLOCATION, 1);
		curl_easy_setopt(curl_, CURLOPT_WRITEFUNCTION, curl_writer);
		curl_easy_setopt(curl_, CURLOPT_WRITEDATA, &buffer);

		// Attempt to retrieve the remote page
		result = curl_easy_perform(curl_);
	}
	
	if (CURLE_OK != result) {
		return;
	}
	
	long response_code = 0;
	result = curl_easy_getinfo(curl_, CURLINFO_RESPONSE_CODE, &response_code);
	
	if (CURLE_OK != result || response_code != 200) {
		return;
	}
	
	// get hosts from received data
	typedef boost::tokenizer<boost::char_separator<char> > tokenizer;
	boost::char_separator<char> sep("\n");
	tokenizer tokens(buffer, sep);
	
	for (tokenizer::iterator tok_iter = tokens.begin(); tok_iter != tokens.end(); ++tok_iter) {
			host_info_t host(*tok_iter);
			hosts.push_back(host);
	}
	
	service_info = service_info_;
}

} // namespace dealer
} // namespace cocaine