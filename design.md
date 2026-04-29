A shittely put together document to get my thoughts on the language's design out.

# Functions
## Basic
```Fumi
procedure main with
   args : array<str>
begin
   puts("Zhyivannye miratte")
end
```

## Default parameters
```Fumi
procedure list-dir with
   path : str,
   max-depth : i32 = 10
begin
   // Implementation
end
```

## External
```Fumi
external "C" procedure puts with
   s: str
end
```

## Generics
```Fumi
procedure display<T> with
   i: T
   returns void
where
   T implements to_str
begin
   puts(i.to_str())
end
```

# Variables
```Fumi
type-inferred := 95 // inferred as i32
specify : str = "type specified manually"
```

# Types
## Structs & Unions
```Fumi
union mystery-box is
   magic-value: i32,
   name: str
end

struct vec2<T> is
   x: T,
   y: T
where
   T implements copy
end
```

## Enums & Tagged unions
```Fumi
enum error is
   OutOfMemory,
   FileNotFound
end

enum token is union of
   Identifier(str),
   IntLiteral(i64),
   Keyword
end
```

# Modules;
std/io.fumi
```Fumi
// Namespace is that of the file's name

procedure println with format: str begin
   // Implementation
end
```

std/path.fumi
```Fumi
module "file-system" // Redefine this file's export namespace to be "file-system"

procedure list-files begin
   // Implementation
end
```

main.fumi
```Fumi
import "std/io.fumi"
import "std/path.fumi"
import "std/path.fumi" as fs; // Alias module "file-system" to "fs" in the current file

procedure main begin
   io.println("zhyivannye, miratte")
   file-system.list-files()
   fs.list-files;
end
```

