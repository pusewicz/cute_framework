[//]: # (This file is automatically generated by Cute Framework's docs parser.)
[//]: # (Do not edit this file by hand!)
[//]: # (See: https://github.com/RandyGaul/cute_framework/blob/master/samples/docs_parser.cpp)
[](../header.md ':include')

# cf_draw_pop_tint

Category: [draw](/api_reference?id=draw)  
GitHub: [cute_draw.h](https://github.com/RandyGaul/cute_framework/blob/master/include/cute_draw.h)  
---

Pops and returns the last tint color.

```cpp
CF_Color cf_draw_pop_tint();
```

## Remarks

Sprites and shapes can be tinted. This is useful for certain effects such as damage flashes.
Tint is implemented under the hood with an overlay operation. If you want to push a no-op, use
[cf_color_grey](/graphics/cf_color_grey.md) to apply no tinting at all.

## Related Pages

[cf_draw_push_tint](/draw/cf_draw_push_tint.md)  
[cf_draw_peek_tint](/draw/cf_draw_peek_tint.md)  
