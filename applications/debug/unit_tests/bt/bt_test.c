#include <furi.h>
#include <furi_hal.h>
#include "../minunit.h"

#include "../../../../applications/services/bt/bt_service/bt_keys_storage.h"
#include <storage/storage.h>

#define BT_TEST_KEY_STORAGE_FILE_PATH EXT_PATH("unit_tests/bt_test.keys")
#define BT_TEST_NVM_RAM_BUFF_SIZE (507 * 4) // The same as in ble NVM storage

#define TAG "BtUnitTest"

MU_TEST(test_bt_keys_storage) {
    Storage* storage = furi_record_open(RECORD_STORAGE);
    BtKeysStorage* instance = bt_keys_storage_alloc();
    // Change default key storage file path
    bt_keys_storage_set_file_path(instance, BT_TEST_KEY_STORAGE_FILE_PATH);
    // Allocate and set nvm buffer
    uint8_t* nvm_ram_buff_dut = malloc(BT_TEST_NVM_RAM_BUFF_SIZE);
    uint8_t* nvm_ram_buff_ref = malloc(BT_TEST_NVM_RAM_BUFF_SIZE);
    bt_keys_storage_set_ram_params(instance, nvm_ram_buff_dut, BT_TEST_NVM_RAM_BUFF_SIZE);
    // Emulate nvm change on initial connection
    const int nvm_change_size_on_connection = 88;
    for(size_t i = 0; i < nvm_change_size_on_connection; i++) {
        nvm_ram_buff_dut[i] = rand();
        nvm_ram_buff_ref[i] = nvm_ram_buff_dut[i];
    }
    // Emulate update storage on initial connect
    mu_assert(
        bt_keys_storage_update(
            instance, BtProfileSerial, nvm_ram_buff_dut, nvm_change_size_on_connection),
        "Failed to update key storage on initial connect");
    mu_assert(
        storage_common_stat(storage, BT_TEST_KEY_STORAGE_FILE_PATH, NULL) == FSE_OK,
        "Failed in creating keys file");
    memset(nvm_ram_buff_dut, 0, BT_TEST_NVM_RAM_BUFF_SIZE);
    mu_assert(_bt_keys_storage_load(instance, BtProfileSerial), "Failed to load NVM");
    mu_assert(
        memcmp(nvm_ram_buff_ref, nvm_ram_buff_dut, nvm_change_size_on_connection) == 0,
        "Wrong buffer loaded");

    const int nvm_disconnect_update_offset = 84;
    const int nvm_disconnect_update_size = 324;
    const int nvm_total_size = nvm_change_size_on_connection -
                               (nvm_change_size_on_connection - nvm_disconnect_update_offset) +
                               nvm_disconnect_update_size;
    // Emulate update storage on initial disconnect
    for(size_t i = nvm_disconnect_update_offset;
        i < nvm_disconnect_update_offset + nvm_disconnect_update_size;
        i++) {
        nvm_ram_buff_dut[i] = rand();
        nvm_ram_buff_ref[i] = nvm_ram_buff_dut[i];
    }
    mu_assert(
        bt_keys_storage_update(
            instance,
            BtProfileSerial,
            &nvm_ram_buff_dut[nvm_disconnect_update_offset],
            nvm_disconnect_update_size),
        "Failed to update key storage on initial disconnect");
    memset(nvm_ram_buff_dut, 0, BT_TEST_NVM_RAM_BUFF_SIZE);
    mu_assert(_bt_keys_storage_load(instance, BtProfileSerial), "Failed to load NVM");
    mu_assert(
        memcmp(nvm_ram_buff_ref, nvm_ram_buff_dut, nvm_total_size) == 0, "Wrong buffer loaded");

    free(nvm_ram_buff_dut);
    free(nvm_ram_buff_ref);

    bt_keys_storage_free(instance);

    furi_record_close(RECORD_STORAGE);
}

MU_TEST_SUITE(test_bt) {
    MU_RUN_TEST(test_bt_keys_storage);
}

int run_minunit_test_bt() {
    MU_RUN_SUITE(test_bt);
    return MU_EXIT_CODE;
}
