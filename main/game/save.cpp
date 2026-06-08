#include "save.h"

#include "esp_spiffs.h"
#include "esp_log.h"
#include <cstdio>
#include <cstring>

static const char* TAG = "SAVE";
static bool s_initialized = false;

static uint32_t calc_crc(const uint8_t* data, int len) {
    uint32_t crc = 0xFFFFFFFF;
    for (int i = 0; i < len; i++) {
        crc ^= data[i];
        for (int j = 0; j < 8; j++) {
            crc = (crc >> 1) ^ (0xEDB88320 & (-(crc & 1)));
        }
    }
    return ~crc;
}

bool save_init(void) {
    esp_vfs_spiffs_conf_t conf = {
        .base_path = "/spiffs",
        .partition_label = "assets",
        .max_files = 5,
        .format_if_mount_failed = true,
    };

    esp_err_t err = esp_vfs_spiffs_register(&conf);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "SPIFFS mount failed: %s", esp_err_to_name(err));
        return false;
    }

    size_t total = 0, used = 0;
    esp_spiffs_info("assets", &total, &used);
    ESP_LOGI(TAG, "SPIFFS mounted: total=%d used=%d", total, used);

    s_initialized = true;
    return true;
}

static void get_path(int slot, char* buf, int buf_len) {
    snprintf(buf, buf_len, "/spiffs/save%d.dat", slot);
}

void save_compute_crc(SaveData* data) {
    data->crc = 0;
    data->crc = calc_crc((const uint8_t*)data, sizeof(SaveData));
}

bool save_verify_crc(const SaveData* data) {
    SaveData copy = *data;
    uint32_t stored = copy.crc;
    copy.crc = 0;
    uint32_t computed = calc_crc((const uint8_t*)&copy, sizeof(SaveData));
    return stored == computed;
}

bool save_exists(int slot) {
    if (!s_initialized || slot < 0 || slot >= SAVE_MAX_SLOTS) return false;
    char path[64];
    get_path(slot, path, sizeof(path));
    FILE* f = fopen(path, "rb");
    if (!f) return false;
    fclose(f);
    return true;
}

bool save_load(int slot, SaveData* data) {
    if (!s_initialized || slot < 0 || slot >= SAVE_MAX_SLOTS) return false;

    char path[64];
    get_path(slot, path, sizeof(path));

    FILE* f = fopen(path, "rb");
    if (!f) {
        ESP_LOGW(TAG, "Save slot %d not found", slot);
        return false;
    }

    size_t read = fread(data, 1, sizeof(SaveData), f);
    fclose(f);

    if (read != sizeof(SaveData)) {
        ESP_LOGE(TAG, "Save slot %d: read %d bytes, expected %d", slot, read, sizeof(SaveData));
        return false;
    }

    if (data->magic != SAVE_MAGIC) {
        ESP_LOGE(TAG, "Save slot %d: bad magic 0x%08X", slot, data->magic);
        return false;
    }

    if (!save_verify_crc(data)) {
        ESP_LOGE(TAG, "Save slot %d: CRC mismatch", slot);
        return false;
    }

    ESP_LOGI(TAG, "Loaded save slot %d (level=%d score=%d)", slot, data->current_level, data->score);
    return true;
}

bool save_write(int slot, const SaveData* data) {
    if (!s_initialized || slot < 0 || slot >= SAVE_MAX_SLOTS) return false;

    SaveData to_write = *data;
    to_write.magic = SAVE_MAGIC;
    to_write.version = SAVE_VERSION;
    save_compute_crc(&to_write);

    char path[64];
    get_path(slot, path, sizeof(path));

    FILE* f = fopen(path, "wb");
    if (!f) {
        ESP_LOGE(TAG, "Failed to open %s for writing", path);
        return false;
    }

    size_t written = fwrite(&to_write, 1, sizeof(SaveData), f);
    fclose(f);

    if (written != sizeof(SaveData)) {
        ESP_LOGE(TAG, "Write incomplete: %d / %d", written, sizeof(SaveData));
        return false;
    }

    ESP_LOGI(TAG, "Saved slot %d (level=%d score=%d)", slot, to_write.current_level, to_write.score);
    return true;
}
