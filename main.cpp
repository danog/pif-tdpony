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
#include <td/telegram/td_log.h>

#include <phpcpp.h>

#include <string>
#include <queue>

class API : public Php::Base {
 public:
  API() = default;
  virtual ~API() = default;

  void __construct(Php::Parameters &params) {
    self = Php::Value(this);
    if (params.size() != 1) {
      throw Php::Exception("Invalid parameter count");
    }
    if (params[0].isString()) {
      if (!Php::call("file_exists", params[0])) {
        throw Php::Exception("Provided file does not exist!");
      }
      Php::Value unserialized = Php::call("unserialize", Php::call("file_get_contents", params[0]));
      if (!unserialized.instanceOf("\\danog\\MadelineProto\\X\\TD")) {
        throw Php::Exception("An error occurred during deserialization!");
      }
      self["API"] = unserialized;
      self["session"] = params[0];
    } else {
      self["API"] = Php::Object("\\danog\\MadelineProto\\X\\TD", params[0]);
    }
  }
  void serialize(Php::Parameters &params) {
    Php::Value path;
    if (params.size() == 1 && params[0].isString()) {
      path = params[0];
    } else if (self.contains("session") && self.get("session").isString()) {
      path = self.get("session");
    } else {
      throw Php::Exception("No valid path was provided");
    }
    last_serialized = Php::call("time");
    Php::call("file_put_contents", path, Php::call("serialize", self.get("API")));
  }
  void __destruct() {
    if (self.contains("session") && self.get("session").isString()) {
      last_serialized = Php::call("time");
      Php::call("file_put_contents", self.get("session"), Php::call("serialize", self.get("API")));
    }
  }
  Php::Value __actualcall(Php::Parameters &params) {
    if (last_serialized < Php::call("time") - 30 && self.contains("session") && self.get("session").isString()) {
      last_serialized = Php::call("time");
      Php::call("file_put_contents", self.get("session"), Php::call("serialize", self.get("API")));
    }
    if (params[1].size() == 1) {
      return self.get("API").call(params[0], params[1].get(0));
    }
    return self.get("API").call(params[0]);
  }

 private:
  Php::Value self;
  Php::Value last_serialized = 0;
};

class TD : public Php::Base {
 public:
  TD() = default;
  virtual ~TD() = default;

  void __construct(Php::Parameters &params) {
    self = Php::Value(this);
    self["tdlibParameters"]["@type"] = "setTdlibParameters";
    self["tdlibParameters"]["parameters"] = params[0];
    initTdlib();
  }
  void __wakeup() {
    self = Php::Value(this);
    initTdlib();
  }
  void __destruct() {
    deinitTdlib();
  }

  Php::Value __sleep() {
    Php::Array buffer;
    while (updates.size()) {
      buffer[buffer.size()] = updates.front();
      updates.pop();
    }
    for (auto &item : buffer) {
      updates.push(item.second);
    }
    self["updatesBuffer"] = buffer;
    Php::Array result({"updatesBuffer", "tdlibParameters"});
    return result;
  }

  void initTdlib() {
    client = td_json_client_create();
    if (self["updatesBuffer"]) {
      Php::Value buffer = self["updatesBuffer"];
      for (auto &item : buffer) {
        updates.push(item.second);
      }
      Php::Array empty;
      self["updatesBuffer"] = empty;
    }

    Php::Value update;
    while (true) {
      update = json_decode(td_json_client_receive(client, 0));
      if (update) {
        if (update.get("@type") == "updateAuthorizationState" && update.get("authorization_state").get("@type") == "authorizationStateWaitTdlibParameters") {
          execute(self["tdlibParameters"]);
          break;
        }
        updates.push(update);
      }
    }
  }
  void deinitTdlib() {
    td_json_client_destroy(client);
  }


  void send(Php::Parameters &params) {
    Php::Value value = params[0];
    if (value.get("@type") == "setTdlibParameters") {
      self["tdlibParameters"] = value;
    }
    td_json_client_send(client, json_encode(value));
  }

  Php::Value receive(Php::Parameters &params) {
    if (updates.size()) {
      Php::Value res = updates.front();
      updates.pop();
      return res;
    }
    return json_decode(td_json_client_receive(client, params[0]));
  }

  Php::Value execute(Php::Parameters &params) {
    return execute(params[0]);
  }

  Php::Value execute(Php::Value value) {
    if (value["@type"] && value.get("@type") == "setTdlibParameters") {
      self["tdlibParameters"] = value;
    }
    std::int64_t query_id = next_query_id();
    value["@extra"] = query_id;

    td_json_client_send(client, json_encode(value));

    Php::Value update;
    while (true) {
      update = json_decode(td_json_client_receive(client, 0));
      if (update) {
        if (update["@extra"] == query_id) {
          return update;
        }

        updates.push(update);
      }
    }
  }
  Php::Value __call(const char *name, Php::Parameters &params) {
    Php::Array value;
    if (params.size() == 1) {
      value = params[0];
    }
    value["@type"] = name;
    return execute(value);
  }
 private:
  void *client;
  std::queue<Php::Value> updates;
  Php::Value self;

  std::int64_t query_id = 0;
  std::int64_t next_query_id() {
    return ++query_id;
  }
  const char *json_encode(Php::Value value) {
    return strdup(Php::call("json_encode", value));
  }
  Php::Value json_decode(const char *value) {
    Php::Value result;
    if (value) {
      result = Php::call("json_decode", value, true);
    }
    return result;
  }
};


class Logger : public Php::Base
{
  public:
  Logger() = default;
  virtual ~Logger() = default;
  
  static Php::Value set_file_path(Php::Parameters &params) {
    return td_set_log_file_path(params[0]);
  }
  static void set_max_file_size(Php::Parameters &params) {
    std::int64_t size = params[0];
    td_set_log_max_file_size(size);
  }
  static void set_verbosity_level(Php::Parameters &params) {
    td_set_log_verbosity_level(params[0]);
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

        api.method<&API::__construct>("__construct", Php::Public | Php::Final);
        api.method<&API::__destruct>("__destruct", Php::Public | Php::Final, {});
        api.method<&API::__actualcall>("__call", Php::Public | Php::Final, {Php::ByVal("method", Php::Type::String), Php::ByVal("args", Php::Type::Array)});
        api.method<&API::serialize>("serialize", Php::Public | Php::Final, {Php::ByVal("path", Php::Type::String)});

        api.constant("PIF_TDPONY_VERSION", "1.0");


        Php::Class<TD> td("TD");
        td.method<&TD::__construct>("__construct", Php::Public | Php::Final);
        td.method<&TD::__destruct>("__destruct", Php::Public | Php::Final);
        td.method<&TD::__wakeup>("__wakeup", Php::Public | Php::Final);
        td.method<&TD::__sleep>("__sleep", Php::Public | Php::Final);
        td.method<&TD::send>("send", Php::Public | Php::Final);
        td.method<&TD::receive>("receive", Php::Public | Php::Final, {Php::ByVal("timeout", Php::Type::Float)});
        td.method<&TD::execute>("execute", Php::Public | Php::Final);

        td.property("tdlibParameters", nullptr, Php::Private);
        td.property("updatesBuffer", nullptr, Php::Private);


        Php::Namespace danog("danog");
        Php::Namespace MadelineProto("MadelineProto");
        Php::Namespace X("X");

        Php::Class<Logger> logger("Logger");

        logger.method<&Logger::set_file_path>("set_file_path", Php::Public, {Php::ByVal("file_path", Php::Type::String)});
        logger.method<&Logger::set_max_file_size>("set_max_file_size", Php::Public, {Php::ByVal("set_max_file_size", Php::Type::Numeric)});
        logger.method<&Logger::set_verbosity_level>("set_verbosity_level", Php::Public, {Php::ByVal("verbosity_level", Php::Type::Numeric)});

        X.add(std::move(api));
        X.add(std::move(td));
        X.add(std::move(logger));
        MadelineProto.add(std::move(X));
        danog.add(std::move(MadelineProto));
        extension.add(std::move(danog));

        return extension;
    }
}
