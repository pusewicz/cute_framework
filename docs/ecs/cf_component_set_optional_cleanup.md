[//]: # (This file is automatically generated by Cute Framework's docs parser.)
[//]: # (Do not edit this file by hand!)
[//]: # (See: https://github.com/RandyGaul/cute_framework/blob/master/samples/docs_parser.cpp)
[](../header.md ':include')

# cf_component_set_optional_cleanup

Category: [ecs](/api_reference?id=ecs)  
GitHub: [cute_ecs.h](https://github.com/RandyGaul/cute_framework/blob/master/include/cute_ecs.h)  
---

Sets an optional cleanup callback, called whenever a component of this type is free'd.

```cpp
void cf_component_set_optional_cleanup(CF_ComponentFn* cleanup, void* udata);
```

## Related Pages

[cf_component_begin](/ecs/cf_component_begin.md)  
[cf_component_set_name](/ecs/cf_component_set_name.md)  
[cf_component_set_size](/ecs/cf_component_set_size.md)  
[cf_component_set_optional_initializer](/ecs/cf_component_set_optional_initializer.md)  
[cf_component_end](/ecs/cf_component_end.md)  
