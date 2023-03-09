[](../header.md ':include')

# cf_clear_color

Category: [graphics](/api_reference?id=graphics)  
GitHub: [cute_graphics.h](https://github.com/RandyGaul/cute_framework/blob/master/include/cute_graphics.h)  
---

Sets the color used when clearing a canvas.

```cpp
void cf_clear_color(float red, float green, float blue, float alpha);
```

## Remarks

This will get used when [cf_apply_canvas](/graphics/cf_apply_canvas.md) or when [cf_app_draw_onto_screen](/app/cf_app_draw_onto_screen.md) is called.

## Related Pages

[cf_app_draw_onto_screen](/app/cf_app_draw_onto_screen.md)  
[cf_clear_color2](/graphics/cf_clear_color2.md)  
[cf_clear_depth_stencil](/graphics/cf_clear_depth_stencil.md)  
[cf_apply_canvas](/graphics/cf_apply_canvas.md)  