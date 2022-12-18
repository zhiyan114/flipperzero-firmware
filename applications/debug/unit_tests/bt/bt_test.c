#include <furi.h>
#include <furi_hal.h>
#include "../minunit.h"

#include "../../../../applications/services/bt/bt_service/bt_keys_storage.h"

#define BT_TEST_KEY_STORAGE_FILE_PATH ("/ext/bt_test.keys")
#define BT_TEST_NVM_RAM_BUFF_SIZE (507 * 4) // The same as in

#define TAG "BtUnitTest"

MU_TEST(test_bt_keys_storage) {
    FURI_LOG_I(TAG, "Bt tests");
    BtKeysStorage* instance = bt_keys_storage_alloc();
    // Change default key storage file path
    bt_keys_storage_set_file_path(instance, BT_TEST_KEY_STORAGE_FILE_PATH);
    // Allocate and set nvm buffer
    uint8_t* nvm_ram_buff = malloc(BT_TEST_NVM_RAM_BUFF_SIZE);
    bt_keys_storage_set_ram_params(instance, nvm_ram_buff, BT_TEST_NVM_RAM_BUFF_SIZE);
    // Emulate connection
    for(size_t i = 0; i < 88; i++) {
        nvm_ram_buff[i] = i;
    }

}

MU_TEST_SUITE(test_bt) {
    MU_RUN_TEST(test_bt_keys_storage);
}

int run_minunit_test_bt() {
    MU_RUN_SUITE(test_bt);
    return MU_EXIT_CODE;
}
