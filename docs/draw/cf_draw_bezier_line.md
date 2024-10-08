[//]: # (This file is automatically generated by Cute Framework's docs parser.)
[//]: # (Do not edit this file by hand!)
[//]: # (See: https://github.com/RandyGaul/cute_framework/blob/master/samples/docs_parser.cpp)
[](../header.md ':include')

# cf_draw_bezier_line

Category: [draw](/api_reference?id=draw)  
GitHub: [cute_draw.h](https://github.com/RandyGaul/cute_framework/blob/master/include/cute_draw.h)  
---

Draws line segments over a quadratic bezier line.

```cpp
void cf_draw_bezier_line(CF_V2 a, CF_V2 c0, CF_V2 b, int iters, float thickness);
```

Parameters | Description
--- | ---
a | The starting point.
c0 | A bezier control point.
b | The end point.
thickness | The thickness of the line to draw.
iters | The number of lines used to draw the bezier spline.

## Related Pages

[cf_draw_line](/draw/cf_draw_line.md)  
[cf_draw_polyline](/draw/cf_draw_polyline.md)  
[cf_draw_arrow](/draw/cf_draw_arrow.md)  
[cf_draw_bezier_line2](/draw/cf_draw_bezier_line2.md)  
