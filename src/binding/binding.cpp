extern "C" {
#include "sgp30.h"
}

#include "binding_utils.h"

#include <napi.h>

#include <string>

Napi::Object init(const Napi::CallbackInfo &info) {
  Napi::Env env = info.Env();

  std::string i2cAdaptor{"/dev/i2c-1"}; 
  if (info.Length() == 1) {
    i2cAdaptor = static_cast<std::string>(info[0].As<Napi::String>());
  }

  int err = SGP30_init(i2cAdaptor.c_str());
  if (err) {
    return BindingUtils::errFactory(env, err,
      "Could not initialize SGP30 module; are you running as root?");
  }

  Napi::Object returnObject = Napi::Object::New(env);
  returnObject.Set(Napi::String::New(env, "returnCode"), Napi::Number::New(env, err));
  return returnObject;
}

Napi::Object deinit(const Napi::CallbackInfo &info) {
  Napi::Env env = info.Env();

  int err = SGP30_deinit();
  if (err) {
    return BindingUtils::errFactory(env, err,
      "Could not deinitialize SGP30 module; are you running as root?");
  }

  Napi::Object returnObject = Napi::Object::New(env);
  returnObject.Set(Napi::String::New(env, "returnCode"), Napi::Number::New(env, err));
  return returnObject;
}

Napi::Object measure_air_quality(const Napi::CallbackInfo &info) {
  Napi::Env env = info.Env();

  uint16_t eCO2, TVOC;
  int err = SGP30_measure_air_quality(&eCO2, &TVOC);
  if (err) {
    return BindingUtils::errFactory(env, err,
      "Could not measure air quality from SGP30 module; did you run init() first?");
  }

  Napi::Object returnObject = Napi::Object::New(env);
  returnObject.Set(Napi::String::New(env, "eCO2"), Napi::Number::New(env, eCO2));
  returnObject.Set(Napi::String::New(env, "TVOC"), Napi::Number::New(env, TVOC));
  return returnObject;
}

Napi::Object measure_raw_signals(const Napi::CallbackInfo &info) {
  Napi::Env env = info.Env();

  uint16_t H2, ethanol;
  int err = SGP30_measure_air_quality(&H2, &ethanol);
  if (err) {
    return BindingUtils::errFactory(env, err,
      "Could not measure raw signals from SGP30 module; did you run init() first?");
  }

  Napi::Object returnObject = Napi::Object::New(env);
  returnObject.Set(Napi::String::New(env, "H2"), Napi::Number::New(env, H2));
  returnObject.Set(Napi::String::New(env, "ethanol"), Napi::Number::New(env, ethanol));
  return returnObject;
}

Napi::Object get_baseline(const Napi::CallbackInfo &info) {
  Napi::Env env = info.Env();

  uint16_t eCO2_baseline, TVOC_baseline;
  int err = SGP30_get_baseline(&eCO2_baseline, &TVOC_baseline);
  if (err) {
    return BindingUtils::errFactory(env, err,
      "Could not measure raw signals from SGP30 module; did you run init() first?");
  }

  Napi::Object returnObject = Napi::Object::New(env);
  returnObject.Set(Napi::String::New(env, "eCO2_baseline"), Napi::Number::New(env, eCO2_baseline));
  returnObject.Set(Napi::String::New(env, "TVOC_baseline"), Napi::Number::New(env, TVOC_baseline));
  return returnObject;
}

Napi::Object set_baseline(const Napi::CallbackInfo &info) {
  uint16_t eCO2_baseline = static_cast<uint32_t>(info[0].As<Napi::Number>()) & 0xFFFF;
  uint16_t TVOC_baseline = static_cast<uint32_t>(info[1].As<Napi::Number>()) & 0xFFFF;
  Napi::Env env = info.Env();

  int err = SGP30_set_baseline(eCO2_baseline, TVOC_baseline);
  if (err) {
    return BindingUtils::errFactory(env, err,
      "Could not measure raw signals from SGP30 module; did you run init() first?");
  }

  Napi::Object returnObject = Napi::Object::New(env);
  returnObject.Set(Napi::String::New(env, "returnCode"), Napi::Number::New(env, err));
  return returnObject;
}

Napi::Object set_humidity(const Napi::CallbackInfo &info) {
  double humidity_input = info[0].As<Napi::Number>();
  uint8_t humidity_integer = (uint8_t)humidity_input;
  uint8_t humidity_fractional = humidity_input-(uint8_t)humidity_input;   
  uint16_t humidity = (humidity_integer << 8) | humidity_fractional;

  Napi::Env env = info.Env();

  int err = SGP30_set_humidity(humidity);
  if (err) {
    return BindingUtils::errFactory(env, err,
      "Could not measure raw signals from SGP30 module; did you run init() first?");
  }

  Napi::Object returnObject = Napi::Object::New(env);
  returnObject.Set(Napi::String::New(env, "returnCode"), Napi::Number::New(env, err));
  return returnObject;
}

Napi::Boolean measure_test(const Napi::CallbackInfo &info) {
  Napi::Env env = info.Env();

  int err = SGP30_measure_test();

  Napi::Boolean returnBoolean = Napi::Boolean::New(env, err);
  return returnBoolean;
}

Napi::Object get_feature_set_version(const Napi::CallbackInfo &info) {
  Napi::Env env = info.Env();

  uint16_t version;
  int err = SGP30_get_feature_set_version(&version);
  if (err) {
    return BindingUtils::errFactory(env, err,
      "Could not get feature set version from SGP30 module; did you run init() first?");
  }

  Napi::Object returnObject = Napi::Object::New(env);
  returnObject.Set(Napi::String::New(env, "version"), Napi::Number::New(env, version));
  return returnObject;
}

Napi::Object get_serial_id(const Napi::CallbackInfo &info) {
  Napi::Env env = info.Env();

  uint64_t serial_id;
  int err = SGP30_get_serial_id(&serial_id);
  if (err) {
    return BindingUtils::errFactory(env, err,
      "Could not get serial ID from SGP30 module; did you run init() first?");
  }

  Napi::Object returnObject = Napi::Object::New(env);
  returnObject.Set(Napi::String::New(env, "serial"), Napi::Number::New(env, serial_id));
  return returnObject;
}

Napi::Object Init(Napi::Env env, Napi::Object exports) {
  exports.Set(Napi::String::New(env, "init"),
              Napi::Function::New(env, init));
  exports.Set(Napi::String::New(env, "deinit"),
              Napi::Function::New(env, deinit));
  exports.Set(Napi::String::New(env, "measureAirQuality"),
              Napi::Function::New(env, measure_air_quality));
  exports.Set(Napi::String::New(env, "measureRawSignals"),
              Napi::Function::New(env, measure_raw_signals));
  exports.Set(Napi::String::New(env, "getBaseline"),
              Napi::Function::New(env, get_baseline));
  exports.Set(Napi::String::New(env, "setBaseline"),
              Napi::Function::New(env, set_baseline));
  exports.Set(Napi::String::New(env, "setHumidity"),
              Napi::Function::New(env, set_humidity));
  exports.Set(Napi::String::New(env, "measureTest"),
              Napi::Function::New(env, measure_test));
  exports.Set(Napi::String::New(env, "getFeatureSetVersion"),
              Napi::Function::New(env, get_feature_set_version));
  exports.Set(Napi::String::New(env, "getSerialID"),
              Napi::Function::New(env, get_serial_id));
  return exports;
}

NODE_API_MODULE(homebridgesgp30, Init)
