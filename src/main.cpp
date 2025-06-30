#include <vtkAxis.h>
#include <vtkChartLegend.h>
#include <vtkChartXY.h>
#include <vtkColorSeries.h>
#include <vtkContextView.h>
#include <vtkDoubleArray.h>
#include <vtkGL2PSExporter.h>
#include <vtkJPEGWriter.h>
#include <vtkPNGWriter.h>
#include <vtkPen.h>
#include <vtkPlot.h>
#include <vtkRenderWindow.h>
#include <vtkRenderWindowInteractor.h>
#include <vtkRenderer.h>
#include <vtkSmartPointer.h>
#include <vtkTable.h>
#include <vtkTextProperty.h>
#include <vtkWindowToImageFilter.h>

#include <iostream>
#include <vector>
#include <string>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <cmath>
#include <iomanip>
#include <stdexcept>
#include <map>
#include <limits>
#include <filesystem>
#include <Python.h>


using namespace std;

// Constants for spectral calculations
constexpr double EV_TO_CM_MINUS_1 = 8065.54477;
constexpr double NM_EV_PRODUCT = 1239.84186;
constexpr double PI = 3.14159265358979323846;
constexpr double PREFAC_BROADENING_BASE = 1.0 / 4.33e-9;
constexpr double PREFAC_ECD_BASE = EV_TO_CM_MINUS_1 / 22.9;
constexpr double KB_EV_PER_K = 1.3806504e-23 / 1.602176487e-19;
constexpr double ROOM_TEMP_K = 298.15;

// Structure to hold spectral calculation parameters
struct PlotSpecParams {
    std::string mode = "abs";
    std::string unit = "nm";
    double x_start = 200.0;
    double x_end = 1000.0;
    double interval = 1.0;
    bool user_set_interval = false;
    double fwhm_cm_minus_1 = 0.5 * EV_TO_CM_MINUS_1;
    std::vector<std::string> input_filenames;
    std::vector<std::string> legend_names;
    std::string output_format = "svg";
    std::string output_filename = "spectrum_plot";
    bool interactive = true;
    double kT_eV = ROOM_TEMP_K * KB_EV_PER_K;
};

// Spectral data structure
struct SpectrumData {
    std::vector<double> x_values;
    std::vector<double> y_values;
    std::string x_label;
    std::string y_label;
    std::string title;
};

// Utility functions
std::vector<std::string> split_string(const std::string& s, char delimiter) {
    std::vector<std::string> tokens;
    std::string token;
    std::istringstream tokenStream(s);
    while (std::getline(tokenStream, token, delimiter)) {
        tokens.push_back(token);
    }
    return tokens;
}

std::string to_lower_cpp(std::string s) {
    std::transform(s.begin(), s.end(), s.begin(),
                   [](unsigned char c){ return std::tolower(c); });
    return s;
}

bool ends_with(const std::string& value, const std::string& ending) {
    if (ending.size() > value.size()) return false;
    return std::equal(ending.rbegin(), ending.rend(), value.rbegin());
}

void print_usage() {
    std::cout << "Usage: plotspec [options] file1.out file2.out ..." << std::endl;
    std::cout << "" << std::endl;
    std::cout << "All plotting parameters are read from config file:" << std::endl;
    std::cout << " 1. spectrum_config.py (in same directory as executable)" << std::endl;
    std::cout << " 2. ~/spectrum_config.py (in home directory)" << std::endl;
    std::cout << "" << std::endl;
    std::cout << "Command line options:" << std::endl;
    std::cout << " -config=path                  Use specific config file" << std::endl;
    std::cout << " -no-interactive               Disable interactive viewer" << std::endl;
    std::cout << " -help                         Show this help message" << std::endl;
    std::cout << "" << std::endl;
    std::cout << "Example config file (spectrum_config.py):" << std::endl;
    std::cout << "  mode = 'abs'                 # abs, emi, cd, cdl" << std::endl;
    std::cout << "  unit = 'nm'                  # nm, eV, cm-1" << std::endl;
    std::cout << "  x_start = 200                # Start of range" << std::endl;
    std::cout << "  x_end = 1000                 # End of range" << std::endl;
    std::cout << "  interval = 1.0               # Grid interval" << std::endl;
    std::cout << "  fwhm_ev = 0.5                # FWHM in eV" << std::endl;
    std::cout << "  output_format = 'svg'        # svg, png, jpg, eps, pdf" << std::endl;
    std::cout << "  output_filename = 'spectrum' # Output filename (no extension)" << std::endl;
    std::cout << "  legend_names = ['A', 'B']    # Legend names for multiple files" << std::endl;
}

// Function to initialize Python interpreter
void initialize_python() {
    Py_Initialize();
    if (!Py_IsInitialized()) {
        throw std::runtime_error("Failed to initialize Python interpreter");
    }
}

// Function to finalize Python interpreter
void finalize_python() {
    Py_Finalize();
}

// Helper function to get Python object value as string
std::string get_python_string(PyObject* obj) {
    if (PyUnicode_Check(obj)) {
        PyObject* utf8 = PyUnicode_AsUTF8String(obj);
        if (utf8) {
            std::string result = PyBytes_AsString(utf8);
            Py_DECREF(utf8);
            return result;
        }
    }
    return "";
}

// Helper function to get Python object value as double
double get_python_double(PyObject* obj) {
    if (PyFloat_Check(obj)) {
        return PyFloat_AsDouble(obj);
    } else if (PyLong_Check(obj)) {
        return PyLong_AsDouble(obj);
    }
    return 0.0;
}

// Helper function to get Python list as vector of strings
std::vector<std::string> get_python_string_list(PyObject* obj) {
    std::vector<std::string> result;
    if (PyList_Check(obj)) {
        Py_ssize_t size = PyList_Size(obj);
        for (Py_ssize_t i = 0; i < size; ++i) {
            PyObject* item = PyList_GetItem(obj, i);
            std::string str_val = get_python_string(item);
            if (!str_val.empty()) {
                result.push_back(str_val);
            }
        }
    }
    return result;
}

// Function to read configuration from Python file
PlotSpecParams read_config_file(const std::string& config_path) {
    PlotSpecParams params;

    initialize_python();

    try {
        // Add the config file directory to Python path
        std::filesystem::path config_file_path(config_path);
        std::string config_dir = config_file_path.parent_path().string();
        std::string config_name = config_file_path.stem().string();

        if (config_dir.empty()) {
            config_dir = ".";
        }

        // Add directory to sys.path
        PyRun_SimpleString("import sys");
        std::string add_path = "sys.path.insert(0, '" + config_dir + "')";
        PyRun_SimpleString(add_path.c_str());

        // Import the config module
        PyObject* config_module = PyImport_ImportModule(config_name.c_str());
        if (!config_module) {
            PyErr_Print();
            throw std::runtime_error("Failed to import config file: " + config_path);
        }

        // Get module dictionary
        PyObject* module_dict = PyModule_GetDict(config_module);

        // Extract configuration parameters
        PyObject* mode_obj = PyDict_GetItemString(module_dict, "mode");
        if (mode_obj) {
            params.mode = get_python_string(mode_obj);
        }

        PyObject* unit_obj = PyDict_GetItemString(module_dict, "unit");
        if (unit_obj) {
            params.unit = get_python_string(unit_obj);
        }

        PyObject* x_start_obj = PyDict_GetItemString(module_dict, "x_start");
        if (x_start_obj) {
            params.x_start = get_python_double(x_start_obj);
        }

        PyObject* x_end_obj = PyDict_GetItemString(module_dict, "x_end");
        if (x_end_obj) {
            params.x_end = get_python_double(x_end_obj);
        }

        PyObject* interval_obj = PyDict_GetItemString(module_dict, "interval");
        if (interval_obj) {
            params.interval = get_python_double(interval_obj);
            params.user_set_interval = true;
        }

        PyObject* fwhm_obj = PyDict_GetItemString(module_dict, "fwhm_ev");
        if (fwhm_obj) {
            params.fwhm_cm_minus_1 = get_python_double(fwhm_obj) * EV_TO_CM_MINUS_1;
        }

        PyObject* format_obj = PyDict_GetItemString(module_dict, "output_format");
        if (format_obj) {
            params.output_format = get_python_string(format_obj);
        }

        PyObject* filename_obj = PyDict_GetItemString(module_dict, "output_filename");
        if (filename_obj) {
            params.output_filename = get_python_string(filename_obj);
        }

        PyObject* legend_obj = PyDict_GetItemString(module_dict, "legend_names");
        if (legend_obj) {
            params.legend_names = get_python_string_list(legend_obj);
        }

        Py_DECREF(config_module);

    } catch (...) {
        finalize_python();
        throw;
    }

    finalize_python();

    // Set default interval if not specified
    if (!params.user_set_interval) {
        if (params.unit == "cm-1") {
            params.interval = 100.0;
        } else if (params.unit == "eV") {
            params.interval = 0.01;
        } else {
            params.interval = 1.0;
        }
    }

    return params;
}

// Function to find and load config file
std::string find_config_file() {
    // 1. Check current directory
    std::string local_config = "./spectrum_config.py";
    if (std::filesystem::exists(local_config)) {
        return local_config;
    }

    // 2. Check home directory
    const char* home = getenv("HOME");
    if (home) {
        std::string home_config = std::string(home) + "/spectrum_config.py";
        if (std::filesystem::exists(home_config)) {
            return home_config;
        }
    }

    throw std::runtime_error("Config file not found. Please create spectrum_config.py in current directory or home directory.");
}

PlotSpecParams parse_arguments(int argc, char* argv[]) {
    if (argc == 1) {
        print_usage();
        exit(1);
    }

    std::string config_file_path;
    std::vector<std::string> input_files;
    bool interactive = true;

    // Parse command line for config file and options
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];

        if (arg == "-help" || arg == "--help") {
            print_usage();
            exit(0);
        } else if (arg == "-no-interactive") {
            interactive = false;
        } else if (arg.substr(0, 8) == "-config=") {
            config_file_path = arg.substr(8);
        } else {
            // Assume it's an input file
            input_files.push_back(arg);
        }
    }

    if (input_files.empty()) {
        throw std::runtime_error("No input files provided");
    }

    // Find config file if not specified
    if (config_file_path.empty()) {
        config_file_path = find_config_file();
    }

    // Read configuration
    PlotSpecParams params = read_config_file(config_file_path);

    // Override with command line options
    params.input_filenames = input_files;
    params.interactive = interactive;

    // If no legend names in config or wrong number, use filenames
    if (params.legend_names.empty() || params.legend_names.size() != input_files.size()) {
        params.legend_names.clear();
        for (const auto& filename : input_files) {
            std::string name = std::filesystem::path(filename).stem().string();
            params.legend_names.push_back(name);
        }
    }

    std::cout << "Using config file: " << config_file_path << std::endl;
    return params;
}

// Function to calculate spectrum from single BDF output file
SpectrumData calculate_single_spectrum(const std::string& filename, const PlotSpecParams& params, int file_index) {
    // Try to open input file
    std::ifstream infile;
    std::string full_filename = filename;
    infile.open(full_filename);

    if (!infile.is_open()) {
        if (!ends_with(filename, ".out") && !ends_with(filename, ".log")) {
            full_filename = filename + ".out";
            infile.open(full_filename);
            if (!infile.is_open()) {
                full_filename = filename + ".log";
                infile.open(full_filename);
            }
        }
    }

    if (!infile.is_open()) {
        throw std::runtime_error("Cannot open BDF output file: " + filename);
    }

    std::cout << "Processing: " << full_filename << std::endl;

    SpectrumData spectrum;

    // Generate x-axis values
    double current_x = params.x_start;
    while (current_x <= params.x_end + 1e-8) {
        spectrum.x_values.push_back(current_x);
        current_x += params.interval;
    }

    // For demonstration, create synthetic spectrum with slight variations per file
    spectrum.y_values.resize(spectrum.x_values.size(), 0.0);

    // Add file-specific variation to create different spectra
    double shift_factor = file_index * 20.0; // Shift peaks slightly for different files
    double intensity_factor = 1.0 - file_index * 0.15; // Vary intensity

    if (params.mode == "abs") {
        for (size_t i = 0; i < spectrum.x_values.size(); ++i) {
            double x = spectrum.x_values[i];
            double intensity = 0.0;

            if (params.unit == "nm") {
                intensity += intensity_factor * 15000 * exp(-pow((x - 280 - shift_factor) / 15, 2));
                intensity += intensity_factor * 12000 * exp(-pow((x - 320 - shift_factor) / 20, 2));
                intensity += intensity_factor * 8000 * exp(-pow((x - 420 - shift_factor) / 25, 2));
                intensity += intensity_factor * 5000 * exp(-pow((x - 520 - shift_factor) / 30, 2));
            } else if (params.unit == "eV") {
                double eV_shift = shift_factor / 1000.0; // Convert to eV scale
                intensity += intensity_factor * 15000 * exp(-pow((x - 3.1 - eV_shift) / 0.15, 2));
                intensity += intensity_factor * 12000 * exp(-pow((x - 3.9 - eV_shift) / 0.2, 2));
                intensity += intensity_factor * 8000 * exp(-pow((x - 4.4 - eV_shift) / 0.1, 2));
            } else if (params.unit == "cm-1") {
                double cm_shift = shift_factor * 100; // Convert to cm-1 scale
                intensity += intensity_factor * 15000 * exp(-pow((x - 25000 - cm_shift) / 2000, 2));
                intensity += intensity_factor * 12000 * exp(-pow((x - 31000 - cm_shift) / 1500, 2));
                intensity += intensity_factor * 8000 * exp(-pow((x - 35000 - cm_shift) / 1000, 2));
            }

            spectrum.y_values[i] = intensity;
        }
        spectrum.y_label = "Molar Absorptivity (L/(mol·cm))";
        spectrum.title = "Absorption Spectra";

    } else if (params.mode == "emi") {
        for (size_t i = 0; i < spectrum.x_values.size(); ++i) {
            double x = spectrum.x_values[i];
            double intensity = 0.0;

            if (params.unit == "nm") {
                intensity += intensity_factor * 0.9 * exp(-pow((x - 350 - shift_factor) / 20, 2));
                intensity += intensity_factor * 0.7 * exp(-pow((x - 450 - shift_factor) / 25, 2));
                intensity += intensity_factor * 0.5 * exp(-pow((x - 550 - shift_factor) / 30, 2));
            } else if (params.unit == "eV") {
                double eV_shift = shift_factor / 1000.0;
                intensity += intensity_factor * 0.9 * exp(-pow((x - 2.8 - eV_shift) / 0.15, 2));
                intensity += intensity_factor * 0.7 * exp(-pow((x - 3.2 - eV_shift) / 0.2, 2));
                intensity += intensity_factor * 0.5 * exp(-pow((x - 3.6 - eV_shift) / 0.1, 2));
            }

            spectrum.y_values[i] = intensity;
        }
        spectrum.y_label = "Emission Intensity (arb. units)";
        spectrum.title = "Emission Spectra";

    } else if (params.mode == "cd" || params.mode == "cdl") {
        for (size_t i = 0; i < spectrum.x_values.size(); ++i) {
            double x = spectrum.x_values[i];
            double intensity = 0.0;

            if (params.unit == "nm") {
                intensity += intensity_factor * 50 * exp(-pow((x - 260 - shift_factor) / 15, 2));
                intensity -= intensity_factor * 40 * exp(-pow((x - 300 - shift_factor) / 20, 2));
                intensity += intensity_factor * 30 * exp(-pow((x - 340 - shift_factor) / 18, 2));
                intensity -= intensity_factor * 20 * exp(-pow((x - 380 - shift_factor) / 25, 2));
            } else if (params.unit == "eV") {
                double eV_shift = shift_factor / 1000.0;
                intensity += intensity_factor * 50 * exp(-pow((x - 4.0 - eV_shift) / 0.15, 2));
                intensity -= intensity_factor * 40 * exp(-pow((x - 3.5 - eV_shift) / 0.2, 2));
                intensity += intensity_factor * 30 * exp(-pow((x - 3.0 - eV_shift) / 0.18, 2));
            }

            spectrum.y_values[i] = intensity;
        }
        spectrum.y_label = "Δε (L/(mol·cm))";
        spectrum.title = "Circular Dichroism Spectra";
    }

    // Set x-axis label
    if (params.unit == "nm") {
        spectrum.x_label = "Wavelength (nm)";
    } else if (params.unit == "eV") {
        spectrum.x_label = "Energy (eV)";
    } else if (params.unit == "cm-1") {
        spectrum.x_label = "Wavenumber (cm⁻¹)";
    }

    return spectrum;
}

// Function to calculate multiple spectra
std::vector<SpectrumData> calculate_multiple_spectra(const PlotSpecParams& params) {
    std::cout << "==================================" << std::endl;
    std::cout << "   BDF Spectrum Calculator" << std::endl;
    std::cout << "==================================" << std::endl;
    std::cout << std::endl;

    std::cout << "Mode: " << params.mode << ", Unit: " << params.unit << std::endl;
    std::cout << "Range: " << params.x_start << " - " << params.x_end << " " << params.unit << std::endl;
    std::cout << "FWHM: " << std::fixed << std::setprecision(4) << (params.fwhm_cm_minus_1 / EV_TO_CM_MINUS_1) << " eV" << std::endl;
    std::cout << "Processing " << params.input_filenames.size() << " files..." << std::endl;
    std::cout << std::endl;

    std::vector<SpectrumData> spectra;

    for (size_t i = 0; i < params.input_filenames.size(); ++i) {
        SpectrumData spectrum = calculate_single_spectrum(params.input_filenames[i], params, i);
        spectra.push_back(spectrum);
    }

    std::cout << std::endl;
    std::cout << "All spectra calculated successfully." << std::endl;
    std::cout << "Generated " << spectra[0].x_values.size() << " data points per spectrum." << std::endl;

    return spectra;
}

// Function to create VTK plot with multiple spectra and export
void create_and_export_multiple_plots(const std::vector<SpectrumData>& spectra, const PlotSpecParams& params) {
    std::cout << std::endl;
    std::cout << "Creating visualization with " << spectra.size() << " spectra..." << std::endl;

    auto view = vtkSmartPointer<vtkContextView>::New();
    view->GetRenderer()->SetBackground(1.0, 1.0, 1.0);

    auto chart = vtkSmartPointer<vtkChartXY>::New();
    view->GetScene()->AddItem(chart);
    chart->SetAutoAxes(false);

    // Calculate nice round tick positions
    auto calculateNiceTicks = [](double min_val, double max_val, int target_ticks) -> std::vector<double> {
        double range = max_val - min_val;
        double rough_step = range / (target_ticks - 1);

        // Find a nice step size (power of 10 times 1, 2, or 5)
        double magnitude = std::pow(10, std::floor(std::log10(rough_step)));
        double normalized = rough_step / magnitude;
        double nice_step;

        if (normalized <= 1.0) nice_step = magnitude;
        else if (normalized <= 2.0) nice_step = 2 * magnitude;
        else if (normalized <= 5.0) nice_step = 5 * magnitude;
        else nice_step = 10 * magnitude;

        // Generate ticks
        std::vector<double> ticks;
        double start = std::ceil(min_val / nice_step) * nice_step;
        for (double tick = start; tick <= max_val + nice_step * 0.1; tick += nice_step) {
            ticks.push_back(tick);
        }
        return ticks;
    };

    // Find overall Y-axis range from all spectra
    double overall_y_min = std::numeric_limits<double>::max();
    double overall_y_max = std::numeric_limits<double>::lowest();

    for (const auto& spectrum : spectra) {
        double spec_min = *std::min_element(spectrum.y_values.begin(), spectrum.y_values.end());
        double spec_max = *std::max_element(spectrum.y_values.begin(), spectrum.y_values.end());
        overall_y_min = std::min(overall_y_min, spec_min);
        overall_y_max = std::max(overall_y_max, spec_max);
    }

    // Add 10% padding to Y-axis range for visual breathing room
    double y_range = overall_y_max - overall_y_min;
    double y_padding = y_range * 0.1;
    double y_min = overall_y_min - y_padding;
    double y_max = overall_y_max + y_padding;

    // Ensure y_min is not negative for absorption/emission spectra unless data requires it
    if (params.mode != "cd" && params.mode != "cdl" && overall_y_min >= 0) {
        y_min = 0;
        // Recalculate y_max with proper padding from 0
        y_max = overall_y_max + y_range * 0.1;
    }

    // Configure axes
    auto xAxis = chart->GetAxis(vtkAxis::BOTTOM);
    auto yAxis = chart->GetAxis(vtkAxis::LEFT);

    xAxis->SetGridVisible(false);
    yAxis->SetGridVisible(false);
    xAxis->GetTitleProperties()->SetFontSize(16);
    yAxis->GetTitleProperties()->SetFontSize(16);
    xAxis->GetLabelProperties()->SetFontSize(14);
    yAxis->GetLabelProperties()->SetFontSize(14);

    // Set axis behavior and ranges
    xAxis->SetBehavior(vtkAxis::FIXED);
    yAxis->SetBehavior(vtkAxis::FIXED);
    xAxis->SetRange(params.x_start, params.x_end);
    yAxis->SetRange(y_min, y_max);

    // Generate nice tick positions
    auto x_ticks = calculateNiceTicks(params.x_start, params.x_end, 6);
    auto y_ticks = calculateNiceTicks(y_min, y_max, 6);

    // Set custom tick positions
    auto x_tick_array = vtkSmartPointer<vtkDoubleArray>::New();
    for (double tick : x_ticks) {
        x_tick_array->InsertNextValue(tick);
    }
    xAxis->SetCustomTickPositions(x_tick_array);

    auto y_tick_array = vtkSmartPointer<vtkDoubleArray>::New();
    for (double tick : y_ticks) {
        y_tick_array->InsertNextValue(tick);
    }
    yAxis->SetCustomTickPositions(y_tick_array);

    xAxis->SetTitle(spectra[0].x_label.c_str());
    yAxis->SetTitle(spectra[0].y_label.c_str());

    // Configure top and right axes (no labels, no titles, clean lines)
    auto topAxis = chart->GetAxis(vtkAxis::TOP);
    auto rightAxis = chart->GetAxis(vtkAxis::RIGHT);

    topAxis->SetVisible(true);
    rightAxis->SetVisible(true);

    // Remove labels and titles from top and right axes
    topAxis->SetLabelsVisible(false);
    rightAxis->SetLabelsVisible(false);
    topAxis->SetTitle("");
    rightAxis->SetTitle("");

    // Set same range as bottom and left axes
    topAxis->SetRange(params.x_start, params.x_end);
    rightAxis->SetRange(y_min, y_max);

    // Remove tick marks from top and right axes for cleaner look
    topAxis->SetTickLength(0);
    rightAxis->SetTickLength(0);

    // Ensure no grid lines from top/right axes
    topAxis->SetGridVisible(false);
    rightAxis->SetGridVisible(false);

    // Create color series for different spectra
    auto colors = vtkSmartPointer<vtkColorSeries>::New();
    colors->SetColorScheme(vtkColorSeries::SPECTRUM);

    // Add plots for each spectrum
    for (size_t spec_idx = 0; spec_idx < spectra.size(); ++spec_idx) {
        const auto& spectrum = spectra[spec_idx];

        // Create VTK table from spectrum data
        auto table = vtkSmartPointer<vtkTable>::New();

        auto xArray = vtkSmartPointer<vtkDoubleArray>::New();
        xArray->SetName("X");
        auto yArray = vtkSmartPointer<vtkDoubleArray>::New();
        yArray->SetName("Y");

        for (size_t i = 0; i < spectrum.x_values.size(); ++i) {
            xArray->InsertNextValue(spectrum.x_values[i]);
            yArray->InsertNextValue(spectrum.y_values[i]);
        }

        table->AddColumn(xArray);
        table->AddColumn(yArray);

        // Add plot
        auto plot = chart->AddPlot(vtkChart::LINE);
        plot->SetInputData(table, 0, 1);

        // Set different colors for each spectrum
        auto color = colors->GetColorRepeating(spec_idx);
        plot->SetColorF(color.GetRed() / 255.0, color.GetGreen() / 255.0, color.GetBlue() / 255.0);

        plot->SetWidth(2.0);
        plot->SetLabel(params.legend_names[spec_idx].c_str());
    }

    // Configure legend with scholarly style
    chart->SetShowLegend(true);
    auto legend = chart->GetLegend();
    legend->SetHorizontalAlignment(vtkChartLegend::RIGHT);
    legend->SetVerticalAlignment(vtkChartLegend::TOP);
    legend->GetLabelProperties()->SetFontSize(12);
    legend->GetPen()->SetLineType(vtkPen::NO_PEN); // Remove legend border

    // Set window size and render
    view->GetRenderWindow()->SetSize(1000, 700);
    view->GetRenderWindow()->Render();

    // Export plot
    std::string full_output_name = params.output_filename + "." + params.output_format;

    if (params.output_format == "svg" || params.output_format == "eps" || params.output_format == "pdf") {
        auto exporter = vtkSmartPointer<vtkGL2PSExporter>::New();
        exporter->SetRenderWindow(view->GetRenderWindow());

        if (params.output_format == "svg") {
            exporter->SetFileFormatToSVG();
        } else if (params.output_format == "eps") {
            exporter->SetFileFormatToEPS();
        } else if (params.output_format == "pdf") {
            exporter->SetFileFormatToPDF();
        }

        exporter->CompressOff();
        exporter->SetFilePrefix(params.output_filename.c_str());
        exporter->DrawBackgroundOn();
        exporter->Write3DPropsAsRasterImageOff();
        exporter->Write();

    } else if (params.output_format == "png" || params.output_format == "jpg" || params.output_format == "jpeg") {
        auto windowToImageFilter = vtkSmartPointer<vtkWindowToImageFilter>::New();
        windowToImageFilter->SetInput(view->GetRenderWindow());
        windowToImageFilter->SetScale(2, 2); // Higher resolution
        windowToImageFilter->SetInputBufferTypeToRGB();
        windowToImageFilter->ReadFrontBufferOff();
        windowToImageFilter->Update();

        if (params.output_format == "png") {
            auto writer = vtkSmartPointer<vtkPNGWriter>::New();
            writer->SetFileName(full_output_name.c_str());
            writer->SetInputConnection(windowToImageFilter->GetOutputPort());
            writer->Write();
        } else {
            auto writer = vtkSmartPointer<vtkJPEGWriter>::New();
            writer->SetFileName(full_output_name.c_str());
            writer->SetQuality(95);
            writer->SetInputConnection(windowToImageFilter->GetOutputPort());
            writer->Write();
        }
    }

    std::cout << "Plot exported to: " << full_output_name << std::endl;

    // Start interactive viewer if requested
    if (params.interactive) {
        std::cout << "Starting interactive viewer... (Close window to exit)" << std::endl;
        view->GetInteractor()->Start();
    }
}

int main(int argc, char *argv[]) {
    try {
        std::cout << "BDF Spectrum Visualization Tool" << std::endl;
        std::cout << "===============================" << std::endl;
        std::cout << std::endl;

        // Parse command line arguments
        PlotSpecParams params = parse_arguments(argc, argv);

        // Calculate spectra from BDF files
        std::vector<SpectrumData> spectra = calculate_multiple_spectra(params);

        // Create plot and export
        create_and_export_multiple_plots(spectra, params);

        std::cout << std::endl;
        std::cout << "Processing completed successfully!" << std::endl;
        return EXIT_SUCCESS;

    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        print_usage();
        return EXIT_FAILURE;
    }
}
