/*
Copyright 2018 Daniil Gentili
(https://daniil.it)
This file is part of pif-tdpony.
pif-tdpony is free software: you can redistribute it and/or modify it under the terms of the GNU Affero General Public License as published by the free Software Foundation, either version 3 of the License, or (at your option) any later version.
The PWRTelegram API is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
See the GNU Affero General Public License for more details.
You should have received a copy of the GNU General Public License along with pif-tdpony.
If not, see <http://www.gnu.org/licenses/>.
*/

#include <td/telegram/td_json_client.h>
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
  
  void __construct(Php::Parameters &params) {
    Php::Value self(this);
    self["tdlibParameters"] = params[0];
    initTdlib();
  }
  void __wakeup() {
    initTdlib();
  }
  void __destruct() {
    deinitTdlib();
  }

  void initTdlib() {
    client = td_json_client_create();
    Php::Value self(this);
    Php::Array init;
    init["@type"] = "setTdlibParameters";
    init["tdlibParameters"] = self["tdlibParameters"];

    td_json_client_send(client, json_encode(init));
  }
  void deinitTdlib() {
    td_json_client_destroy(client);
  }

  void send(Php::Parameters &params) {
    Php::Value value = params[0];
    if (value.get("@type") == "setTdlibParameters") {
      Php::Value self(this);
      self["tdlibParameters"] = value;
    }
    td_json_client_send(client, json_encode(value));
  }
  
  Php::Value receive(Php::Parameters &params) {
    return json_decode(td_json_client_receive(client, params[0]));
  }
  
  Php::Value execute(Php::Parameters &params) {
    Php::Value value = params[0];
    if (value.get("@type") == "setTdlibParameters") {
      Php::Value self(this);
      self["tdlibParameters"] = value;
    }
    return json_decode(td_json_client_execute(client, json_encode(value)));
  }
  
  

 private:
  void *client;
  const char *json_encode(Php::Value value) {
    return strdup(Php::call("json_encode", value));
  }
  Php::Value json_decode(std::string value) {
    return Php::call("json_decode", value, true);
  }
};


class Logger : public Php::Base
{
  public:
  Logger() = default;
  virtual ~Logger() = default;
  
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

        api.method<&API::__construct>("__construct", Php::Public | Php::Final, {Php::ByVal("tdlibParameters", Php::Type::Array)});
        api.method<&API::__wakeup>("__wakeup", Php::Public | Php::Final, {});
        api.method<&API::__destruct>("__destruct", Php::Public | Php::Final, {});
        api.method<&API::send>("send", Php::Public | Php::Final);
        api.method<&API::receive>("receive", Php::Public | Php::Final, {Php::ByVal("timeout", Php::Type::Float)});
        api.method<&API::execute>("execute", Php::Public | Php::Final);

        api.property("tdlibParameters", nullptr, Php::Private);

        api.constant("PIF_TDPONY_VERSION", "1.0");

        Php::Namespace danog("danog");
        Php::Namespace MadelineProto("MadelineProto");
        Php::Namespace X("X");

        Php::Class<Logger> logger("Logger");

        logger.method<&Logger::set_file_path>("set_file_path", Php::Public, {Php::ByVal("file_path", Php::Type::String)});
        logger.method<&Logger::set_max_file_size>("set_max_file_size", Php::Public, {Php::ByVal("set_max_file_size", Php::Type::Numeric)});
        logger.method<&Logger::set_verbosity_level>("set_verbosity_level", Php::Public, {Php::ByVal("verbosity_level", Php::Type::Numeric)});

        X.add(std::move(api));
        X.add(std::move(logger));
        MadelineProto.add(std::move(X));
        danog.add(std::move(MadelineProto));
        extension.add(std::move(danog));

        return extension;
    }
}
