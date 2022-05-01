struct stepper_config_t {
    uint32_t ap_ip;           // 0x00 + 4
    uint32_t ap_nm;           // 0x04 + 4
    uint32_t sta_ip;          // 0x08 + 4
    uint32_t sta_gw;          // 0x0A + 4
    uint32_t sta_nm;          // 0x10 + 4
    uint8_t sta_enabled : 1;  // 0x14 + 1
    uint8_t sta_use_dhcp : 1;
    uint8_t sta_un3 : 1;
    uint8_t sta_un4 : 1;
    uint8_t sta_un5 : 1;
    uint8_t sta_un6 : 1;
    uint8_t sta_un7 : 1;
    uint8_t sta_un8 : 1;

    uint8_t ap_enabled : 1;  // 0x15 + 1
    uint8_t ap_un2 : 1;
    uint8_t ap_un3 : 1;
    uint8_t ap_un4 : 1;
    uint8_t ap_un5 : 1;
    uint8_t ap_un6 : 1;
    uint8_t ap_un7 : 1;
    uint8_t ap_un8 : 1;
    uint8_t padding[10];  // 0x16 + 10
    char ap_ssid[32];     // 0x20 + 32
    char sta_ssid[32];    // 0x40 + 32
    char ap_pass[64];     // 0x60 + 64
    char sta_pass[64];    // 0xA0 + 64
} __attribute__((packed));
static stepper_config_t stepper_config;
#define OCTA 24
#define OCTB 16
#define OCTC 8
#define OCTD 0
#define CONFIG_SIZE 260

void config_init() {
    EEPROM.begin(CONFIG_SIZE);
}

void config_default() {
    stepper_config.sta_enabled = 0;
    stepper_config.ap_enabled = 1;
    stepper_config.ap_ip = (10 << OCTA) + (255 << OCTB) + (255 << OCTC) + (1 << OCTD);
    stepper_config.ap_nm = (255 << OCTA) + (255 << OCTB) + (255 << OCTC) + (0 << OCTD);
    stepper_config.sta_ip = (0 << OCTA) + (0 << OCTB) + (0 << OCTC) + (0 << OCTD);
    stepper_config.sta_gw = (0 << OCTA) + (0 << OCTB) + (0 << OCTC) + (0 << OCTD);
    stepper_config.sta_nm = (255 << OCTA) + (255 << OCTB) + (255 << OCTC) + (0 << OCTD);
    snprintf(stepper_config.ap_ssid, 32, "stepper-%06x", ESP.getChipId());
    snprintf(stepper_config.ap_pass, 32, "stepper");
    stepper_config.sta_ssid[0] = 0;
    stepper_config.sta_pass[0] = 0;
}