[](../header.md ':include')

# cf_fs_remove_directory

Category: [file](/api_reference?id=file)  
GitHub: [cute_file_system.h](https://github.com/RandyGaul/cute_framework/blob/master/include/cute_file_system.h)  
---

Removes a directory.

```cpp
CF_Result cf_fs_remove_directory(const char* virtual_path);
```

Parameters | Description
--- | ---
virtual_path | The virtual path to the directory.

## Return Value

Returns any errors as a [CF_Result](/utility/cf_result.md).

## Remarks

TODO_LINK_VFS_README.

## Related Pages

[cf_fs_free_enumerated_directory](/file/cf_fs_free_enumerated_directory.md)  
[cf_fs_create_directory](/file/cf_fs_create_directory.md)  
[cf_fs_enumerate_directory](/file/cf_fs_enumerate_directory.md)  