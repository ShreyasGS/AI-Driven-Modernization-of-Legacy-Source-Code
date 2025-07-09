# Mozilla 1.0 Codebase Modernization Opportunities

Report generated on: 2025-07-09 11:42:18

## Summary

Analyzed 404 files with modernization opportunities across 1 components.

## Overall Statistics

| Pattern | Occurrences | Description | Modernization Approach |
|---------|-------------|-------------|------------------------|
| error_code_return | 2022 | Error code return | Use result types or exceptions |
| c_style_cast | 1744 | C-style cast | Use static_cast, dynamic_cast, or const_cast |
| error_check | 1429 | Error code check | Use result types with explicit success/failure |
| null_check | 1074 | Null pointer check | Use std::optional or result type |
| raw_new | 645 | Raw new operator | Use smart pointers (std::unique_ptr, RefPtr) |
| out_parameter | 587 | Out parameter | Return value or result type |
| query_interface | 393 | Manual QueryInterface call | Use do_QueryInterface or as<T>() |
| addref_call | 301 | Manual AddRef call | Use RefPtr or nsCOMPtr |
| release_call | 300 | Manual Release call | Use RefPtr or nsCOMPtr |
| manual_deallocation | 142 | Manual memory deallocation | Replace with smart pointers or RAII pattern |
| raw_delete | 135 | Raw delete operator | Use smart pointers (std::unique_ptr, RefPtr) |
| array_delete | 96 | Raw array deallocation | Use std::vector or other container |
| multiple_out_params | 90 | Multiple out parameters | Return std::tuple or custom struct |
| array_new | 72 | Raw array allocation | Use std::vector or other container |
| manual_allocation | 67 | Manual memory allocation | Replace with smart pointers or RAII pattern |
| default_opportunity | 38 | Empty constructor/destructor | Use = default |

## Component Breakdown


### xpcom (404 files)

| Pattern | Occurrences | Description | Modernization Approach |
|---------|-------------|-------------|------------------------|
| error_code_return | 2022 | Error code return | Use result types or exceptions |
| c_style_cast | 1744 | C-style cast | Use static_cast, dynamic_cast, or const_cast |
| error_check | 1429 | Error code check | Use result types with explicit success/failure |
| null_check | 1074 | Null pointer check | Use std::optional or result type |
| raw_new | 645 | Raw new operator | Use smart pointers (std::unique_ptr, RefPtr) |
| out_parameter | 587 | Out parameter | Return value or result type |
| query_interface | 393 | Manual QueryInterface call | Use do_QueryInterface or as<T>() |
| addref_call | 301 | Manual AddRef call | Use RefPtr or nsCOMPtr |
| release_call | 300 | Manual Release call | Use RefPtr or nsCOMPtr |
| manual_deallocation | 142 | Manual memory deallocation | Replace with smart pointers or RAII pattern |
| raw_delete | 135 | Raw delete operator | Use smart pointers (std::unique_ptr, RefPtr) |
| array_delete | 96 | Raw array deallocation | Use std::vector or other container |
| multiple_out_params | 90 | Multiple out parameters | Return std::tuple or custom struct |
| array_new | 72 | Raw array allocation | Use std::vector or other container |
| manual_allocation | 67 | Manual memory allocation | Replace with smart pointers or RAII pattern |
| default_opportunity | 38 | Empty constructor/destructor | Use = default |

## Top 5 Files with Most Modernization Opportunities

| File | Component | Total Opportunities | Top Patterns |
|------|-----------|---------------------|-------------|
| xpcom/components/nsComponentManager.cpp | xpcom | 434 | error_check (124), error_code_return (118), null_check (57) |
| xpcom/io/nsFastLoadFile.cpp | xpcom | 332 | error_check (133), error_code_return (92), query_interface (38) |
| xpcom/io/nsLocalFileMac.cpp | xpcom | 304 | error_code_return (125), error_check (97), null_check (30) |
| xpcom/io/nsLocalFileWin.cpp | xpcom | 248 | error_code_return (110), error_check (61), null_check (25) |
| xpcom/ds/nsVariant.cpp | xpcom | 229 | error_code_return (117), null_check (39), error_check (20) |
