[](../header.md ':include')

# spnorm

Category: [path](/api_reference?id=path)  
GitHub: [cute_file_system.h](https://github.com/RandyGaul/cute_framework/blob/master/include/cute_file_system.h)  
---

Normalizes a path as a new string.

```cpp
#define spnorm(s) cf_path_normalize(s)
```

Parameters | Description
--- | ---
s | The path string.

## Remarks

All '\\' are replaced with '/'. Any duplicate '////' are replaced with a single '/'. Trailing '/' are removed. Dot folders are resolved, e.g.
```
spnorm("/a/b/./c") -> "/a/b/c"
spnorm("/a/b/../c") -> "/a/c"
```
The first character is always '/', unless it's a windows drive, e.g.
```
spnorm("C:\\Users\\Randy\\Documents") -> "C:/Users/Randy/Documents"
```

## Related Pages

[spfname](/path/spfname.md)  
[spfname_no_ext](/path/spfname_no_ext.md)  
[spext](/path/spext.md)  
[spext_equ](/path/spext_equ.md)  
[sppop](/path/sppop.md)  
[sppopn](/path/sppopn.md)  
[spcompact](/path/spcompact.md)  
[spdir_of](/path/spdir_of.md)  
[sptop_of](/path/sptop_of.md)  