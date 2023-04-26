#include <fstream>
#include <map>
#include <vector>

#define _REALLY_INCLUDE_SYS__SYSTEM_PROPERTIES_H_
#include <sys/_system_properties.h>

void property_override(char const prop[], char const value[], bool add = false) {
    auto pi = (prop_info *) __system_property_find(prop);

    if (pi != nullptr) {
        __system_property_update(pi, value, strlen(value));
    } else if (add) {
        __system_property_add(prop, strlen(prop), value, strlen(value));
    }
}

void property_override(const std::vector<std::string> &props, char const value[], bool add = false) {
    for (const auto &prop : props) {
        property_override(prop.c_str(), value, add);
    }
}

std::string get_config_filename() {
    std::string filename = "/system/etc/ih8sn.conf";
    const char* prop_names[] = {"ro.build.product", "ro.build.model", "ro.boot.serialno"};

    for (auto& prop_name : prop_names) {
        auto prop = (prop_info*) __system_property_find(prop_name);
        if (prop != nullptr) {
            char prop_value[PROP_VALUE_MAX];
            __system_property_read(prop, nullptr, prop_value);

            // Replace spaces with underscores in prop_value, except at the end
            for (size_t i = 0; i < strlen(prop_value) - 1; ++i) {
                if (prop_value[i] == ' ' && prop_value[i + 1] != ' ') {
                    prop_value[i] = '_';
                }
            }

            // Check for file with underscores in prop_value
            std::string prop_filename = filename + "." + prop_value;
            if (std::ifstream(prop_filename)) {
                return prop_filename;
            }

            // Check for file with spaces in prop_value
            prop_filename = filename + "." + prop_value;
            std::replace(prop_filename.begin(), prop_filename.end(), '_', ' ');
            if (std::ifstream(prop_filename)) {
                return prop_filename;
            }
        }
    }

    // If no file with spaces or underscores was found, return the original filename
    return filename;
}

std::map<std::string, std::string> load_config() {
    std::map<std::string, std::string> config;

    if (std::ifstream file(get_config_filename()); file.good()) {
        std::string line;

        while (std::getline(file, line)) {
            if (line[0] == '#') {
                continue;
            }

            if (const auto separator = line.find('='); separator != std::string::npos) {
                config[line.substr(0, separator)] = line.substr(separator + 1);
            }
        }
    }

    return config;
}

std::vector<std::string> property_list(const std::string &prefix, const std::string &suffix) {
    std::vector<std::string> props;

    for (const std::string &part : {
        "",
        "boot.",
        "bootimage.",
        "odm_dlkm.",
        "odm.",
        "oem.",
        "product.",
        "system_dlkm.",
        "system_ext.",
        "system.",
        "vendor_dlkm.",
        "vendor.",
        "vendor.boot.",
    }) {
        props.emplace_back(prefix + part + suffix);
    }

    return props;
}

int main(int argc, char *argv[]) {
    if (__system_properties_init()) {
        return -1;
    }

    if (argc != 2) {
        return -1;
    }

    const auto is_init_stage = strcmp(argv[1], "init") == 0;
    const auto is_boot_completed_stage = strcmp(argv[1], "boot_completed") == 0;

    const auto config = load_config();
    const auto build_fingerprint = config.find("BUILD_FINGERPRINT");
    const auto build_description = config.find("BUILD_DESCRIPTION");
    const auto build_security_patch_date = config.find("BUILD_SECURITY_PATCH_DATE");
    const auto vendor_security_patch_date = config.find("VENDOR_SECURITY_PATCH_DATE");
    const auto build_tags = config.find("BUILD_TAGS");
    const auto build_type = config.find("BUILD_TYPE");
    const auto build_version_release = config.find("BUILD_VERSION_RELEASE");
    const auto build_version_release_or_codename = config.find("BUILD_VERSION_RELEASE_OR_CODENAME");
    const auto debuggable = config.find("DEBUGGABLE");
    const auto force_basic_attestation = config.find("FORCE_BASIC_ATTESTATION");
    const auto manufacturer_name = config.find("MANUFACTURER_NAME");
    const auto product_first_api_level = config.find("PRODUCT_FIRST_API_LEVEL");
    const auto product_device = config.find("PRODUCT_DEVICE");
    const auto product_name = config.find("PRODUCT_NAME");
    const auto product_model = config.find("PRODUCT_MODEL");
    const auto product_brand = config.find("PRODUCT_BRAND");

    if (is_init_stage && build_fingerprint != config.end()) {
        property_override(property_list("ro.", "build.fingerprint"),
                build_fingerprint->second.c_str());
    }

    if (is_init_stage && build_tags != config.end()) {
        property_override(property_list("ro.", "build.tags"), build_tags->second.c_str());
    }

    if (is_init_stage && build_type != config.end()) {
        property_override(property_list("ro.", "build.type"), build_type->second.c_str());
    }

    if (is_boot_completed_stage && build_version_release != config.end()) {
        property_override(property_list("ro.", "build.version.release"),
                build_version_release->second.c_str());
    }

    if (is_boot_completed_stage && build_version_release_or_codename != config.end()) {
        property_override(property_list("ro.", "build.version.release_or_codename"),
                build_version_release_or_codename->second.c_str());
    }

    if (is_init_stage && build_description != config.end()) {
        property_override("ro.build.description", build_description->second.c_str());
    }

    if (is_boot_completed_stage && build_security_patch_date != config.end()) {
        property_override("ro.build.version.security_patch",
                build_security_patch_date->second.c_str());
    }

    if (is_boot_completed_stage && vendor_security_patch_date != config.end()) {
        property_override("ro.vendor.build.security_patch",
                vendor_security_patch_date->second.c_str());
    }

    if (is_boot_completed_stage && product_first_api_level != config.end()) {
        property_override("ro.product.first_api_level",
                product_first_api_level->second.c_str());
    }

    if (is_init_stage && debuggable != config.end()) {
        property_override("ro.debuggable", debuggable->second.c_str());
    }

    if (is_init_stage) {
        property_override("ro.secure", "1");
    }

    if (is_init_stage && manufacturer_name != config.end()) {
        property_override(property_list("ro.product.", "manufacturer"),
                manufacturer_name->second.c_str());
    }

    if (is_init_stage && product_brand != config.end()) {
        property_override(property_list("ro.product.", "brand"), product_brand->second.c_str());
    }

    if (is_init_stage && product_name != config.end()) {
        property_override(property_list("ro.product.", "name"), product_name->second.c_str());
    }

    if (is_init_stage && product_device != config.end()) {
        property_override(property_list("ro.product.", "device"), product_device->second.c_str());
    }

    if (is_init_stage && force_basic_attestation != config.end() && force_basic_attestation->second == "1") {
        const auto& product = product_model != config.end() ? product_model : product_device;
        if (product != config.end()) {
            std::string model = product->second + " ";
            property_override(property_list("ro.product.", "model"), model.c_str());
        } else {
            auto pi = (prop_info *) __system_property_find("ro.product.model");
            if (pi != nullptr) {
                char value[PROP_VALUE_MAX];
                __system_property_read(pi, nullptr, value);
                strcat(value, " ");
                property_override(property_list("ro.product.", "model"), value);
            }
        }
    } else if (is_init_stage && product_model != config.end()) {
        property_override(property_list("ro.product.", "model"), product_model->second.c_str());
    }

    if (is_boot_completed_stage) {
        property_override("ro.boot.flash.locked", "1");
        property_override("ro.boot.vbmeta.device_state", "locked");
        property_override("vendor.boot.vbmeta.device_state", "locked");
        property_override("ro.boot.verifiedbootstate", "green");
        property_override("vendor.boot.verifiedbootstate", "green");
        property_override("ro.boot.veritymode", "enforcing");
        property_override("ro.is_ever_orange", "0");
        property_override(property_list("ro.", "warranty_bit"), "0");
    }

    return 0;
}
