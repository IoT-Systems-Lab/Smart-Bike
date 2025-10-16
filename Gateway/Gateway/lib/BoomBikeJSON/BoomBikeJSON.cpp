#include "BoomBikeJSON.h"

BoomBikeJSON::BoomBikeJSON() {
    // Constructor
}

void BoomBikeJSON::addData(const char* key, const char* value) {
    doc[key] = value;
}

void BoomBikeJSON::addData(const char* key, int value) {
    doc[key] = value;
}

void BoomBikeJSON::addData(const char* key, float value) {
    doc[key] = value;
}

void BoomBikeJSON::addData(const char* key, bool value) {
    doc[key] = value;
}

void BoomBikeJSON::addData(const char* key, double value) {
    doc[key] = value;
}

const char* BoomBikeJSON::toString() {
    String json;
    serializeJson(doc, json);
    return strdup(json.c_str());
}

void BoomBikeJSON::clear() {
    doc.clear();
}
