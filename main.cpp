//
// Copyright Aliaksei Levin (levlam@telegram.org), Arseny Smirnov (arseny30@gmail.com) 2014-2017
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
#include <td/telegram/ClientJson.h>
#include <tdutils/td/utils/Slice.h>
#include <td/telegram/Log.h>

#include <phpcpp.h>

#include <cstdint>
#include <functional>
#include <iostream>
#include <limits>
#include <map>
#include <sstream>
#include <string>
#include <vector>

class API : public Php::Base {
 public:
  API() = default;
  virtual ~API() = default;
  
  void __construct() {
    initTdlib();
  }
  void __wakeup() {
    initTdlib();
    Php::Value self(this);
    if (self["tdlibParameters"]) client_->send(toSlice(self["tdlibParameters"]));
  }
  void __destruct() {
    deinitTdlib();
  }

  void initTdlib() {
    client_ = std::make_unique<td::ClientJson>();
  }
  void deinitTdlib() {
    client_.reset();
  }
  void send(Php::Parameters &params) {
    Php::Value value = params[0];
    if (value.get("@type") == "setTdlibParameters") {
      Php::Value self(this);
      self["tdlibParameters"] = value;
    }
    client_->send(toSlice(value));
  }
  
  Php::Value receive(Php::Parameters &params) {
    auto slice = client_->receive(params[0]);
    if (slice.empty()) {
      return nullptr;
    }
    Php::Value result = Php::call("json_decode", slice.c_str(), true);
    Php::Value self(this);
    return result;
  }
  
  Php::Value execute(Php::Parameters &params) {
    Php::Value value = params[0];
    if (value.get("@type") == "setTdlibParameters") {
      Php::Value self(this);
      self["tdlibParameters"] = value;
    }
    auto slice = client_->execute(toSlice(value));
    if (slice.empty()) {
      return nullptr;
    }
    return Php::call("json_decode", slice.c_str(), true);
  }
  
  

 private:
  std::unique_ptr<td::ClientJson> client_;
  td::Slice toSlice(Php::Value value) {
    std::string str = Php::call("json_encode", value);
    return td::Slice(str);
  }

};


class Logging : public Php::Base
{
  public:
  Logging() = default;
  virtual ~Logging() = default;
  
  static Php::Value set_file_path(Php::Parameters &params) {
    return td::Log::set_file_path(params[0]);
  }
  static void set_max_file_size(Php::Parameters &params) {
    td::Log::set_max_file_size(params[0]);
  }
  static void set_verbosity_level(Php::Parameters &params) {
    td::Log::set_verbosity_level(params[0]);
  }
};


extern "C" {

    /**
         *  Function that is called by PHP right after the PHP process
         *  has started, and that returns an address of an internal PHP
         *  strucure with all the details and features of your extension
         *
         *  @return void*   a pointer to an address that is understood by PHP
         */
    PHPCPP_EXPORT void *get_module()
    {
        // static(!) Php::Extension object that should stay in memory
        // for the entire duration of the process (that's why it's static)
        static Php::Extension extension("pif-tdpony", "1.0");

        // description of the class so that PHP knows which methods are accessible
        Php::Class<API> api("API");

        api.method<&API::__construct>("__construct", Php::Public | Php::Final, {});
        api.method<&API::__wakeup>("__wakeup", Php::Public | Php::Final, {});
        api.method<&API::__destruct>("__destruct", Php::Public | Php::Final, {});
        api.method<&API::send>("send", Php::Public | Php::Final);
        api.method<&API::receive>("receive", Php::Public | Php::Final);
        api.method<&API::execute>("execute", Php::Public | Php::Final);

        api.property("settings", 0, Php::Public);

        api.constant("PIF_TDPONY_VERSION", "1.0");

        Php::Namespace danog("danog");
        Php::Namespace MadelineProto("MadelineProto");
        Php::Namespace X("X");

        Php::Class<Logging> logging("Logging");

        logging.method<&Logging::set_file_path>("set_file_path", Php::Public, {Php::ByVal("file_path", Php::Type::String)});
        logging.method<&Logging::set_max_file_size>("set_max_file_size", Php::Public, {Php::ByVal("set_max_file_size", Php::Type::Numeric)});
        logging.method<&Logging::set_verbosity_level>("set_verbosity_level", Php::Public, {Php::ByVal("verbosity_level", Php::Type::Numeric)});

        X.add(std::move(api));
        X.add(std::move(logging));
        MadelineProto.add(std::move(X));
        danog.add(std::move(MadelineProto));
        extension.add(std::move(danog));

        return extension;
    }
}
