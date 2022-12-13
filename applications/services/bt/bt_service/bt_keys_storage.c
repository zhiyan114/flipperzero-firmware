#include "bt_keys_storage.h"

#include <furi.h>
#include <lib/toolbox/saved_struct.h>
#include <storage/storage.h>

#define BT_KEYS_STORAGE_PATH INT_PATH(BT_KEYS_STORAGE_FILE_NAME)
#define BT_KEYS_STORAGE_VERSION (0)
#define BT_KEYS_STORAGE_MAGIC (0x18)

typedef struct {
    uint32_t data_size;
    uint8_t* data;
} BtKeysStorageProfileKeys;

struct BtKeysStorage {
    uint8_t* nvm_sram_buff;
    uint16_t nvm_sram_buff_size;
    uint32_t nvm_flash_total_size;
    FuriString* file_path;
    BtKeysStorageProfileKeys profile_keys[BtProfileNum];
};

static bool bt_keys_storage_parse_profile_keys(
    BtKeysStorage* instance,
    uint8_t* saved_buff,
    size_t saved_buff_size) {
    size_t buff_i = 0;

    bool profile_keys_loaded = true;
    for(size_t i = 0; i < BtProfileNum; i++) {
        if(buff_i >= saved_buff_size) {
            profile_keys_loaded = false;
            break;
        }
        instance->profile_keys[i].data_size = *(uint32_t*)(saved_buff + buff_i);
        buff_i += sizeof(instance->profile_keys[i].data_size);
        if(instance->profile_keys[i].data_size == 0) continue;
        if(buff_i >= saved_buff_size) {
            profile_keys_loaded = false;
            break;
        }
        instance->profile_keys[i].data = &saved_buff[buff_i];
        buff_i += instance->profile_keys[i].data_size;
    }

    return profile_keys_loaded;
}

bool bt_keys_storage_load(Bt* bt) {
    furi_assert(bt);
    bool file_loaded = false;

    size_t payload_size = 0;
    if(saved_struct_get_payload_size(
           BT_KEYS_STORAGE_PATH, BT_KEYS_STORAGE_MAGIC, BT_KEYS_STORAGE_VERSION, &payload_size)) {
        FURI_LOG_I("BT KEY STORAGE", "Payload size: %d", payload_size);
    } else {
        FURI_LOG_E("BT KEY STORAGE", "Faileld to load payload size");
    }

    furi_hal_bt_get_key_storage_buff(&bt->bt_keys_addr_start, &bt->bt_keys_size);
    furi_hal_bt_nvm_sram_sem_acquire();
    file_loaded = saved_struct_load(
        BT_KEYS_STORAGE_PATH,
        bt->bt_keys_addr_start,
        bt->bt_keys_size,
        BT_KEYS_STORAGE_MAGIC,
        BT_KEYS_STORAGE_VERSION);
    furi_hal_bt_nvm_sram_sem_release();

    return file_loaded;
}

bool bt_keys_storage_save(Bt* bt) {
    furi_assert(bt);
    furi_assert(bt->bt_keys_addr_start);
    bool file_saved = false;

    furi_hal_bt_nvm_sram_sem_acquire();
    file_saved = saved_struct_save(
        BT_KEYS_STORAGE_PATH,
        bt->bt_keys_addr_start,
        bt->bt_keys_size,
        BT_KEYS_STORAGE_MAGIC,
        BT_KEYS_STORAGE_VERSION);
    furi_hal_bt_nvm_sram_sem_release();

    return file_saved;
}

bool bt_keys_storage_delete(Bt* bt) {
    furi_assert(bt);
    bool delete_succeed = false;
    bool bt_is_active = furi_hal_bt_is_active();

    furi_hal_bt_stop_advertising();
    delete_succeed = furi_hal_bt_clear_white_list();
    if(bt_is_active) {
        furi_hal_bt_start_advertising();
    }

    return delete_succeed;
}

BtKeysStorage* bt_keys_storage_alloc() {
    BtKeysStorage* instance = malloc(sizeof(BtKeysStorage));
    // Set nvm ram parameters
    furi_hal_bt_get_key_storage_buff(&instance->nvm_sram_buff, &instance->nvm_sram_buff_size);
    // Set key storage file
    instance->file_path = furi_string_alloc();
    furi_string_set_str(instance->file_path, BT_KEYS_STORAGE_PATH);

    return instance;
}

void bt_keys_storage_free(BtKeysStorage* instance) {
    furi_assert(instance);

    furi_string_free(instance->file_path);
    free(instance);
}

void bt_keys_storage_set_file_path(BtKeysStorage* instance, const char* path) {
    furi_assert(instance);
    furi_assert(path);

    furi_string_set_str(instance->file_path, path);
}

void bt_keys_storage_set_ram_params(BtKeysStorage* instance, uint8_t* buff, uint16_t size) {
    furi_assert(instance);
    furi_assert(buff);

    instance->nvm_sram_buff = buff;
    instance->nvm_sram_buff_size = size;
}

bool _bt_keys_storage_load(BtKeysStorage* instance, BtProfile profile) {
    furi_assert(instance);
    furi_assert(profile < BtProfileNum);

    bool loaded = false;
    do {
        // Get payload size
        size_t payload_size = 0;
        if(!saved_struct_get_payload_size(
               furi_string_get_cstr(instance->file_path),
               BT_KEYS_STORAGE_MAGIC,
               BT_KEYS_STORAGE_VERSION,
               &payload_size))
            break;

        // Load raw payload
        uint8_t* payload = malloc(payload_size);
        if(!saved_struct_load(
               furi_string_get_cstr(instance->file_path),
               payload,
               payload_size,
               BT_KEYS_STORAGE_MAGIC,
               BT_KEYS_STORAGE_VERSION)) {
            free(payload);
            break;
        }

        // Parse profile keys
        if(!bt_keys_storage_parse_profile_keys(instance, payload, payload_size)) {
            free(payload);
            break;
        }

        // Load key data to ram
        if(instance->profile_keys[profile].data_size > instance->nvm_sram_buff_size) {
            free(payload);
            break;
        }

        furi_hal_bt_nvm_sram_sem_acquire();
        memcpy(
            instance->nvm_sram_buff,
            instance->profile_keys[profile].data,
            instance->profile_keys[profile].data_size);
        furi_hal_bt_nvm_sram_sem_release();
        free(payload);

        loaded = true;
    } while(false);

    return loaded;
}

bool bt_keys_storage_update(BtKeysStorage* instance, uint32_t start_addr, uint32_t size) {
    furi_assert(instance);
    UNUSED(start_addr);
    UNUSED(size);

    bool updated = false;

    return updated;
}
