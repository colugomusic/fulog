# i just want to fucking log to a file

This single-header c++20 library was created out of sheer frustration with the number of C++ logging libraries I tried which either:

1. Were stupidly complicated, despite claiming to be "simple"
2. Didn't even work

## Platforms
- Windows
- macOS
- Linux
- You can probably make it work on anything else without much effort

## Features
- Thread safe
- Just fucking logs to a file
- Fuck you

## Performance
Who gives a shit. I just want to log to a file.

## Usage
```c++
#include <fulog.hpp>
int main(int argc, const char* argv[]) {
  fu::log("message");
}
```
The above program creates a log file at: `{logging dir}/{application name}/{log name}.log`

By default:
- `logging dir` is the application data directory for the platform.
- `application name` is `fulog`.
- `log name` is the process id.

So by default, here is where the log file will go on each platform:

- Windows: `C:/Users/(username)/AppData/Roaming/fulog/(process id).log`
- macOS: `/Users/(username)/Library/Application Support/fulog/(process id).log`
- Linux: `/home/(username)/.local/share/fulog/(process id).log`

## To change the logging directory
```c++
fu::set_dir("/somewhere/else");
```

## To change the application name
```c++
fu::set_application_name("something else");
```

## To change the log name
```c++
fu::set_log_name("something else");
```

## To change the timestamp format
```c++
fu::set_timestamp_format("{:%Y-%m-%d %H:%M:%S}");
```

## To log something only if it's a debug build
```c++
fu::debug_log("message");
```

## To print different "types" of log message
```c++
fu::log("INFO: fuck you");
fu::log("WARNING: just do it yourself")
```

## To log using a format string
```c++
fu::log(std::format("fuck you: {}", "just do it yourself"));
```

## To include the source location of the log message
```c++
fu::log("message", std::source_location::current());
```

## To clean up the log file directory by deleting anything older than 48 hours
```c++
fu::delete_old_files(std::chrono::hours{48});
```
