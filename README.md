# BDF Spectrum Visualization Tool

A powerful command-line tool for calculating and visualizing spectra from BDF (Beijing Density Functional) quantum chemistry software output files. The tool supports multiple spectrum types with publication-ready plots and flexible Python-based configuration.

## Features

- **Multiple Spectrum Types**: Absorption, emission, circular dichroism (CD) spectra
- **Flexible Units**: Wavelength (nm), energy (eV), wavenumber (cm⁻¹)
- **Multiple Output Formats**: SVG, PNG, JPG, EPS, PDF
- **Multi-Spectrum Plots**: Compare multiple spectra with different colors and custom legends
- **Python Configuration**: Smart configuration with conditional expressions
- **Publication Ready**: Scholarly styling with proper axis formatting, no background grid
- **Interactive Viewer**: Optional interactive plot window

## Prerequisites

### System Dependencies

**Ubuntu/Debian:**
```bash
sudo apt-get update
sudo apt-get install cmake clang ninja-build python3-dev python3-distutils
sudo apt-get install libvtk9-dev vtk9
```

**RHEL/Fedora:**
```bash
sudo yum install cmake clang ninja-build python3-devel vtk-devel
# or for newer versions:
sudo dnf install cmake clang ninja-build python3-devel vtk-devel
```

**macOS:**

**Install Homebrew if not already installed**
```bash
# Install Homebrew if not already installed
/bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"

# Install dependencies
brew install cmake ninja python3 vtk
# Apple Clang is included with Xcode command line tools
xcode-select --install
```

**Windows:**
- Install Visual Studio with C++ support or LLVM Clang
- Install Python 3.8+ from python.org
- Install Ninja: `choco install ninja` or download from GitHub
- Install VTK (recommend using vcpkg)

## Building from Source

1. **Clone the repository:**
   ```bash
   git clone https://github.com/shiyuwang123/BDF-spectrum-visualize-tool.git
   cd BDF-spectrum-visualize-tool
   ```

2. **Create build directory:**
   ```bash
   mkdir build
   cd build
   ```

3. **Configure and build:**

   **Option A: Using ccmake (recommended for configuration):**
   ```bash
   ccmake .. -G Njnja
   # Press 'c' to configure, 't' for advanced options, 'g' to generate
   ninja
   ```

   **Option B: Command line with Ninja:**
   ```bash
   cmake -G Ninja -DCMAKE_CXX_COMPILER=clang++ ..
   ninja
   ```

4. **Install (optional):**
   ```bash
   sudo cmake --install .
   ```

### Troubleshooting Build Issues

**If Python3 not found:**
```bash
cmake -DPython3_EXECUTABLE=/usr/bin/python3 -DCMAKE_CXX_COMPILER=clang++ ..
```

**If VTK not found:**
```bash
cmake -DVTK_DIR=/path/to/vtk/lib/cmake/vtk-9.x -DCMAKE_CXX_COMPILER=clang++ ..
```

**Using ccmake for interactive configuration:**
```bash
ccmake ..
# Navigate with arrow keys, press Enter to edit values
# Set CMAKE_CXX_COMPILER to clang++
# Set VTK_DIR if needed
# Press 'c' to configure, 'g' to generate
```

**Check CMake configuration:**
```bash
cmake .. -DPython3_FIND_STRATEGY=LOCATION -DCMAKE_CXX_COMPILER=clang++
```

## Configuration

### Creating Configuration File

The tool uses Python configuration files for all plotting parameters. Create one of these files:

1. `./spectrum_config.py` (in current directory)
2. `~/spectrum_config.py` (in home directory)
3. Custom file specified with `-config=path`

### Basic Configuration Example

Create `spectrum_config.py`:

```python
# Basic absorption spectrum configuration
mode = 'abs'                    # 'abs', 'emi', 'cd', 'cdl'
unit = 'nm'                     # 'nm', 'eV', 'cm-1'
x_start = 200                   # Start of spectral range
x_end = 800                     # End of spectral range
interval = 1.0                  # Grid interval
fwhm_ev = 0.3                   # FWHM in eV
output_format = 'svg'           # 'svg', 'png', 'jpg', 'eps', 'pdf'
output_filename = 'uv_vis_spectrum'
legend_names = ['Sample A', 'Sample B']
```

### Advanced Configuration with Conditionals

```python
import os

# Auto-detect experiment type from directory
current_dir = os.path.basename(os.getcwd())

# Smart mode selection
if 'emission' in current_dir.lower():
    mode = 'emi'
    x_start, x_end = 300, 700
    fwhm_ev = 0.2
elif 'cd' in current_dir.lower():
    mode = 'cd'
    x_start, x_end = 180, 400
    fwhm_ev = 0.3
else:
    mode = 'abs'
    x_start, x_end = 200, 800
    fwhm_ev = 0.3

unit = 'nm'
interval = 1.0

# Publication vs presentation settings
if 'publication' in current_dir.lower():
    output_format = 'eps'
    interval = 0.5  # Higher resolution
elif 'presentation' in current_dir.lower():
    output_format = 'png'

# Smart legend naming
if 'comparison' in current_dir.lower():
    legend_names = ['Control', 'Treatment 1', 'Treatment 2']
elif 'kinetic' in current_dir.lower():
    legend_names = ['t=0min', 't=5min', 't=10min', 't=30min']
elif 'ph' in current_dir.lower():
    legend_names = ['pH 6', 'pH 7', 'pH 8', 'pH 9']
else:
    legend_names = []  # Will use filenames
```

## Usage

### Command Line Syntax

```bash
plotspec [options] file1.out file2.out ...
```

### Basic Examples

**Single spectrum:**
```bash
./plotspec sample.out
```

**Multiple spectra comparison:**
```bash
./plotspec control.out treated1.out treated2.out
```

**With custom config:**
```bash
./plotspec -config=my_config.py sample1.out sample2.out
```

**Disable interactive viewer:**
```bash
./plotspec -no-interactive sample.out
```

### Configuration Examples for Different Spectroscopy Types

#### UV-Vis Absorption
```python
mode = 'abs'
unit = 'nm'
x_start = 200
x_end = 800
interval = 1.0
fwhm_ev = 0.3
output_format = 'svg'
```

#### Fluorescence Emission
```python
mode = 'emi'
unit = 'nm'
x_start = 300
x_end = 700
interval = 1.0
fwhm_ev = 0.2
output_format = 'png'
```

#### Circular Dichroism
```python
mode = 'cd'
unit = 'nm'
x_start = 180
x_end = 400
interval = 0.5
fwhm_ev = 0.3
output_format = 'eps'
```

#### High-Resolution Energy Domain
```python
mode = 'abs'
unit = 'eV'
x_start = 1.5
x_end = 6.0
interval = 0.005
fwhm_ev = 0.1
output_format = 'pdf'
```

### Environment Variables

Set environment variables for special modes:

```bash
# High resolution mode
export SPECTRUM_HIGH_RES=1
./plotspec sample.out

# Publication mode
export SPECTRUM_PUBLICATION=1
./plotspec paper_data.out
```

## Workflow Examples

### Basic Research Workflow

1. **Build the tool:**
   ```bash
   mkdir build && cd build
   ccmake ..  # Configure interactively
   ninja      # Fast parallel build
   ```

2. **Setup configuration once:**
   ```bash
   cd my_project/
   nano spectrum_config.py
   ```

3. **Calculate multiple spectra:**
   ```bash
   ./plotspec experiment1.out
   ./plotspec control.out treated.out
   ./plotspec time_series/*.out
   ```

### Publication Workflow

1. **Create publication config:**
   ```python
   # pub_config.py
   mode = 'abs'
   unit = 'nm'
   x_start = 250
   x_end = 600
   interval = 0.2
   fwhm_ev = 0.1
   output_format = 'eps'
   output_filename = 'figure_1'
   legend_names = ['Compound 1', 'Compound 2']
   ```

2. **Generate publication plots:**
   ```bash
   ./plotspec -config=pub_config.py -no-interactive compound1.out compound2.out
   ```

### Comparative Study Workflow

1. **Auto-configured comparison:**
   ```bash
   cd comparison_study/
   # Config auto-detects comparison mode
   ./plotspec control.out low_dose.out high_dose.out
   ```

2. **Time series analysis:**
   ```bash
   cd kinetic_study/
   # Config auto-generates time-based legends
   ./plotspec t0.out t5min.out t10min.out t30min.out
   ```

## Build System Details

### C++ Compiler Requirements
- **Minimum**: C++17 standard support
- **Recommended**: Clang++ or Apple Clang (better error messages, faster compilation)
- **Alternative**: GCC 7+ also supported

### Recommended Build Tools
- **Compiler**: Clang++ (better error messages and standards compliance)
- **Build System**: Ninja (faster parallel builds than Make)
- **Configuration**: ccmake (interactive CMake interface)

### Build Performance Tips
```bash
# Use Ninja for faster builds
cmake -G Ninja -DCMAKE_CXX_COMPILER=clang++ ..
ninja -j$(nproc)

# Use ccache for faster rebuilds (if available)
sudo apt-get install ccache  # Install ccache first
cmake -DCMAKE_CXX_COMPILER_LAUNCHER=ccache -G Ninja ..
ninja

# Debug vs Release builds
cmake -DCMAKE_BUILD_TYPE=Release -G Ninja ..  # Optimized
cmake -DCMAKE_BUILD_TYPE=Debug -G Ninja ..    # Debug symbols
```

### Interactive Configuration with ccmake
```bash
ccmake ..
```
**ccmake Navigation:**
- Arrow keys: Navigate options
- Enter: Edit value
- 'c': Configure
- 't': Toggle advanced mode
- 'g': Generate and exit
- 'q': Quit without generating

**Important CMake Variables:**
- `CMAKE_CXX_COMPILER`: Set to `clang++`
- `CMAKE_BUILD_TYPE`: `Release` or `Debug`
- `VTK_DIR`: Path to VTK installation
- `Python3_EXECUTABLE`: Path to Python3

## Output Files

The tool generates:
- **Plot file**: `output_filename.format` (e.g., `spectrum_plot.svg`)
- **Interactive window**: (if not disabled with `-no-interactive`)

## Advanced Features

### Custom Legend Names
```python
legend_names = ['Control', 'Treatment A', 'Treatment B']
```

### Multiple File Types
Input files can be:
- `.out` files (BDF output)
- Files without extension (will try `.out`)

## Troubleshooting

### Common Issues

**Config file not found:**
```
Error: Config file not found. Please create spectrum_config.py in current directory or home directory.
```
**Solution:** Create a config file or specify path with `-config=`

**Python import errors:**
```
Failed to import config file
```
**Solution:** Check Python syntax in config file

### Getting Help

```bash
./plotspec -help
```

## Contributing

1. Fork the repository
2. Create a feature branch
3. Make your changes
4. Test with different spectrum types
5. Submit a pull request

## License

This project is licensed under the MIT License - see the LICENSE file for details.

## Citation

If you use this tool in your research, please cite some papers regarding BDF:

## Support

For issues and questions:
- Open an issue on GitHub
- Check the troubleshooting section above
- Review example configurations
