# Mozilla 1.0 Modernization Templates

This directory contains templates for common modernization patterns identified in the Mozilla 1.0 codebase. Each template provides a detailed guide on how to apply a specific modernization pattern.

## Available Templates

1. [Error Code Returns → Result Types](error_code_result_type.md)
   - Replaces error code returns and out parameters with a Result type
   - Addresses the most common pattern found in our baseline (15,486 occurrences)

2. [Raw Pointers → Smart Pointers](raw_ptr_to_smart_ptr.md)
   - Replaces raw pointers with appropriate smart pointers
   - Addresses memory management issues (3,245 raw new occurrences)

3. [C-style Casts → Safe Casts](c_style_cast_to_safe_cast.md)
   - Replaces C-style casts with safer alternatives (static_cast, dynamic_cast)
   - Addresses 5,098 occurrences found in our baseline

4. [Out Parameters → Return Values](out_param_to_return_value.md)
   - Replaces out parameters with return values
   - Addresses 5,027 occurrences found in our baseline

## Planned Templates

5. Manual Reference Counting → RefPtr/nsCOMPtr
   - Replaces manual AddRef/Release calls with smart pointers
   - Will address 2,832 occurrences found in our baseline

## How to Use These Templates

1. **Identify the Pattern**: Use the modernization finder tool to identify instances of these patterns in your code
2. **Select the Appropriate Template**: Choose the template that matches the pattern you want to modernize
3. **Follow the Step-by-Step Guide**: Each template includes detailed instructions on how to apply the pattern
4. **Consider Compatibility**: Pay attention to the compatibility considerations in each template

## Contributing New Templates

To contribute a new modernization template:

1. Identify a common pattern that could benefit from modernization
2. Create a markdown file with the following sections:
   - Pattern Overview
   - Before and After Example
   - Step-by-Step Implementation Guide
   - Compatibility Considerations
   - Benefits
3. Add the template to this README file

## Related Resources

- [Modernization Process Documentation](../modernization_process_documentation.md)
- [Baseline Measurements](../analysis/reports/baseline/summary.md)
- [Modernization Finder Tool](../analysis/modernization_finder.py) 