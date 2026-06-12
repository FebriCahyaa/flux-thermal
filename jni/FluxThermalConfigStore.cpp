/*
 * Copyright (C) 2024-2026 FebriCahyaa
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "FluxThermalConfigStore.hpp"

#include <fstream>

#include <rapidjson/error/en.h>
#include <rapidjson/filereadstream.h>
#include <rapidjson/prettywriter.h>
#include <rapidjson/stringbuffer.h>

// ── Load ─────────────────────────────────────────────────────────────────────

bool FluxThermalConfigStore::load_config(const std::string &path) {
    {
        std::lock_guard<std::mutex> lk(mutex_);
        config_path_ = path;
    }

    FILE *fp = fopen(path.c_str(), "rb");
    if (!fp) {
        LOGW_TAG("Config", "Config not found, creating default: {}", path);
        return create_default_config();
    }

    char buf[65536];
    rapidjson::FileReadStream is(fp, buf, sizeof(buf));
    rapidjson::Document doc;
    doc.ParseStream(is);
    fclose(fp);

    if (doc.HasParseError()) {
        LOGE_TAG("Config", "Parse error: {} @ offset {}",
                 rapidjson::GetParseError_En(doc.GetParseError()),
                 doc.GetErrorOffset());
        return create_default_config();
    }

    if (!doc.IsObject()) {
        LOGE_TAG("Config", "Root is not a JSON object");
        return create_default_config();
    }

    return parse_config(doc);
}

// ── Save ─────────────────────────────────────────────────────────────────────

bool FluxThermalConfigStore::save_config(const std::string &path) {
    std::lock_guard<std::mutex> lk(mutex_);

    rapidjson::Document doc;
    doc.SetObject();
    auto &alloc = doc.GetAllocator();

    // preferences
    rapidjson::Value pobj(rapidjson::kObjectType);
    pobj.AddMember("enabled",              config_.preferences.enabled,              alloc);
    pobj.AddMember("aggressive_mode",      config_.preferences.aggressive_mode,      alloc);
    pobj.AddMember("protect_battery",      config_.preferences.protect_battery,      alloc);
    pobj.AddMember("cool_down_on_charge",  config_.preferences.cool_down_on_charge,  alloc);
    pobj.AddMember("log_level",            config_.preferences.log_level,            alloc);
    pobj.AddMember("threshold_cool_mc",    config_.preferences.threshold_cool_mc,    alloc);
    pobj.AddMember("threshold_warm_mc",    config_.preferences.threshold_warm_mc,    alloc);
    pobj.AddMember("threshold_hot_mc",     config_.preferences.threshold_hot_mc,     alloc);
    pobj.AddMember("threshold_critical_mc",config_.preferences.threshold_critical_mc,alloc);
    doc.AddMember("preferences", pobj, alloc);

    // zone_filter
    rapidjson::Value zobj(rapidjson::kObjectType);
    rapidjson::Value ztypes(rapidjson::kArrayType);
    for (const auto &t : config_.zone_filter.include_types)
        ztypes.PushBack(rapidjson::Value(t.c_str(), alloc).Move(), alloc);
    zobj.AddMember("include_types", ztypes, alloc);
    doc.AddMember("zone_filter", zobj, alloc);

    rapidjson::StringBuffer sb;
    rapidjson::PrettyWriter<rapidjson::StringBuffer> w(sb);
    w.SetIndent(' ', 2);
    doc.Accept(w);

    FILE *fp = fopen(path.c_str(), "wb");
    if (!fp) {
        LOGE_TAG("Config", "Cannot open config for writing: {}", path);
        return false;
    }
    fwrite(sb.GetString(), 1, sb.GetSize(), fp);
    fclose(fp);
    return true;
}

// ── Parse ─────────────────────────────────────────────────────────────────────

bool FluxThermalConfigStore::parse_config(const rapidjson::Document &doc) {
    std::lock_guard<std::mutex> lk(mutex_);

    if (doc.HasMember("preferences") && doc["preferences"].IsObject()) {
        const auto &p = doc["preferences"];
        auto &prefs = config_.preferences;
#define GET_BOOL(field) if (p.HasMember(#field) && p[#field].IsBool()) prefs.field = p[#field].GetBool()
#define GET_INT(field)  if (p.HasMember(#field) && p[#field].IsInt())  prefs.field = p[#field].GetInt()
        GET_BOOL(enabled);
        GET_BOOL(aggressive_mode);
        GET_BOOL(protect_battery);
        GET_BOOL(cool_down_on_charge);
        GET_INT(log_level);
        GET_INT(threshold_cool_mc);
        GET_INT(threshold_warm_mc);
        GET_INT(threshold_hot_mc);
        GET_INT(threshold_critical_mc);
#undef GET_BOOL
#undef GET_INT
    }

    if (doc.HasMember("zone_filter") && doc["zone_filter"].IsObject()) {
        const auto &zf = doc["zone_filter"];
        if (zf.HasMember("include_types") && zf["include_types"].IsArray()) {
            config_.zone_filter.include_types.clear();
            for (const auto &v : zf["include_types"].GetArray()) {
                if (v.IsString())
                    config_.zone_filter.include_types.emplace_back(v.GetString());
            }
        }
    }

    LOGI_TAG("Config", "Config loaded: enabled={} aggressive={} log_level={}",
             config_.preferences.enabled,
             config_.preferences.aggressive_mode,
             config_.preferences.log_level);
    return true;
}

// ── Default ───────────────────────────────────────────────────────────────────

bool FluxThermalConfigStore::create_default_config() {
    {
        std::lock_guard<std::mutex> lk(mutex_);
        config_ = ConfigData{};
        // Default zone filter: monitor CPU, GPU, skin, SoC zones
        config_.zone_filter.include_types = {"cpu", "gpu", "soc", "skin", "thermal"};
    }
    return save_config(config_path_);
}

// ── Reload ─────────────────────────────────────────────────────────────────────

bool FluxThermalConfigStore::reload() {
    return load_config(config_path_);
}

// ── Accessors ─────────────────────────────────────────────────────────────────

FluxThermalConfigStore::Preferences FluxThermalConfigStore::get_preferences() const {
    std::lock_guard<std::mutex> lk(mutex_);
    return config_.preferences;
}

FluxThermalConfigStore::ZoneFilter FluxThermalConfigStore::get_zone_filter() const {
    std::lock_guard<std::mutex> lk(mutex_);
    return config_.zone_filter;
}

FluxThermalConfigStore::ConfigData FluxThermalConfigStore::get_config() const {
    std::lock_guard<std::mutex> lk(mutex_);
    return config_;
}

void FluxThermalConfigStore::set_preferences(const Preferences &p) {
    std::lock_guard<std::mutex> lk(mutex_);
    config_.preferences = p;
}

// ── Effective thresholds ─────────────────────────────────────────────────────

int FluxThermalConfigStore::effective_threshold_cool() const {
    std::lock_guard<std::mutex> lk(mutex_);
    return config_.preferences.threshold_cool_mc > 0
           ? config_.preferences.threshold_cool_mc
           : TEMP_THRESHOLD_COOL;
}

int FluxThermalConfigStore::effective_threshold_warm() const {
    std::lock_guard<std::mutex> lk(mutex_);
    return config_.preferences.threshold_warm_mc > 0
           ? config_.preferences.threshold_warm_mc
           : TEMP_THRESHOLD_WARM;
}

int FluxThermalConfigStore::effective_threshold_hot() const {
    std::lock_guard<std::mutex> lk(mutex_);
    return config_.preferences.threshold_hot_mc > 0
           ? config_.preferences.threshold_hot_mc
           : TEMP_THRESHOLD_HOT;
}

int FluxThermalConfigStore::effective_threshold_critical() const {
    std::lock_guard<std::mutex> lk(mutex_);
    return config_.preferences.threshold_critical_mc > 0
           ? config_.preferences.threshold_critical_mc
           : TEMP_THRESHOLD_CRITICAL;
}
