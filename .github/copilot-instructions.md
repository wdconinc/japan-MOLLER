# JAPAN-MOLLER Analysis Framework

**Always reference these instructions first and fallback to search or bash commands only when you encounter unexpected information that does not match the info here.**

## Working Effectively

JAPAN (Just Another Parity ANalyzer) is a C++ physics data analysis framework for the MOLLER experiment. The project uses CMake and has complex ROOT/Boost dependencies that require specific setup procedures.

### Critical Build Requirements - NEVER CANCEL BUILDS

- **NEVER CANCEL ANY BUILD OR LONG-RUNNING COMMAND** - Docker builds take 15-45 minutes, compilation takes 5-15 minutes
- **Always use timeout values of 60+ minutes for builds and 30+ minutes for tests**
- **If a command appears to hang, wait at least 60 minutes before considering alternatives**

### Primary Build Method: Docker (RECOMMENDED)

Use the Docker containerized build as the primary approach. The host environment dependency management is complex and fragile.

```bash
# Build Docker image - takes 15-45 minutes. NEVER CANCEL. Set timeout to 60+ minutes.
docker build -t japan-moller .

# Run analysis in container
docker run -it japan-moller /bin/bash
```

Docker build includes all dependencies (ROOT, Boost, MySQL++) pre-configured on AlmaLinux 9.

### Alternative: Host Environment Build (COMPLEX)

Only attempt if Docker is unavailable. Requires significant dependency management.

#### Install Dependencies
```bash
# Ubuntu/Debian - ROOT installation is challenging, try multiple approaches:
# Option 1: Try conda/mamba
conda install -c conda-forge root boost-cpp

# Option 2: Build from source (takes 60+ minutes, NEVER CANCEL)
# Download ROOT source and follow build instructions

# Always install system dependencies:
sudo apt-get install -y build-essential cmake libboost-all-dev libmysqlclient-dev
```

#### Environment Setup
```bash
# Generate environment setup scripts
./SetupFiles/make_SET_ME_UP

# Source the generated environment - REQUIRED before every build/run
source SetupFiles/SET_ME_UP.bash

# Create required directories
mkdir -p /path/to/scratch/directory
export QWSCRATCH=/path/to/scratch/directory
```

#### Build Process - NEVER CANCEL
```bash
# Configure build - takes 1-3 minutes
mkdir -p build && cd build
cmake .. -DCMAKE_PREFIX_PATH=/path/to/root

# Compile - takes 5-15 minutes. NEVER CANCEL. Set timeout to 30+ minutes.
make -j$(nproc)

# Install (optional)
make install
```

### Testing and Validation

**Always run the complete test suite after any changes**

```bash
# Run all regression tests - takes 5-10 minutes. NEVER CANCEL. Set timeout to 20+ minutes.
./Tests/run_tests.sh

# Individual tests (require completed build first):
./Tests/001_compilation.sh    # Build test (may use different build system)
./Tests/002_setupfiles.sh     # Environment test  
./Tests/003_qwanalysis.sh     # Basic analysis
./Tests/004_qwparity.sh       # Mock data test

# Note: Test framework may expect legacy build system - adapt as needed
```

### Validation Scenarios - ALWAYS TEST THESE

After making changes, always test these complete workflows:

1. **Mock Data Generation and Analysis**:
   ```bash
   # Generate mock data - takes 1-2 minutes
   build/qwmockdatagenerator -r 10 -e :10000 --config qwparity_simple.conf --detectors mock_detectors.map
   
   # Analyze mock data - takes 2-5 minutes  
   build/qwparity -r 10 -e :10000 --config qwparity_simple.conf --detectors mock_detectors.map
   ```

2. **Panguin Plotting Tool**:
   ```bash
   # Build panguin separately
   cd panguin && mkdir -p build && cd build
   cmake .. && make
   
   # Test basic plotting functionality
   ./build/panguin -f macros/default.cfg
   ```

3. **Environment Validation**:
   ```bash
   # Verify all environment variables are set
   echo $QWANALYSIS $QWSCRATCH $ROOTSYS
   
   # Test ROOT functionality
   root-config --version
   root -l -q -e "gROOT->GetVersion()"
   ```

## Project Structure and Navigation

### Key Directories
- **Analysis/** - Core analysis engine classes and framework
- **Parity/** - Main analysis executables and configurations
  - `Parity/main/` - Source for qwparity and qwmockdatagenerator executables
  - `Parity/prminput/` - Configuration files (*.conf) and detector maps (*.map)
- **panguin/** - Standalone plotting/visualization tool with separate build
- **Tests/** - Regression test framework
- **SetupFiles/** - Environment configuration scripts  
- **evio/** - CODA event I/O library (auto-downloaded during build)
- **cmake/** - Build system configuration

### Main Executables
- **qwparity** - Primary parity analysis executable
- **qwmockdatagenerator** - Mock data generator for testing
- **panguin** - Plotting and visualization tool

### Configuration Files
- `Parity/prminput/prex*.conf` - Main analysis configurations
- `Parity/prminput/qwparity_simple.conf` - Simple configuration for testing
- `Parity/prminput/mock_detectors.map` - Mock detector mappings for testing
- `Parity/prminput/*.map` - Detector and data handler mappings  
- `panguin/macros/*.cfg` - Plotting configuration files

## Common Issues and Solutions

### Build Failures
- **ROOT not found**: Ensure ROOT is properly installed and ROOTSYS is set. Use Docker approach if ROOT installation is problematic
- **Boost library errors**: Install boost-devel packages, not just boost runtime
- **MySQL++ missing**: Database support is optional, build will proceed without it
- **Compiler errors**: Ensure C++11 or later compiler (GCC 4.8+)
- **features.h not found**: Mixed conda/system library conflict - use pure Docker or pure system approach

### Environment Issues  
- **QWSCRATCH errors**: Create directory and set environment variable before sourcing setup
- **Path conflicts**: Use clean environment, avoid mixing conda and system libraries
- **Permission errors**: Ensure write access to build directories

### Runtime Issues
- **Missing configuration files**: Configuration files are in Parity/prminput/ directory
- **ROOT file errors**: Ensure input data files exist and are accessible
- **Memory issues**: Large datasets may require significant RAM (8GB+ recommended)

## Timing Expectations

- **Docker build**: 15-45 minutes (includes dependency compilation)
- **CMake configuration**: 1-3 minutes  
- **Compilation (make)**: 5-15 minutes with parallel build
- **Full test suite**: 5-10 minutes
- **Mock data generation**: 1-2 minutes for small datasets
- **Analysis runs**: 2-5 minutes for mock data, varies for real data

## Development Workflow

1. **Always start with environment setup**:
   ```bash
   source SetupFiles/SET_ME_UP.bash
   ```

2. **Use existing test framework for validation**:
   ```bash
   ./Tests/run_tests.sh
   ```

3. **For panguin changes, test both CLI and GUI modes** (note: GUI may not work in headless environments)

4. **Always test with mock data before using real data**

5. **Check configuration file syntax** if analysis fails - files are in Parity/prminput/

## Critical Notes

- **Database support is optional** - MySQL++ failures will not prevent build
- **Mac builds have known issues** - use Docker on macOS  
- **Large memory requirements** - ensure adequate RAM for data processing
- **Environment variables are critical** - always source setup scripts
- **Configuration files are required** - analysis will fail without proper *.conf and *.map files

**Remember: Always wait for long-running commands to complete. Builds may take 45+ minutes in some environments.**

## Code Style and Conventions

### C++ Coding Standards

The project follows modern C++ practices with some physics-specific adaptations:

- **C++ Standard**: C++11 or later (GCC 4.8+ required)
- **Header Files**: Use `.hh` extension for headers, `.cc` for implementation
- **Naming Conventions**:
  - Classes: PascalCase (e.g., `QwPromptSummary`)
  - Methods: PascalCase (e.g., `ProcessEvent()`)
  - Variables: camelCase with prefixes (e.g., `fElementName` for member variables)
  - Constants: ALL_CAPS or kConstantName

### Static Analysis

The project uses clang-tidy for code quality. Configuration in `.clang-tidy`:
- Enables: bugprone, concurrency, cppcoreguidelines, modernize, portability, readability
- Disabled checks accommodate ROOT framework and physics code patterns
- Run with: `clang-tidy Analysis/src/filename.cc`

### ROOT Framework Integration

- **ROOT Types**: Use ROOT types (TString, Double_t) for ROOT compatibility
- **Memory Management**: Follow ROOT ownership patterns for histograms and trees
- **Style**: Physics analysis code may use different patterns than general C++

## Git Workflow and Contribution Guidelines

### Branch Management

Follow the workflow described in README.md:

1. **Always start with latest code**: `git pull`
2. **Create feature branches**: `git checkout -b descriptiveName`
3. **Track branches remotely**: `git push -u origin branchName`  
4. **Commit with author info**: `git commit --author=username`
5. **Keep commits focused**: One logical change per commit

### Repository Structure for Contributions

- **Analysis/**: Core framework - requires careful testing
- **Parity/**: Main executables - test with mock data first
- **Tests/**: Always run full test suite after changes
- **SetupFiles/**: Environment setup - changes affect all users
- **panguin/**: Independent plotting tool - separate build required

### Pull Request Guidelines

- Test builds in both Docker and host environments when possible
- Run complete test suite: `./Tests/run_tests.sh`
- Validate with mock data before testing with real data
- Document any new configuration requirements
- Include timing expectations for new long-running operations

## Tool Calling Guidelines for AI Agents

### Using bash Tool Effectively

**Long-running Commands** (CRITICAL - do not ignore):
```bash
# Always use appropriate timeouts for builds
bash: docker build -t japan-moller .
  timeout: 3600  # 60 minutes minimum
  async: false

# Test runs need sufficient time  
bash: ./Tests/run_tests.sh
  timeout: 1800  # 30 minutes minimum
  async: false

# Mock data analysis
bash: build/qwparity -r 10 -e :10000 --config qwparity_simple.conf
  timeout: 600   # 10 minutes minimum
  async: false
```

**Environment Setup Sequence**:
```bash
# 1. Generate setup scripts first
./SetupFiles/make_SET_ME_UP

# 2. Source environment (required for builds)
source SetupFiles/SET_ME_UP.bash

# 3. Set scratch directory
export QWSCRATCH=/tmp/qw_scratch && mkdir -p $QWSCRATCH
```

**Docker Usage Patterns**:
```bash
# Build container (long-running - 15-45 minutes)
docker build -t japan-moller . 
  # timeout: 3600, async: false

# Run interactive analysis
docker run -it japan-moller /bin/bash
  # Use async: true for interactive work
```

### File Operations

- **Always use absolute paths**: `/home/runner/work/japan-MOLLER/japan-MOLLER/...`
- **Check file existence** before operations: `str_replace_editor view` first
- **Configuration files** are in: `Parity/prminput/`
- **Test scripts** are in: `Tests/`
- **Built executables** are in: `build/` (after compilation)

### Testing Strategy for Agents

1. **Environment validation**: Run `Tests/002_setupfiles.sh` first
2. **Compilation test**: Run `Tests/001_compilation.sh` 
3. **Mock data workflow**: Generate and analyze test data
4. **Regression tests**: Run full `Tests/run_tests.sh` suite
5. **Tool-specific tests**: Test panguin separately if modified

## Common Commands and Frequently Used Information

The following are outputs from frequently run commands. Reference them instead of running bash commands to save time.

### Repository Root Structure
```
ls -la
Analysis/          # Core analysis framework
CMakeLists.txt     # Main build configuration
Dockerfile         # Container build definition  
Parity/            # Main analysis executables and configs
README.md          # Basic project documentation
SetupFiles/        # Environment setup scripts
Tests/             # Test framework
bin/               # Built executables (after make install)
build/             # Build directory (after mkdir build)
cmake/             # Build system configuration
evio/              # CODA event I/O library
lib/               # Built libraries (after make install)
panguin/           # Plotting tool (separate build)
```

### Key Executable Locations
```
# After successful build:
build/qwparity                    # Main analysis executable
build/qwmockdatagenerator        # Mock data generator
panguin/build/panguin            # Plotting tool (requires separate build)
```

### Essential Configuration Files
```
# Main analysis configurations:
Parity/prminput/prex.conf              # Main PREX analysis config
Parity/prminput/qwparity_simple.conf   # Simple test configuration
Parity/prminput/mock_detectors.map     # Mock detector mapping for tests

# Panguin plotting configurations:
panguin/macros/default.cfg             # Default plotting configuration
panguin/macros/defaultOnline.cfg       # Online monitoring configuration
```

### Environment Variables After Setup
```
QWANALYSIS=/path/to/japan-MOLLER       # Project root directory  
QWSCRATCH=/path/to/scratch             # Scratch/temporary directory
ROOTSYS=/path/to/root                  # ROOT installation directory
BOOST_INC_DIR=/usr/include             # Boost headers location
BOOST_LIB_DIR=/usr/lib/x86_64-linux-gnu # Boost libraries location
```