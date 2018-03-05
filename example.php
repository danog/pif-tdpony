<?php
$parameters = [
  "use_message_database" => true,
  "api_id" => YOUR_APIID,
  "api_hash" => YOUR_APIHASH,
  "system_language_code" => "en",
  "device_model" => "MadelineProto X",
  "system_version" => "unk",
  "application_version" => "1.1",
  "enable_storage_optimizer" => true,
  "use_pfs" => true,
  "database_directory" => "./madelinex"
];
$client = new \danog\MadelineProto\X\API($parameters);
