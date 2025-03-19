#pragma once

#include <vector>
#include <string>
#include "ArduinoJson.h"

// copied from https://arduinojson.org/v7/how-to/create-converters-for-stl-containers/

namespace ArduinoJson
{
  template <typename T>
  struct Converter<std::vector<T> >
  {
    static void toJson(const std::vector<T>& src, JsonVariant dst)
    {
      JsonArray array = dst.to<JsonArray>();
      for (T item : src)
        array.add(item);
    }

    static std::vector<T> fromJson(JsonVariantConst src)
    {
      std::vector<T> dst;
      for (T item : src.as<JsonArrayConst>())
        dst.push_back(item);
      return dst;
    }

    static bool checkJson(JsonVariantConst src)
    {
      JsonArrayConst array = src;
      bool result = array;
      for (JsonVariantConst item : array)
        result &= item.is<T>();
      return result;
    }
  };
}