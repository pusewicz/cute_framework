[//]: # (This file is automatically generated by Cute Framework's docs parser.)
[//]: # (Do not edit this file by hand!)
[//]: # (See: https://github.com/RandyGaul/cute_framework/blob/master/samples/docs_parser.cpp)
[](../header.md ':include')

# cf_memory_pool_try_alloc

Category: [allocator](/api_reference?id=allocator)  
GitHub: [cute_alloc.h](https://github.com/RandyGaul/cute_framework/blob/master/include/cute_alloc.h)  
---

Allocates a chunk of memory from the pool. The allocation size was determined by `element_size` in [cf_make_memory_pool](/allocator/cf_make_memory_pool.md).

```cpp
void* cf_memory_pool_try_alloc(CF_MemoryPool* pool);
```

Parameters | Description
--- | ---
pool | The pool.

## Return Value

Returns an aligned pointer of `size` bytes.

## Remarks

Does not return an allocation if the internal pool is full, and will instead return `NULL` in this case. See
[cf_memory_pool_alloc](/allocator/cf_memory_pool_alloc.md) for more details about overflowing the pool to use `malloc` as a backup.

## Related Pages

[cf_make_memory_pool](/allocator/cf_make_memory_pool.md)  
[cf_destroy_memory_pool](/allocator/cf_destroy_memory_pool.md)  
[cf_memory_pool_alloc](/allocator/cf_memory_pool_alloc.md)  
[cf_memory_pool_free](/allocator/cf_memory_pool_free.md)  
