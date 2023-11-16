#include <filesystem>
#include <format>
#include <iostream>

#include <stdlib.h>

#include <leveldb/db.h>


// TODO refactor to use std::filesystem::path instead of std::format?


// Valid Steam path is one that exists and has a libraryfolders.vdf and config.vdf (excludes empty installs)
bool is_valid_steam_path(std::string steam_path)
{
    std::string config_vdf_path = std::format("{}/config/libraryfolders.vdf", steam_path);
    std::string libraryfolders_vdf_path = std::format("{}/config/libraryfolders.vdf", steam_path);
    
    return (std::filesystem::exists(steam_path) && std::filesystem::exists(config_vdf_path) && std::filesystem::exists(libraryfolders_vdf_path));
}

// Return first known Steam path from list of common paths (includes symlinks)
std::string get_steam_install_path()
{
    const std::string home = getenv("HOME");
    // Possible paths where Steam for Linux may be installed
    const std::string potential_steam_paths[4] = {
        std::format("{}/.local/share/Steam", home),
        std::format("{}/.steam/root", home),
        std::format("{}/.steam/steam", home),
        std::format("{}/.steam/debian-installation", home)
    };
    
    std::string found_steam_path;
    for (std::string steam_path : potential_steam_paths)
    {
        if (is_valid_steam_path(steam_path))
        {
            found_steam_path = steam_path;
        }
    }

    return found_steam_path;
}

// Return first folder in steam_path's folder with a directory name length > 2 (skips '0' and 'ac')
std::string guess_steam_user_id(std::string steam_path)
{
    const std::string steam_userdata_path = std::format("{}/userdata", steam_path);
    
    std::string found_steam_userdata_dir;
    for (auto const& steam_userdata_dir : std::filesystem::directory_iterator(steam_userdata_path))
    {
        std::string steam_userdata_dirname = std::filesystem::path(steam_userdata_dir).filename();
        if (steam_userdata_dir.is_directory() && steam_userdata_dirname.length() > 2)
        {
            // Assume first match is correct steam user
            found_steam_userdata_dir = steam_userdata_dirname;
            break;
        }
    }
    
    return found_steam_userdata_dir;
}

int main(int argc, char *argv[])
{
    // Basic Steam constants
    const std::string home = getenv("HOME");
    const std::string steam_path = get_steam_install_path();
    const std::string steam_leveldb_path = std::format("{}/config/htmlcache/Local Storage/leveldb", steam_path);

    // Get Steam User ID to build known LevelDB key name containing collections information
    const std::string steam_user_id = argc >= 2 ? argv[1] : guess_steam_user_id(steam_path);
    const std::string steam_local_loopback_keys[2] = { "https://steamloopback.host", "U" + steam_user_id + "-cloud-storage-namespace" };
    
    // Exit if we can't find Steam install location
    if (steam_path.length() <= 0)
    {
        std::cout << "Could not find Steam install path, exiting...";
        return 1;
    }
    
    // Initialize LevelDB stuff
    leveldb::DB* steam_db;
    leveldb::Options steam_db_options;
    steam_db_options.create_if_missing = false;
    leveldb::Status steam_db_status = leveldb::DB::Open(steam_db_options, steam_leveldb_path, &steam_db);
    
    // TODO perhaps there is a way to check for leveldb lock?
    if (!steam_db_status.ok())
    {
        std::cout << "Could not connect to DB! Maybe Steam is running?" << std::endl;
        return 1;
    }
    
    // Iterate over LevelDB until we find a key matching the known one containing collections information
    leveldb::Iterator* steam_db_iterator = steam_db->NewIterator(leveldb::ReadOptions());
    for (steam_db_iterator->SeekToFirst(); steam_db_iterator->Valid(); steam_db_iterator->Next())
    {
        // Iterate until we find key containing known LevelDB key name containing collections
        std::string current_key = steam_db_iterator->key().ToString();
        if ( (current_key.find(steam_local_loopback_keys[0]) != std::string::npos) && (current_key.find(steam_local_loopback_keys[1]) != std::string::npos) )
        {
            // Strip any characters preceding opening bracket
            std::string steam_db_collections_info = steam_db_iterator->value().ToString();
            steam_db_collections_info = steam_db_collections_info.substr(steam_db_collections_info.find("["));
            
            std::cout << steam_db_collections_info << std::endl;
            break;
        }
    }

    delete steam_db_iterator;
    delete steam_db;
    
    return 0;
}
