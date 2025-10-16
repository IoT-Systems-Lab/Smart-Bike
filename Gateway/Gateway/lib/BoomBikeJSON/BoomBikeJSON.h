// Put data in JSON format
// Returns JSON as string
// by Kjell

#ifndef BOOMBIKEJSON_H
#define BOOMBIKEJSON_H

#include <ArduinoJson.h>

class BoomBikeJSON {
private:
    StaticJsonDocument<512> doc;
public:
    BoomBikeJSON();
    void addData(const char* key, const char* value);
    void addData(const char* key, int value);
    void addData(const char* key, float value);
    void addData(const char* key, bool value);
    void addData(const char* key, double value);
    const char* toString();
    void clear();
};


#endif // BOOMBIKEJSON_H