A shittely put together document to get my thoughts on the language's design out.

# Functions
```Fumi
procedure constraints<T> with
   a: T
   returns T
where
   T implements dup
begin
   result : T = a.dup
   return result;
end

procedure add with a : i32, b : i32 returns i32 begin
   result := a + b;
   return result;
end

extern puts

procedure main begin
   puts("zhyivannye miratte")
end
```

# Types and variables
```Fumi
infer := 69
specify : i32 = 420;

struct vec2<T> is
   x: T
   y: T
where
   T implements copy
end
```
