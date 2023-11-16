# DumpSteamCollections
C++ program for Linux to dump Steam Collection information JSON from LevelDB. This program will print the raw, unformatted JSON, which can be further processed with other tools such as `jq`.

More than simply collection information alone will be output, so you can filter as you need to.

## Overview
### LevelDB
Steam stores information about Collections (or "Categories") in a LevelDB. Internally, collection names are identified by Steam using either the LevelDB key for these collections, or for legacy tags from before the Library Update, `from-tag-tagname`, where `tagname` is the name of the tag.

So while a collection might have a name like `"Action Games"`, internally Steam will identify it as `"uc-xxxxxxxx"`, where `xxxxxxxx` is a random sequence of characters.

In the LevelDB, Steam stores collection information under a key formatted like `"https://steamloopback.host...U...-cloud-storage-namespace"`, where `'...'` means there could be any characters here. The first `'...'` is usually some unprintable character or sequence of characters, and the last `'...'` corresponds to the Steam User ID. The User ID refers to the ID of the Steam user who owns these collections. For example, you could have multiple users logged into Steam at once, but each would have their own User ID.

This key stores a JSON value, and this value contains lots of information. Primarily it is a JSON list of lists, and these inner-lists contain JSON objects representing various information about Steam collections and ajacently-related items, such as which news elements to show or hide in the category.

The inner-lists containing objects which refer to a collection start with the collection name, `user-collections.uc-xxxxxxxx` or `user-collections.from-tag-tagname`, and then are followed in this list by an object containing information about this collection. However, to know the corresponding collection name that this key represents, you only want the lists that contain objects with a `value` section.

Below is an example of the type of object that has the value we want:
```json
  [
    "user-collections.uc-zGsgCL84GNzg",
    {
      "key": "user-collections.uc-zGsgCL84GNzg",
      "timestamp": 1697927286,
      "value": "{\"id\":\"uc-zGsgCL84GNzg\",\"name\":\"Touhou\",\"added\":[1584090,1100140,851100],\"removed\":[]}",
      "conflictResolutionMethod": "custom",
      "strMethodId": "union-collections",
      "version": "3086"
    }
  ],
```

From here, the `value` section can be parsed as JSON, and the `id` and `name` fields can be related accordingly.

### Uses for LevelDB Collection Key Name
Steam stores information about which Collections a Non-Steam Game is a part of in `/path/to/Steam/userdata/<userid>/config/localconfig.vdf`, under the `user-collections` key. This is a stringified JSON, and uses the LevelDB Collection Key name to identify collections. If you want to manually add Non-Steam Games to a Collection, you would have to know this key name, and there is no straightforward way which I am aware of to get this key name without parsing the LevelDB directly.

Getting this key name was the primary motivation for writing this program, as I could not find a viable way to parse the LevelDB without using some LevelDB implementation directly.

## Dependencies
To build this program, you will need `g++` and `leveldb`.
- `g++`
- `leveldb`

Often `leveldb` is available from your distributions repositories. On Arch Linux, it can be installed from the official repos with `pacman -S leveldb`. 

## Compiling
Currently no pre-built releases are available, the program has to be compiled manually. Please note that this program is **only for GNU/Linux distributions**.

A small script has been provided to build the program, which runs `g++` to create the executable, and strips it to reduce binary size. However, you're free to compile manually if you want.

## Usage
Compile the C++ code with `sh build.sh` and run `./dumpsteamcollections`. Optionally, you can provide a Steam User ID to search for, such as `./dumpsteamcollections 123456789`.

From here, the output can be parsed with a tool such as [JQ](https://github.com/jqlang/jq) on the commandline. Some examples:

```bash
# Get collections JSON for default Steam user (ideal if you only have one Steam User logged in)
./dumpsteamcollections

# Get collections JSON for a specific Steam User ID
# a list can be found at your Steam install path, i.e. $HOME/.local/share/Steam/config/loginusers.vdf
./dumpsteamcollections 123456789

# Create file with formatted JSON
./dumpsteamcollections | jq '.' > formatted_collections.json

# Output only Steam Collection information
./dumpsteamcollections | jq -r '.[][1] | select(.key | startswith("user-collections")) | select (.value != null) | .value | fromjson | tojson'

# Find Steam Collection Key from name (i.e. return uc-xxxxxxxx from 'Action Games')
## Function to find the Steam Collection Key by name from the JSON file using jq
function findSteamCollectionKeyFromName {
  findname="$1"
  collections_json="$2"
  while read -r collection_object; do
      collection_name="$( echo "${collection_object}" | jq -r '.name' )"
      collection_id="$( echo "${collection_object}" | jq -r '.id' )"
  
      if [ "${collection_name}" = "${findname}" ]; then
          echo "${collection_name} -> ${collection_id}"
          break
      fi
  done <<< "$( jq -r '.[][1] | select(.key | startswith("user-collections")) | select (.value != null) | .value | fromjson | tojson' "${collections_json}" )"
}

collections_dump_path="/tmp/collections_dump.json"
./dumpsteamcollections > "${collections_dump_path}"
findSteamCollectionKeyFromName "SEGA" "${collections_dump_path}"  # Find Key ID for collection named "SEGA"
```

## License
DumpSteamCollections is licensed under the terms of the MIT license. See LICENSE for more information.
