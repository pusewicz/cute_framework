[//]: # (This file is automatically generated by Cute Framework's docs parser.)
[//]: # (Do not edit this file by hand!)
[//]: # (See: https://github.com/RandyGaul/cute_framework/blob/master/samples/docs_parser.cpp)
[](../header.md ':include')

# cf_make_material

Category: [graphics](/api_reference?id=graphics)  
GitHub: [cute_graphics.h](https://github.com/RandyGaul/cute_framework/blob/master/include/cute_graphics.h)  
---

Creates a new material.

```cpp
CF_Material cf_make_material();
```

## Remarks

A material holds render state (see [CF_RenderState](/graphics/cf_renderstate.md)), texture inputs (see [CF_Texture](/graphics/cf_texture.md)), as well as shader inputs called
uniforms (see [CF_UniformType](/graphics/cf_uniformtype.md)). For an overview see [CF_Material](/graphics/cf_material.md).

## Related Pages

[CF_UniformType](/graphics/cf_uniformtype.md)  
[CF_Material](/graphics/cf_material.md)  
[cf_material_set_uniform_fs](/graphics/cf_material_set_uniform_fs.md)  
[cf_destroy_material](/graphics/cf_destroy_material.md)  
[cf_material_set_render_state](/graphics/cf_material_set_render_state.md)  
[cf_material_set_texture_vs](/graphics/cf_material_set_texture_vs.md)  
[cf_material_set_texture_fs](/graphics/cf_material_set_texture_fs.md)  
[cf_material_set_uniform_vs](/graphics/cf_material_set_uniform_vs.md)  
