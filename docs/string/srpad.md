[](../header.md ':include')

# srpad

Category: [string](/api_reference?id=string)  
GitHub: [cute_string.h](https://github.com/RandyGaul/cute_framework/blob/master/include/cute_string.h)  
---

Appends n characters `ch` onto the end of the string.

```cpp
#define srpad(s, ch, n) cf_string_rpad(s, ch, n)
```

Parameters | Description
--- | ---
s | The string. Can be `NULL`.
ch | A character to insert.
n | Number of times to insert `ch`.

## Related Pages

[strim](/string/strim.md)  
[sltrim](/string/sltrim.md)  
[srtrim](/string/srtrim.md)  
[slpad](/string/slpad.md)  
[serase](/string/serase.md)  
[sdedup](/string/sdedup.md)  
[sreplace](/string/sreplace.md)  