#define PENGINE_CONFIG_KEY_SIZE 16

#define PENGINE_CONFIG_KEY___APP_NAME "AAAAAAAAAAAAAAAA"

struct ConfigEntry {
    char* id;
    char* val;
    int val_len;
    struct ConfigEntry* next;
};

/*
 * Initializes the config for the game / engine settings which may change per use case.
 *
 */

void init_config();
/*
 * Loads config from a specified path
 * @param config_path the relative path string of the config file to load
 *
 */
void load_config(char* config_path);

/*
 * fetch an entry from the loaded config
 * @param key string key of config entry
 * @param size_out out-var of value size 
 */
char* get_config_entry(char* key, int* size_out);

/*
 * initializes the config values as defaults for initial file write
 *
 */
void init_config_from_memory();
