/*
    Copyright (c) 2011-2012 Andrey Sibiryov <me@kobology.ru>
    Copyright (c) 2011-2012 Other contributors as noted in the AUTHORS file.

    This file is part of Cocaine.

    Cocaine is free software; you can redistribute it and/or modify
    it under the terms of the GNU Lesser General Public License as published by
    the Free Software Foundation; either version 3 of the License, or
    (at your option) any later version.

    Cocaine is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
    GNU Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public License
    along with this program. If not, see <http://www.gnu.org/licenses/>. 
*/

#include "cocaine/archive.hpp"
#include "cocaine/config.hpp"
#include "cocaine/context.hpp"
#include "cocaine/logging.hpp"

#include "cocaine/api/storage.hpp"

#include "cocaine/traits/json.hpp"

#include <boost/filesystem/fstream.hpp>
#include <boost/filesystem/operations.hpp>
#include <boost/filesystem/path.hpp>
#include <boost/program_options.hpp>

using namespace cocaine;

namespace po = boost::program_options;
namespace fs = boost::filesystem;

namespace {
    void
    list(context_t& context) {
        std::vector<std::string> apps;
      
        try {
            apps = api::storage(context, "core")->list("manifests");
        } catch(const storage_error_t& e) {
            std::cerr << "Error: unable to retrieve the app list." << std::endl;
            std::cerr << e.what() << std::endl;
            return;
        }

        std::cout << "Currently uploaded apps:" << std::endl;

        for(std::vector<std::string>::const_iterator it = apps.begin();
            it != apps.end();
            ++it)
        {
            std::cout << "\t" << *it << std::endl;
        }
    }

    void
    upload(context_t& context,
           const std::string& name,
           const fs::path& manifest_path,
           const fs::path& package_path)
    {
        std::string type,
                    compression;
        
        Json::Reader reader(Json::Features::strictMode());
        Json::Value manifest;

        fs::ifstream manifest_stream(manifest_path);

        if(!manifest_stream) {
            std::cerr << "Error: unable to open '" << manifest_path << "'." << std::endl;
            return;
        }
        
        if(!reader.parse(manifest_stream, manifest)) {
            std::cerr << "Error: the app manifest in '" << manifest_path << "' is corrupted." << std::endl;
            std::cerr << reader.getFormattedErrorMessages() << std::endl;
            return;
        }

        type = manifest.get("type", "").asString();

        if(type.empty()) {
            std::cerr << "Error: no app type has been specified in the manifest." << std::endl;
            return;
        }

        fs::ifstream package_stream(
            package_path,
            std::fstream::binary | std::fstream::in
        );

        if(!package_stream) {
            std::cerr << "Error: unable to open '" << package_path << "'." << std::endl;
            return;
        }

        std::stringstream buffer;
        buffer << package_stream.rdbuf();

        std::string blob(buffer.str());

        try {
            archive_t archive(context, blob);
            compression = archive.type();
        } catch(const archive_error_t& e) {
            std::cerr << "Error: the app package in '" << package_path << "' is corrupted." << std::endl;
            std::cerr << e.what() << std::endl;
            return;
        }

        std::cout << "Detected app type: '" << type 
                  << "', package compression: '" << compression
                  << "'." << std::endl;

        auto storage = api::storage(context, "core");

        try {
            storage->put("manifests", name, manifest);
            storage->put("apps", name, blob);
        } catch(const storage_error_t& e) {
            std::cerr << "Error: unable to upload the app." << std::endl;
            std::cerr << e.what() << std::endl;
            return;
        }

        std::cout << "The '" << name << "' app has been successfully uploaded." << std::endl;
    }

    void
    upload_profile(context_t& context,
                   const std::string& name,
                   const fs::path& manifest_path)
    {
        Json::Reader reader(Json::Features::strictMode());
        Json::Value manifest;

        fs::ifstream manifest_stream(manifest_path);

        if(!manifest_stream) {
            std::cerr << "Error: unable to open '" << manifest_path << "'." << std::endl;
            return;
        }
        
        if(!reader.parse(manifest_stream, manifest)) {
            std::cerr << "Error: the app manifest in '" << manifest_path << "' is corrupted." << std::endl;
            std::cerr << reader.getFormattedErrorMessages() << std::endl;
            return;
        }

        auto storage = api::storage(context, "core");

        try {
            storage->put("profiles", name, manifest);
        } catch(const storage_error_t& e) {
            std::cerr << "Error: unable to upload the profile." << std::endl;
            std::cerr << e.what() << std::endl;
            return;
        }

        std::cout << "The '" << name << "' profile has been successfully uploaded." << std::endl;
    }

    void
    remove(context_t& context,
           std::string name)
    {
        auto storage = api::storage(context, "core");

        try {
            storage->remove("manifests", name);
            storage->remove("apps", name);
        } catch(const storage_error_t& e) {
            std::cerr << "Error: unable to remove the app." << std::endl;
            std::cerr << e.what() << std::endl;
            return;
        }

        std::cout << "The '" << name << "' app has been successfully removed." << std::endl;
    }

    void
    cleanup(context_t& context,
          const std::string& name)
    {
        std::cout << "Cleaning up the '" << name << "' app." << std::endl;

        auto cache = api::storage(context, "cache");

        Json::Value manifest;

        try {
            manifest = cache->get<Json::Value>("manifests", name);
        } catch(const storage_error_t& e) {
            std::cerr << "Error: unable to clean up the app." << std::endl;
            std::cerr << e.what() << std::endl;
            return;
        }

        uint64_t filecount;

        try {
            filecount = fs::remove_all(
                fs::path(context.config.path.spool) / name
            );
        } catch(const fs::filesystem_error& e) {
            std::cerr << "Error: unable to remove app files." << std::endl;
            std::cerr << e.what() << std::endl;
            return;
        }

        std::cout << "Removed " << filecount << " file(s) from the app spool." << std::endl;
        
        try {
            // Remove the cached app manifest.
            cache->remove("manifests", name);
        } catch(const storage_error_t& e) {
            std::cerr << "Unable to clean up the app - " << std::endl;
            std::cerr << e.what() << std::endl;
        }
    }

    void
    purge(context_t& context) {
        auto cache = api::storage(context, "cache");
        
        std::vector<std::string> manifests(
            cache->list("manifests")
        );

        if(manifests.empty()) {
            std::cout << "No apps has been found in the cache." << std::endl;
            return;
        }

        std::cout << "Cleaning up " << manifests.size() << " app(s)." << std::endl;
        
        for(std::vector<std::string>::const_iterator it = manifests.begin();
            it != manifests.end();
            ++it)
        {
            cleanup(context, *it);
        }
    }
}

int
main(int argc, char * argv[]) {
    po::options_description general_options("General options"),
                            hidden_options,
                            combined_options;
    
    po::positional_options_description positional_options;
    po::variables_map vm;

    general_options.add_options()
        ("help,h", "show this message")
        ("version,v", "show version and build information")
        ("configuration,c", po::value<std::string>
            ()->default_value("/etc/cocaine/cocaine-default.conf"),
            "location of the configuration file")
        ("manifest,m", po::value<std::string>
            ()->default_value("manifest.json"),
            "location of the app manifest json or profile manifest json")
        ("package,p", po::value<std::string>
            ()->default_value("package.tar.gz"),
            "location of the app source package")
        ("name,n", po::value<std::string>(),
            "app name");

    hidden_options.add_options()
        ("operation", po::value<std::string>());

    positional_options.add("operation", 1);

    combined_options.add(general_options)
                    .add(hidden_options);

    try {
        po::store(
            po::command_line_parser(argc, argv).
                options(combined_options).
                positional(positional_options).
                run(),
            vm);
        po::notify(vm);
    } catch(const po::unknown_option& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return EXIT_FAILURE;
    } catch(const po::ambiguous_option& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return EXIT_FAILURE;
    }

    if(vm.count("help")) {
        std::cout << "Usage: " << argv[0] << " app:list|app:upload|profile:upload|app:remove|app:cleanup <options>" << std::endl;
        std::cout << general_options;
        return EXIT_SUCCESS;
    }

    if(vm.count("version")) {
        std::cout << "Cocaine " << COCAINE_VERSION << std::endl;
        return EXIT_SUCCESS;
    }

    // Validation
    // ----------

    if(!vm.count("operation")) {
        std::cerr << "Error: no operation has been specified." << std::endl;
        std::cerr << "Type '" << argv[0] << " --help' for usage information." << std::endl;
        return EXIT_FAILURE;
    }

    if(!vm.count("configuration")) {
        std::cerr << "Error: no configuration file location has been specified." << std::endl;
        std::cerr << "Type '" << argv[0] << " --help' for usage information." << std::endl;
        return EXIT_FAILURE;
    }

    // Startup
    // -------

    std::unique_ptr<context_t> context;

    try {
        context.reset(new context_t(vm["configuration"].as<std::string>(), "tools"));
    } catch(const std::exception& e) {
        std::cerr << "Error: unable to initialize the context - " << e.what() << std::endl;
        return EXIT_FAILURE;
    }

    std::string operation(vm["operation"].as<std::string>());

    if(operation == "app:list") {
        list(*context);
    } else if(operation == "app:upload") {
        if(!vm.count("manifest")) {
            std::cerr << "Error: no app manifest file location has been specified." << std::endl;
            std::cerr << "Type '" << argv[0] << " --help' for usage information." << std::endl;
            return EXIT_FAILURE;
        }
        
        if(!vm.count("package")) {
            std::cerr << "Error: no app package file location has been specified." << std::endl;
            std::cerr << "Type '" << argv[0] << " --help' for usage information." << std::endl;
            return EXIT_FAILURE;
        }

        if(!vm.count("name")) {
            std::cerr << "Error: no app name has been specified." << std::endl;
            std::cerr << "Type '" << argv[0] << " --help' for usage information." << std::endl;
            return EXIT_FAILURE;
        }
    
        upload(
            *context,
            vm["name"].as<std::string>(),
            vm["manifest"].as<std::string>(),
            vm["package"].as<std::string>()
        );
    } else if(operation == "profile:upload") {
        if(!vm.count("manifest")) {
            std::cerr << "Error: no profile manifest file location has been specified." << std::endl;
            std::cerr << "Type '" << argv[0] << " --help' for usage information." << std::endl;
            return EXIT_FAILURE;
        }
        
        if(!vm.count("name")) {
            std::cerr << "Error: no app name has been specified." << std::endl;
            std::cerr << "Type '" << argv[0] << " --help' for usage information." << std::endl;
            return EXIT_FAILURE;
        }

        upload_profile(
            *context,
            vm["name"].as<std::string>(),
            vm["manifest"].as<std::string>()
        );
    } else if(operation == "app:remove") {
        if(!vm.count("name")) {
            std::cerr << "Error: no app name has been specified." << std::endl;
            std::cerr << "Type '" << argv[0] << " --help' for usage information." << std::endl;
            return EXIT_FAILURE;
        }
    
        remove(
            *context,
            vm["name"].as<std::string>()
        );
    } else if(operation == "app:cleanup") {
        if(vm.count("name")) {
            cleanup(
                *context,
                vm["name"].as<std::string>()
            );
        } else {
            purge(*context);
        }
    } else {
        std::cerr << "Error: unknown operation has been specified" << std::endl;
        std::cerr << "Type '" << argv[0] << " --help' for usage information." << std::endl;
        return EXIT_FAILURE;
    }
}

