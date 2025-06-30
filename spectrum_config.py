# BDF Spectrum Visualization Tool Configuration File
# This file contains all plotting parameters for the spectrum tool
# Only input filenames need to be passed via command line

import os
import sys

# You can use Python expressions and conditionals in this config file
# This allows for dynamic configuration based on environment, file names, etc.

# Detect spectrum type based on current directory or environment
current_dir = os.path.abspath(os.getcwd())
print(current_dir)
is_uv_vis = 'uv' in current_dir.lower() or 'vis' in current_dir.lower()
is_emission = 'emission' in current_dir.lower() or 'fluor' in current_dir.lower()
is_cd = 'cd' in current_dir.lower() or 'circular' in current_dir.lower()

# Conditional mode selection
if is_emission:
    mode = 'emi'
elif is_cd:
    mode = 'cd'
else:
    mode = 'abs'  # Default to absorption

# Conditional unit and range settings based on mode
if mode == 'abs':
    if is_uv_vis:
        unit = 'nm'
        x_start = 200
        x_end = 800
        interval = 1.0
        fwhm_ev = 0.3
    else:
        # General absorption settings
        unit = 'nm'
        x_start = 200
        x_end = 1000
        interval = 1.0
        fwhm_ev = 0.5
elif mode == 'emi':
    unit = 'nm'
    x_start = 300
    x_end = 700
    interval = 1.0
    fwhm_ev = 0.2
elif mode in ['cd', 'cdl']:
    unit = 'nm'
    x_start = 180
    x_end = 400
    interval = 0.5
    fwhm_ev = 0.3

# Conditional output settings
output_format = 'svg'  # Default

# Check if we're in a publication directory
if 'publication' in current_dir.lower() or 'paper' in current_dir.lower():
    output_format = 'eps'  # Better for publications
elif 'presentation' in current_dir.lower():
    output_format = 'png'  # Better for presentations

# Dynamic output filename based on mode and directory
if mode == 'abs':
    output_filename = f'absorption_spectrum_{current_dir}'
elif mode == 'emi':
    output_filename = f'emission_spectrum_{current_dir}'
elif mode in ['cd', 'cdl']:
    output_filename = f'cd_spectrum_{current_dir}'
else:
    output_filename = 'spectrum_plot'

# Legend settings with conditional expressions
# You can define different legend sets for different scenarios
if 'comparison' in current_dir.lower():
    legend_names = ['Control', 'Treatment 1', 'Treatment 2', 'Treatment 3']
elif 'time' in current_dir.lower() or 'kinetic' in current_dir.lower():
    legend_names = ['t=0min', 't=5min', 't=10min', 't=30min', 't=60min']
elif 'concentration' in current_dir.lower() or 'conc' in current_dir.lower():
    legend_names = ['0.1 mM', '0.5 mM', '1.0 mM', '5.0 mM', '10.0 mM']
elif 'ph' in current_dir.lower():
    legend_names = ['pH 6', 'pH 7', 'pH 8', 'pH 9']
elif 'temperature' in current_dir.lower() or 'temp' in current_dir.lower():
    legend_names = ['25°C', '35°C', '45°C', '55°C', '65°C']
else:
    # Default legend names - will be overridden by filenames if count doesn't match
    legend_names = ['Sample A', 'Sample B', 'Sample C']

# Advanced conditional settings
# You can even read environment variables or external files
if os.getenv('SPECTRUM_HIGH_RES'):
    # High resolution mode
    if unit == 'nm':
        interval = 0.5
    elif unit == 'eV':
        interval = 0.005
    elif unit == 'cm-1':
        interval = 2.0

# Publication-ready settings
if os.getenv('SPECTRUM_PUBLICATION'):
    output_format = 'svg'
    fwhm_ev = 0.1  # Sharper peaks for publication
    if unit == 'nm':
        interval = 0.2  # Higher resolution

# Example of reading from external configuration
external_config_file = os.path.expanduser('~/.spectrum_user_config.py')
if os.path.exists(external_config_file):
    # Execute user's personal config to override defaults
    exec(open(external_config_file).read())

# Validation and error checking
valid_modes = ['abs', 'emi', 'cd', 'cdl']
valid_units = ['nm', 'eV', 'cm-1']
valid_formats = ['svg', 'png', 'jpg', 'jpeg', 'eps', 'pdf']

if mode not in valid_modes:
    mode = 'abs'  # Fallback to safe default

if unit not in valid_units:
    unit = 'nm'  # Fallback to safe default

if output_format not in valid_formats:
    output_format = 'svg'  # Fallback to safe default

# Ensure reasonable ranges
if x_start >= x_end:
    if unit == 'nm':
        x_start, x_end = 200, 1000
    elif unit == 'eV':
        x_start, x_end = 1.0, 6.0
    elif unit == 'cm-1':
        x_start, x_end = 400, 4000

if fwhm_ev <= 0:
    fwhm_ev = 0.5

if interval <= 0:
    if unit == 'nm':
        interval = 1.0
    elif unit == 'eV':
        interval = 0.01
    elif unit == 'cm-1':
        interval = 100.0

# Example configurations for common scenarios:
"""
# For UV-Vis absorption (200-800 nm):
mode = 'abs'
unit = 'nm'
x_start = 200
x_end = 800
interval = 1.0
fwhm_ev = 0.3
output_format = 'svg'
legend_names = ['Compound A', 'Compound B']

# For fluorescence emission:
mode = 'emi'
unit = 'nm'
x_start = 300
x_end = 700
interval = 1.0
fwhm_ev = 0.2
output_format = 'png'
legend_names = ['Ex 280nm', 'Ex 320nm']

# For circular dichroism:
mode = 'cd'
unit = 'nm'
x_start = 180
x_end = 400
interval = 0.5
fwhm_ev = 0.3
output_format = 'eps'
legend_names = ['Native', 'Denatured']

# For energy domain plots:
mode = 'abs'
unit = 'eV'
x_start = 1.5
x_end = 6.0
interval = 0.01
fwhm_ev = 0.1
output_format = 'pdf'

# For high-resolution publication plots:
mode = 'abs'
unit = 'nm'
x_start = 250
x_end = 600
interval = 0.2
fwhm_ev = 0.1
output_format = 'eps'
"""

# Print configuration summary (will be shown when config is loaded)
print(f"Spectrum config loaded: mode={mode}, unit={unit}, range={x_start}-{x_end}, format={output_format}")
