#include <boost/filesystem/fstream.hpp>

#include "persistance.hpp"

using namespace yappi::persistance;
namespace fs = boost::filesystem;

file_storage_t::file_storage_t(const std::string& storage_path):
    m_storage_path(storage_path)
{
    if(!fs::exists(m_storage_path)) {
        try {
            fs::create_directory(m_storage_path);
        } catch(const std::runtime_error& e) {
            throw std::runtime_error("file storage path " + m_storage_path.string() + " is unavailable");
        }
    }
}

bool file_storage_t::put(const std::string& key, const Json::Value& value) {
    Json::StyledStreamWriter writer;
    fs::path filepath = m_storage_path / key;
    fs::ofstream stream;
   
    stream.exceptions(std::ofstream::badbit | std::ofstream::failbit);

    try {
        stream.open(filepath, std::ofstream::out | std::ofstream::trunc);
    } catch(const fs::ofstream::failure& e) {
        syslog(LOG_ERR, "failed to write %s", filepath.string().c_str());
        return false;
    }     

    writer.write(stream, value);
    stream.close();

    return true;
}

bool file_storage_t::exists(const std::string& key) const {
    fs::path filepath = m_storage_path / key;
    return fs::exists(filepath) && fs::is_regular(filepath);
}

Json::Value file_storage_t::get(const std::string& key) const {
    Json::Reader reader(Json::Features::strictMode());
    Json::Value root(Json::objectValue);
    fs::path filepath = m_storage_path / key;
    fs::ifstream stream;
     
    stream.exceptions(std::ofstream::badbit | std::ofstream::failbit);

    try {
        stream.open(filepath, std::ifstream::in);
    } catch(const fs::ifstream::failure& e) {
        return root;
    }

    if(!reader.parse(stream, root)) {
        syslog(LOG_ERR, "malformed object in %s: %s",
            filepath.string().c_str(), reader.getFormatedErrorMessages().c_str());
    }

    return root;
}

Json::Value file_storage_t::all() const {
    Json::Value root(Json::objectValue);
    Json::Reader reader(Json::Features::strictMode());

    fs::directory_iterator it(m_storage_path), end;

    while(it != end) {
        if(fs::is_regular(it->status())) {
            Json::Value value = get(it->leaf());

            if(!value.empty()) {
                root[it->leaf()] = value;
            }
        }

        ++it;
    }

    return root;
}

void file_storage_t::remove(const std::string& key) {
    fs::remove(m_storage_path / key);
}