[](../header.md ':include')

# scmp

Category: [string](/api_reference?id=string)  
GitHub: [cute_string.h](https://github.com/RandyGaul/cute_framework/blob/master/include/cute_string.h)  
---

Compares two strings.

```cpp
#define scmp(a, b) cf_string_cmp(a, b)
```

Parameters | Description
--- | ---
a | The first string.
b | The second string.

## Remarks

Returns 0 if the two strings are equivalent. Otherwise returns 1 if a[i] > b[i], or -1 if a[i] < b[i].

## Related Pages

[siequ](/string/siequ.md)  
[sicmp](/string/sicmp.md)  
[sequ](/string/sequ.md)  