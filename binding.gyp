{
  "targets": [
    {
      "target_name": "cloud_breaker",
      "sources": [ "binding.cpp" ],
      "include_dirs": [
        "<!(node -e \"require('nan')\")"
      ]
    }
  ]
}