[](../header.md ':include')

# cf_expand_aabb_f

Category: [math](/api_reference?id=math)  
GitHub: [cute_math.h](https://github.com/RandyGaul/cute_framework/blob/master/include/cute_math.h)  
---

Expands an AABB (axis-aligned bounding box).

```cpp
CF_Aabb cf_expand_aabb_f(CF_Aabb aabb, float v)
```

## Remarks

`v` is added to to `max.x` and `max.y` of `aabb`, and subtracted from `min.x` and `min.y` of `aabb`.

## Related Pages

[CF_Aabb](/math/cf_aabb.md)  
[cf_make_aabb](/math/cf_make_aabb.md)  
[cf_expand_aabb](/math/cf_expand_aabb.md)  