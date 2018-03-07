<?php


if (file_exists('session.madelinex')) {
    $a = new \danog\MadelineProto\X\API('session.madelinex');
} else {
    $params = [
       "use_message_database" => true,
       "api_id" => 6,
       "api_hash" => "eb06d4abfb49dc3eeb1aeb98ae0f581e",
       "system_language_code" => "en",
       "device_model" => "MadelineProto X",
       "system_version" => "unk",
       "application_version" => "1.1",
       "enable_storage_optimizer" => true,
       "use_pfs" => true,
       "database_directory" => "./madelinex"
    ];
    $a = new \danog\MadelineProto\X\API($params);
}
\danog\Madelineproto\X\Logger::set_verbosity_level(5);

$a->serialize('session.madelinex');
$a->setDatabaseEncryptionKey(); // no password for tdlib database


while (true) {
readline();
    $res = $a->receive(1);
    var_dumP($res);
}
