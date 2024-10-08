[//]: # (This file is automatically generated by Cute Framework's docs parser.)
[//]: # (Do not edit this file by hand!)
[//]: # (See: https://github.com/RandyGaul/cute_framework/blob/master/samples/docs_parser.cpp)
[](../header.md ':include')

# cf_fs_destroy

Category: [file](/api_reference?id=file)  
GitHub: [cute_file_system.h](https://github.com/RandyGaul/cute_framework/blob/master/include/cute_file_system.h)  
---

Destroys the [Virtual File System](https://randygaul.github.io/cute_framework/#/topics/virtual_file_system).

```cpp
void cf_fs_destroy();
```

Parameters | Description
--- | ---
argv0 | The first command-line argument passed into your `main` function.

## Remarks

Cleans up all static memory used by [cf_fs_init](/file/cf_fs_init.md). You probably don't need to call this function,
as `cf_app_destroy` already does this for you.

## Related Pages

[cf_fs_init](/file/cf_fs_init.md)  
