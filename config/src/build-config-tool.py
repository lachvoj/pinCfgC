#!/usr/bin/env python3
"""
pinCfgC Config Tool Builder
Combines separate HTML, CSS, and JavaScript files into a single HTML file.
Automatically extracts V_TYPES and S_TYPES from MySensors header.
"""

import os
import sys
import re

def read_file(filepath):
    """Read file contents."""
    try:
        with open(filepath, 'r', encoding='utf-8') as f:
            return f.read()
    except FileNotFoundError:
        print(f"Error: File not found: {filepath}")
        sys.exit(1)
    except Exception as e:
        print(f"Error reading {filepath}: {e}")
        sys.exit(1)

def write_file(filepath, content):
    """Write content to file."""
    try:
        with open(filepath, 'w', encoding='utf-8') as f:
            f.write(content)
        print(f"Successfully created: {filepath}")
    except Exception as e:
        print(f"Error writing {filepath}: {e}")
        sys.exit(1)

def parse_enum(content, enum_name, start_marker):
    """Parse an enum from C header file."""
    # Find the enum block
    pattern = rf'{start_marker}\s*{{\s*(.*?)\s*}}\s*{enum_name};'
    match = re.search(pattern, content, re.DOTALL)
    
    if not match:
        print(f"Warning: Could not find {enum_name} enum")
        return {}
    
    enum_content = match.group(1)
    
    # Parse enum entries: NAME = VALUE, //!< Description
    entries = {}
    pattern = r'([A-Z_][A-Z0-9_]*)\s*=\s*(\d+)\s*,?\s*(?://!<\s*(.*))?'
    
    for match in re.finditer(pattern, enum_content):
        name = match.group(1)
        value = match.group(2)
        description = match.group(3).strip() if match.group(3) else ""
        
        # Skip deprecated entries (they have same values)
        if '\\deprecated' not in description:
            entries[value] = {
                'name': name,
                'description': description
            }
    
    return entries

def generate_mysensors_types_js(mysensors_header_path):
    """Generate JavaScript code for MySensors V_TYPES and S_TYPES."""
    
    if not os.path.exists(mysensors_header_path):
        print(f"Warning: MySensors header not found: {mysensors_header_path}")
        print("Using fallback types...")
        return generate_fallback_types()
    
    try:
        # Read header file
        with open(mysensors_header_path, 'r', encoding='utf-8') as f:
            content = f.read()
        
        print(f"Parsing MySensors types from: {mysensors_header_path}")
        
        # Parse enums
        s_types = parse_enum(content, 'mysensors_sensor_t', 'typedef enum')
        v_types = parse_enum(content, 'mysensors_data_t', 'typedef enum')
        
        if not s_types or not v_types:
            print("Warning: Failed to parse MySensors types, using fallback")
            return generate_fallback_types()
        
        print(f"  Found {len(s_types)} sensor types (S_TYPE)")
        print(f"  Found {len(v_types)} variable types (V_TYPE)")
        
        # Generate JavaScript code
        js_code = []
        
        # Generate S_TYPES
        js_code.append("// MySensors sensor types (S_TYPE) - auto-generated from MyMessage.h")
        js_code.append("const S_TYPES = {")
        
        for value in sorted(s_types.keys(), key=lambda x: int(x)):
            entry = s_types[value]
            name = entry['name']
            desc = entry['description']
            # Clean up description - take first part before comma
            desc_short = desc.split(',')[0].strip() if desc else name
            js_code.append(f"    '{value}': '{name} - {desc_short}',")
        
        js_code.append("};")
        js_code.append("")
        
        # Generate V_TYPES
        js_code.append("// MySensors variable types (V_TYPE) - auto-generated from MyMessage.h")
        js_code.append("const V_TYPES = {")
        
        for value in sorted(v_types.keys(), key=lambda x: int(x)):
            entry = v_types[value]
            name = entry['name']
            desc = entry['description']
            # Clean up description - take first part before period
            desc_short = desc.split('.')[0].strip() if desc else name
            # Remove sensor type references like "S_TEMP."
            desc_short = re.sub(r'^[A-Z_]+\.\s*', '', desc_short)
            js_code.append(f"    '{value}': '{name} - {desc_short}',")
        
        js_code.append("};")
        
        return '\n'.join(js_code)
        
    except Exception as e:
        print(f"Error parsing MySensors header: {e}")
        print("Using fallback types...")
        return generate_fallback_types()

def parse_pincfg_defines(types_header_path):
    """Parse #define constants from Types.h for limits and defaults."""
    
    if not os.path.exists(types_header_path):
        print(f"Warning: Types.h not found: {types_header_path}")
        print("Using fallback constraints...")
        return generate_fallback_constraints()
    
    try:
        with open(types_header_path, 'r', encoding='utf-8') as f:
            content = f.read()
        
        print(f"Parsing pinCfgC constraints from: {types_header_path}")
        
        # Parse #define CONSTANT VALUE or #define CONSTANT VALUE /* comment */
        defines = {}
        pattern = r'#define\s+(PINCFG_[A-Z_0-9]+)\s+(\S+)(?:\s*/\*.*?\*/)?'
        
        for match in re.finditer(pattern, content):
            name = match.group(1)
            value = match.group(2)
            
            # Try to convert to number
            try:
                # Handle suffixes like LL
                value_clean = value.rstrip('LlUu')
                if value_clean.replace('.', '', 1).isdigit():
                    # Convert to int or float
                    if '.' in value_clean:
                        defines[name] = float(value_clean)
                    else:
                        defines[name] = int(value_clean)
                else:
                    defines[name] = value
            except:
                defines[name] = value
        
        print(f"  Found {len(defines)} pinCfgC defines")
        
        # Generate JavaScript constants object
        js_code = []
        js_code.append("// pinCfgC constraints and defaults - auto-generated from Types.h")
        js_code.append("const PINCFG_LIMITS = {")
        
        # Group related constants
        relevant_defines = [
            'PINCFG_CONFIG_MAX_SZ_D',
            'PINCFG_TRIGGER_MAX_SWITCHES_D',
            'PINCFG_DEBOUNCE_MS_D',
            'PINCFG_MULTICLICK_MAX_DELAY_MS_D',
            'PINCFG_SWITCH_IMPULSE_DURATIN_MS_D',
            'PINCFG_SWITCH_FB_ON_DELAY_MS_D',
            'PINCFG_SWITCH_FB_OFF_DELAY_MS_D',
            'PINCFG_SENSOR_SAMPLING_INTV_MIN_MS_D',
            'PINCFG_SENSOR_SAMPLING_INTV_MAX_MS_D',
            'PINCFG_SENSOR_SAMPLING_INTV_MS_D',
            'PINCFG_SENSOR_REPORTING_INTV_MIN_SEC_D',
            'PINCFG_SENSOR_REPORTING_INTV_MAX_SEC_D',
            'PINCFG_SENSOR_REPORTING_INTV_SEC_D',
            'PINCFG_SENSOR_SCALE_MIN_D',
            'PINCFG_SENSOR_SCALE_MAX_D',
            'PINCFG_SENSOR_SCALE_D',
            'PINCFG_SENSOR_OFFSET_MIN_D',
            'PINCFG_SENSOR_OFFSET_MAX_D',
            'PINCFG_SENSOR_OFFSET_D',
            'PINCFG_SENSOR_PRECISION_MIN_D',
            'PINCFG_SENSOR_PRECISION_MAX_D',
            'PINCFG_SENSOR_PRECISION_D',
            'PINCFG_TIMED_SWITCH_MIN_PERIOD_MS_D',
            'PINCFG_TIMED_SWITCH_MAX_PERIOD_MS_D',
        ]
        
        for def_name in relevant_defines:
            if def_name in defines:
                value = defines[def_name]
                # Convert name to more readable JS property
                # Remove PINCFG_ prefix and _D suffix (only at the end)
                js_name = def_name.replace('PINCFG_', '')
                if js_name.endswith('_D'):
                    js_name = js_name[:-2]  # Remove last 2 chars (_D)
                js_code.append(f"    {js_name}: {value},")
        
        js_code.append("};")
        
        return '\n'.join(js_code)
        
    except Exception as e:
        print(f"Error parsing Types.h: {e}")
        print("Using fallback constraints...")
        return generate_fallback_constraints()

def generate_fallback_constraints():
    """Generate fallback constraints if Types.h is not available."""
    return """// pinCfgC constraints and defaults - fallback
const PINCFG_LIMITS = {
    TRIGGER_MAX_SWITCHES: 5,
    DEBOUNCE_MS: 100,
    MULTICLICK_MAX_DELAY_MS: 500,
    SWITCH_IMPULSE_DURATIN_MS: 300,
    SWITCH_FB_ON_DELAY_MS: 1000,
    SWITCH_FB_OFF_DELAY_MS: 30000,
    SENSOR_SAMPLING_INTV_MIN_MS: 100,
    SENSOR_SAMPLING_INTV_MAX_MS: 5000,
    SENSOR_SAMPLING_INTV_MS: 1000,
    SENSOR_REPORTING_INTV_MIN_SEC: 1,
    SENSOR_REPORTING_INTV_MAX_SEC: 3600,
    SENSOR_REPORTING_INTV_SEC: 300,
    SENSOR_SCALE_MIN: -1000.0,
    SENSOR_SCALE_MAX: 1000.0,
    SENSOR_SCALE: 1.0,
    SENSOR_OFFSET_MIN: -1000.0,
    SENSOR_OFFSET_MAX: 1000.0,
    SENSOR_OFFSET: 0.0,
    SENSOR_PRECISION_MIN: 0,
    SENSOR_PRECISION_MAX: 6,
    SENSOR_PRECISION_DEFAULT: 0,
    TIMED_SWITCH_MIN_PERIOD_MS: 50,
    TIMED_SWITCH_MAX_PERIOD_MS: 600000,
};"""

def generate_fallback_types():
    """Generate fallback types if MySensors header is not available."""
    return """// MySensors sensor types (S_TYPE) - fallback
const S_TYPES = {
    '0': 'S_DOOR - Door sensor',
    '1': 'S_MOTION - Motion sensor',
    '4': 'S_BARO - Barometer sensor',
    '6': 'S_TEMP - Temperature sensor',
    '7': 'S_HUM - Humidity sensor',
    '16': 'S_LIGHT_LEVEL - Light level sensor',
    '30': 'S_MULTIMETER - Multimeter device',
    '35': 'S_MOISTURE - Moisture sensor',
    '37': 'S_MULTIMETER - Multimeter device',
    '99': 'S_CUSTOM - Custom sensor'
};

// MySensors variable types (V_TYPE) - fallback
const V_TYPES = {
    '0': 'V_TEMP - Temperature',
    '1': 'V_HUM - Humidity',
    '2': 'V_STATUS - Binary status',
    '3': 'V_LIGHT_LEVEL - Light level',
    '6': 'V_TEMP - Temperature',
    '17': 'V_PRESSURE - Pressure',
    '23': 'V_LIGHT_LEVEL - Light level',
    '37': 'V_LEVEL - Level',
    '38': 'V_VOLTAGE - Voltage',
    '39': 'V_CURRENT - Current',
    '99': 'V_CUSTOM - Custom'
};"""

def main():
    # Get script directory (config/src)
    script_dir = os.path.dirname(os.path.abspath(__file__))
    # Config directory (one level up)
    config_dir = os.path.dirname(script_dir)
    
    # Define file paths (all sources in config/src)
    template_file = os.path.join(script_dir, 'config-tool-template.html')
    css_file = os.path.join(script_dir, 'config-tool-styles.css')
    js_files = [
        os.path.join(script_dir, 'config-tool-script-part1.js'),
        os.path.join(script_dir, 'config-tool-script-part2.js'),
        os.path.join(script_dir, 'config-tool-script-part3.js'),
        os.path.join(script_dir, 'config-tool-script-part4.js'),
    ]
    # Output in config directory
    output_file = os.path.join(config_dir, 'config-tool.html')
    
    # MySensors header path (from config/src -> ../MySensors)
    # pinCfgC and MySensors are both in SW/submodules/
    mysensors_header = os.path.join(
        script_dir, '..', '..', '..', 'MySensors', 'core', 'MyMessage.h'
    )
    
    # Types.h path (from config/src -> ../src/Types.h)
    types_header = os.path.join(
        script_dir, '..', '..', 'src', 'Types.h'
    )
    
    print("Building pinCfgC Configuration Tool...")
    print(f"Template: {template_file}")
    print(f"CSS: {css_file}")
    print(f"JavaScript parts: {len(js_files)}")
    print(f"MySensors header: {mysensors_header}")
    print(f"Types.h header: {types_header}")
    print(f"Output: {output_file}")
    print()
    
    # Read template
    template = read_file(template_file)
    
    # Read CSS
    css_content = read_file(css_file)
    
    # Generate MySensors types
    print("Generating MySensors types...")
    mysensors_types_js = generate_mysensors_types_js(mysensors_header)
    print()
    
    # Parse pinCfgC constraints
    print("Parsing pinCfgC constraints...")
    pincfg_limits_js = parse_pincfg_defines(types_header)
    print()
    
    # Read and combine JavaScript files
    js_content = ""
    for js_file in js_files:
        print(f"Reading: {os.path.basename(js_file)}")
        js_part = read_file(js_file)
        
        # Replace hardcoded V_TYPES, S_TYPES, and PINCFG_LIMITS in part1 with generated ones
        if 'part1' in js_file:
            # Remove existing V_TYPES and S_TYPES definitions
            js_part = re.sub(
                r'// MySensors variable types.*?const V_TYPES = \{[^}]+\};',
                '',
                js_part,
                flags=re.DOTALL
            )
            js_part = re.sub(
                r'// MySensors sensor types.*?const S_TYPES = \{[^}]+\};',
                '',
                js_part,
                flags=re.DOTALL
            )
            # Remove existing PINCFG_LIMITS if present
            js_part = re.sub(
                r'// pinCfgC constraints.*?const PINCFG_LIMITS = \{[^}]+\};',
                '',
                js_part,
                flags=re.DOTALL
            )
            # Insert generated types and limits at the beginning
            js_part = mysensors_types_js + "\n\n" + pincfg_limits_js + "\n\n" + js_part
        
        js_content += js_part + "\n\n"
    
    # Replace placeholders
    final_html = template.replace('/* CSS_PLACEHOLDER */', css_content)
    final_html = final_html.replace('/* JAVASCRIPT_PLACEHOLDER */', js_content)
    
    # Write output file
    write_file(output_file, final_html)
    
    # Calculate sizes
    template_size = len(template)
    css_size = len(css_content)
    js_size = len(js_content)
    final_size = len(final_html)
    
    print()
    print("Build Summary:")
    print(f"  Template size: {template_size:,} bytes")
    print(f"  CSS size: {css_size:,} bytes")
    print(f"  JavaScript size: {js_size:,} bytes")
    print(f"  Final HTML size: {final_size:,} bytes ({final_size / 1024:.1f} KB)")
    print()
    print(f"✓ Build complete! Open {output_file} in your browser.")

if __name__ == '__main__':
    main()
