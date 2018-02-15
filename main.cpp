//
// Copyright Aliaksei Levin (levlam@telegram.org), Arseny Smirnov (arseny30@gmail.com) 2014-2017
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
#include <td/telegram/Client.h>
#include <td/telegram/Log.h>

#include <phpcpp.h>
#include "exception.cpp"

#include <exception>
#include <cstdint>
#include <functional>
#include <iostream>
#include <limits>
#include <map>
#include <sstream>
#include <string>
#include <vector>

class X {
 public:
  void __construct(Php::Parameters &params) {
    if (params[0]) {
       if (!params[0].isArray()) {
         throw new Exception("Invalid setting array was provided");
       }
    td::Log::set_verbosity_level(1);
    client_ = std::make_unique<td::Client>();
  }

  void loop() {
    while (true) {
      if (need_restart_) {
        restart();
      } else if (!are_authorized_) {
        process_response(client_->receive(10));
      } else {
        std::cerr
            << "Enter action [q] quit [u] check for updates and request results [c] show chats [m <id> <text>] send message [l] logout: "
            << std::endl;
        std::string line;
        std::getline(std::cin, line);
        std::istringstream ss(line);
        std::string action;
        if (!(ss >> action)) {
          continue;
        }
        if (action == "q") {
          return;
        }
        if (action == "u") {
          std::cerr << "Checking for updates..." << std::endl;
          while (true) {
            auto response = client_->receive(0);
            if (response.object) {
              process_response(std::move(response));
            } else {
              break;
            }
          }
        } else if (action == "l") {
          std::cerr << "Logging out..." << std::endl;
          send_query(td::api::make_object<td::api::logOut>(), {});
        } else if (action == "m") {
          std::int64_t chat_id;
          ss >> chat_id;
          ss.get();
          std::string text;
          std::getline(ss, text);

          std::cerr << "Sending message to chat " << chat_id << "..." << std::endl;
          auto send_message = td::api::make_object<td::api::sendMessage>();
          send_message->chat_id_ = chat_id;
          auto message_content = td::api::make_object<td::api::inputMessageText>();
          message_content->text_ = std::move(text);
          send_message->input_message_content_ = std::move(message_content);

          send_query(std::move(send_message), {});
        } else if (action == "c") {
          std::cerr << "Loading chat list..." << std::endl;
          send_query(td::api::make_object<td::api::getChats>(std::numeric_limits<std::int64_t>::max(), 0, 20),
                     [this](Object object) {
                       if (object->get_id() == td::api::error::ID) {
                         return;
                       }
                       auto chats = td::move_tl_object_as<td::api::chats>(object);
                       for (auto chat_id : chats->chat_ids_) {
                         std::cerr << "[id:" << chat_id << "] [title:" << chat_title_[chat_id] << "]" << std::endl;
                       }
                     });
        }
      }
    }
  }

 private:
  using Object = td::api::object_ptr<td::api::Object>;
  std::unique_ptr<td::Client> client_;

  td::api::object_ptr<td::api::AuthorizationState> authorization_state_;
  bool are_authorized_{false};
  bool need_restart_{false};
  std::uint64_t current_query_id_{0};
  std::uint64_t authentication_query_id_{0};

  std::map<std::uint64_t, std::function<void(Object)>> handlers_;

  std::map<std::int32_t, td::api::object_ptr<td::api::user>> users_;

  std::map<std::int64_t, std::string> chat_title_;

  void restart() {
    client_.reset();
    *this = X();
  }

  void send_query(td::api::object_ptr<td::api::Function> f, std::function<void(Object)> handler) {
    auto query_id = next_query_id();
    if (handler) {
      handlers_.emplace(query_id, std::move(handler));
    }
    client_->send({query_id, std::move(f)});
  }

  void process_response(td::Client::Response response) {
    if (!response.object) {
      return;
    }
    //std::cerr << response.id << " " << to_string(response.object) << std::endl;
    if (response.id == 0) {
      return process_update(std::move(response.object));
    }
    auto it = handlers_.find(response.id);
    if (it != handlers_.end()) {
      it->second(std::move(response.object));
    }
  }

  std::string get_user_name(std::int32_t user_id) {
    auto it = users_.find(user_id);
    if (it == users_.end()) {
      return "unknown user";
    }
    return it->second->first_name_ + " " + it->second->last_name_;
  }

  void process_update(td::api::object_ptr<td::api::Object> update) {
    td::api::downcast_call(
        *update, overloaded(
                     [this](td::api::updateAuthorizationState &update_authorization_state) {
                       authorization_state_ = std::move(update_authorization_state.authorization_state_);
                       on_authorization_state_update();
                     },
                     [this](td::api::updateNewChat &update_new_chat) {
                       chat_title_[update_new_chat.chat_->id_] = update_new_chat.chat_->title_;
                     },
                     [this](td::api::updateChatTitle &update_chat_title) {
                       chat_title_[update_chat_title.chat_id_] = update_chat_title.title_;
                     },
                     [this](td::api::updateUser &update_user) {
                       auto user_id = update_user.user_->id_;
                       users_[user_id] = std::move(update_user.user_);
                     },
                     [this](td::api::updateNewMessage &update_new_message) {
                       auto chat_id = update_new_message.message_->chat_id_;
                       auto sender_user_name = get_user_name(update_new_message.message_->sender_user_id_);
                       std::string text;
                       if (update_new_message.message_->content_->get_id() == td::api::messageText::ID) {
                         text = static_cast<td::api::messageText &>(*update_new_message.message_->content_).text_;
                       }
                       std::cerr << "Got message: [chat_id:" << chat_id << "] [from:" << sender_user_name << "] ["
                                 << text << "]" << std::endl;
                     },
                     [](auto &update) {}));
  }

  auto create_authentication_query_handler() {
    return [this, id = authentication_query_id_](Object object) {
      if (id == authentication_query_id_) {
        check_authentication_error(std::move(object));
      }
    };
  }

  void on_authorization_state_update() {
    authentication_query_id_++;
    td::api::downcast_call(
        *authorization_state_,
        overloaded(
            [this](td::api::authorizationStateReady &) {
              are_authorized_ = true;
              std::cerr << "Got authorization" << std::endl;
            },
            [this](td::api::authorizationStateLoggingOut &) {
              are_authorized_ = false;
              std::cerr << "Logging out" << std::endl;
            },
            [this](td::api::authorizationStateClosing &) { std::cerr << "Closing" << std::endl; },
            [this](td::api::authorizationStateClosed &) {
              are_authorized_ = false;
              need_restart_ = true;
              std::cerr << "Terminated" << std::endl;
            },
            [this](td::api::authorizationStateWaitCode &wait_code) {
              std::string first_name;
              std::string last_name;
              if (!wait_code.is_registered_) {
                std::cerr << "Enter your first name: ";
                std::cin >> first_name;
                std::cerr << "Enter your last name: ";
                std::cin >> last_name;
              }
              std::cerr << "Enter authentication code: ";
              std::string code;
              std::cin >> code;
              send_query(td::api::make_object<td::api::checkAuthenticationCode>(code, first_name, last_name),
                         create_authentication_query_handler());
            },
            [this](td::api::authorizationStateWaitPassword &) {
              std::cerr << "Enter authentication password: ";
              std::string password;
              std::cin >> password;
              send_query(td::api::make_object<td::api::checkAuthenticationPassword>(password),
                         create_authentication_query_handler());
            },
            [this](td::api::authorizationStateWaitPhoneNumber &) {
              std::cerr << "Enter phone number: ";
              std::string phone_number;
              std::cin >> phone_number;
              send_query(td::api::make_object<td::api::setAuthenticationPhoneNumber>(
                             phone_number, false /*allow_flash_calls*/, false /*is_current_phone_number*/),
                         create_authentication_query_handler());
            },
            [this](td::api::authorizationStateWaitEncryptionKey &) {
              std::cerr << "Enter encryption key or DESTROY: ";
              std::string key;
              std::getline(std::cin, key);
              if (key == "DESTROY") {
                send_query(td::api::make_object<td::api::destroy>(), create_authentication_query_handler());
              } else {
                send_query(td::api::make_object<td::api::checkDatabaseEncryptionKey>(std::move(key)),
                           create_authentication_query_handler());
              }
            },
            [this](td::api::authorizationStateWaitTdlibParameters &) {
              auto parameters = td::api::make_object<td::api::tdlibParameters>();
              parameters->use_message_database_ = true;
              parameters->use_secret_chats_ = true;
              parameters->api_id_ = 94575;
              parameters->api_hash_ = "a3406de8d171bb422bb6ddf3bbd800e2";
              parameters->system_language_code_ = "en";
              parameters->device_model_ = "Desktop";
              parameters->system_version_ = "Unknown";
              parameters->application_version_ = "1.0";
              parameters->enable_storage_optimizer_ = true;
              send_query(td::api::make_object<td::api::setTdlibParameters>(std::move(parameters)),
                         create_authentication_query_handler());
            }));
  }

  void check_authentication_error(Object object) {
    if (object->get_id() == td::api::error::ID) {
      auto error = td::move_tl_object_as<td::api::error>(object);
      std::cerr << "Error: " << to_string(error);
      on_authorization_state_update();
    }
  }

  std::uint64_t next_query_id() {
    return ++current_query_id_;
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
        Php::Class<X> x("X");

        x.method<&X::__construct>("__construct", Php::Public | Php::Final);
        x.method<&X::__wakeup>("__wakeup", Php::Public | Php::Final);
        x.method<&X::__sleep>("__sleep", Php::Public | Php::Final);

        x.property("settings", 0, Php::Public);

        x.constant("PIF_TDPONY_VERSION", "1.0");

        Php::Namespace danog("danog");
        Php::Namespace MadelineProto("MadelineProto");

        Php::Class<Exception> exception("Exception");

        MadelineProto.add(std::move(exception));
        MadelineProto.add(std::move(x));
        danog.add(std::move(MadelineProto));
        extension.add(std::move(danog));

        return extension;
    }
}
