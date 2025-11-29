## Distributed Image System (underwater-ipc)

A modular, distributed image processing system written in modern C++ that simulates image generation, 
performs feature extraction using __OpenCV__, and stores processed data into a databse using __inter-process communication (IPC)__.
THis project was built to demostrate __system design__, __IPC__, __image processing__ and __storage piplines__.

### Platform & Build Environment
- Development OS: macOS (Apple Silicon).
- Target OS: Linus (as per project requirement).
- Compiler:
  ````
  Apple clang version 17.0.0 (clang-1700.4.4.1)
  Target: arm64-apple-darwin25.1.0
  Thread model: posix
  ````
- BUild System: CMake
  ````
  cmake version 4.0.3
  ````
- Package Manager:
  - Homebrew (pkg-config not installed)
    
### System Architecture Overview
1. Image Generator
   - Reads imgaes from disk continuosly.
   - Publish images data via IPC.
   - Loops infinitely over the input dataset.
2. Feature Processor
   -  REceives images from Generator.
   -  Extracts keypoints using __SIFT__ (Scale-Invarient Feature Transform)
   -  Publishes:
       - Original Images
       - Extracted keypoints
3. Data Logger
   - Receives Processed Data
   - Stores metadata in SQLite database
   - Saves:
     - Raw processed images
     - Visualized keypoints

### Project Structure
````
underwater-ipc/
├─ CMakeLists.txt
├─ src/
│  ├─ generator/
│  ├─ processor/
│  └─ logger/
├─ include/
|  ├─ common/
│      └─ dual_logger.hpp
│      └─ ipc_utils.hpp
├─ config/
│  └─ default_config.json
├─ scripts/
│  └─ run_all.sh
├─ tests/
│  ├─ unit/
│  └─ e2e/
├─ underwater_images/
├─ processed_images/
│  ├─ processed/
│  └─ visualized/
├─ logs/
│  ├─ generator.log
│  ├─ processor.log
│  └─ logger.log
└─ data/
   └─ database.db
````

### Configuraiton
````
config/default_config.json
````

##### Controls:
  - IPC ports
  - INput image path
  - Output paths
  - Database location
  - Logging behaviour

### Input & Output

##### Input
````
underwater_images/
````
Contains the dataset used by Generator.
##### Output
- __Processed Images__:
  ````
  processed_images/processed
  ````
- __Visulalized Keypoints__:
  ````
  processed_images/visualized/
  ````
- __Logs__:
  ````
  logs/generator.log
  logs/processor.log
  logs/logger.log
  ````

### Database
- __Type__:  SQLite
- __Path__:
  ````
  data/database.db
  ````
  
##### Stores:
- IMage Metadata
- NUmber of Extracted Keypoints
- Processing timestamps

### BUild Instructions
###### From the Project root:
````
mkdir build
cd build
cmake ..
cmake --build .
````
This builds all the three Applications:
- Generator
- Processor
- Logger

### Running the System

THe system supports __two execution modes__.
1. Standalone execution (applications can be started in any order)
2. Single-script execution
##### Option 1: Run Applications Standalone (ANY Order)
All executables are located inside the `build/` directory after compilation

1. __Start Logger (Optional First)__
   ````
   cd build/src/logger
   ./logger
   ````
2. __Start Processor__
   ````
   cd build/src/processor
   ./processor
   ````
3. __Start Generator__
   ````
   cd build/src/generator
   ./generator ../../underwater_images
   ````
__Note__: 
- If __Generator starts before Processor__, images will wait until Processor becomes avaiable.
- If __Processor restarts__,it will resume receiving new images automatically.
- If __Logger restarts__,it will resume consuming processed data without crashing the system.
- NO application depdends on the startup timing of another.
  
##### Option 2: Run All Applications Using a Single Script

From Project Root
````
bash scripts/run_all.sh
````
THis will:
- Start __Generator__, __Processor__, and __Logger__ in seperate `tmux` windows
- Automatically careate:
  - `logs/`
  - `processed_images/`
  - `data/`
- Begin Continuous image processing

##### View Running Applications
````
tmux attach-session -t logger
````
Switch betweem windows:
- `Ctrl + B` then `0` -> Logger
- `Ctrl + B` then `1` -> Processor
- `Ctrl + B` then `2` -> Generator

Stop all:
`tmux kill-session -t logger`

### Testing
- __Unit Tests__:
  ````
  tests/unit/ipc_utils_test.cpp
  ````
- End-to-End Tests:
  ````
  tests/e2e/e2e_flow_test.cpp
  ````

  Tests are currently executed manually as standalone binaries after build.
### Logging
- Logging method: __File-based logging__
- Log files located in
  ````
  logs/
  ````
Files:
- generator.log
- processor.log
- logger.log

### Key Feature
- Modular distributed design
- Independednt fault-tolerant services
- Continuous image streaming simulation
- OpenSV SIFT feature extraction
- Binary IPC serialization
- SQLIte-based persistence.
- Single-script multi-process execution
- End-to-End processig validaiton.

### Author
- Project Name: Distributed Image System
- Repository Name: underwater-ipc
- Developed by: Mahaboob Danish
- Role: Software Developer

