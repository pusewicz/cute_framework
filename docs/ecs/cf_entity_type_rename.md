[//]: # (This file is automatically generated by Cute Framework's docs parser.)
[//]: # (Do not edit this file by hand!)
[//]: # (See: https://github.com/RandyGaul/cute_framework/blob/master/samples/docs_parser.cpp)
[](../header.md ':include')

# cf_entity_type_rename

Category: [ecs](/api_reference?id=ecs)  
GitHub: [cute_ecs.h](https://github.com/RandyGaul/cute_framework/blob/master/include/cute_ecs.h)  
---

Changes the string identifier for an entity type.

```cpp
void cf_entity_type_rename(const char* entity_type, const char* new_entity_type_name);
```

## Remarks

This function can be useful for implementing certain editors.

## Related Pages

[cf_entity_delayed_change_type](/ecs/cf_entity_delayed_change_type.md)  
[cf_entity_change_type](/ecs/cf_entity_change_type.md)  
entity_get_type_string  
