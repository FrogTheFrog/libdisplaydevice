#pragma once

#ifdef DD_JSON_DETAIL
  // system includes
  #include <nlohmann/json.hpp>

  // Special versions of the NLOHMANN definitions to remove the "m_" prefix in string form ('cause I like it that way ;P)
  #define DD_JSON_TO(v1) nlohmann_json_j[#v1] = nlohmann_json_t.m_##v1;
  #define DD_JSON_FROM(v1) nlohmann_json_j.at(#v1).get_to(nlohmann_json_t.m_##v1);

  // Coverage has trouble with inlined functions when they are included in different units (sometimes),
  // therefore the usual macro was split into declaration and definition
  #define DD_JSON_DECLARE_SERIALIZE_STRUCT(Type)                                \
    void to_json(nlohmann::json &nlohmann_json_j, const Type &nlohmann_json_t); \
    void from_json(const nlohmann::json &nlohmann_json_j, Type &nlohmann_json_t);

  #define DD_JSON_DEFINE_SERIALIZE_STRUCT(Type, ...)                               \
    void to_json(nlohmann::json &nlohmann_json_j, const Type &nlohmann_json_t) {   \
      NLOHMANN_JSON_EXPAND(NLOHMANN_JSON_PASTE(DD_JSON_TO, __VA_ARGS__))           \
    }                                                                              \
                                                                                   \
    void from_json(const nlohmann::json &nlohmann_json_j, Type &nlohmann_json_t) { \
      NLOHMANN_JSON_EXPAND(NLOHMANN_JSON_PASTE(DD_JSON_FROM, __VA_ARGS__))         \
    }

  // Reusing the same NLOHMANN macro as there seems to be no issues with coverage (yet...)
  #define DD_JSON_SERIALIZE_ENUM NLOHMANN_JSON_SERIALIZE_ENUM

namespace nlohmann {
  // Specialization for optional types until they actually implement it.
  template <typename T>
  struct adl_serializer<std::optional<T>> {
    static void
    to_json(json &j, const std::optional<T> &opt) {
      if (opt == std::nullopt) {
        j = nullptr;
      }
      else {
        j = *opt;
      }
    }

    static void
    from_json(const json &j, std::optional<T> &opt) {
      if (j.is_null()) {
        opt = std::nullopt;
      }
      else {
        opt = j.template get<T>();
      }
    }
  };
}  // namespace nlohmann
#endif
