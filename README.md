# 3D_touch_haptic_touch

A project for 3D haptic touch interaction, designed for use with compatible haptic devices and OpenHaptics SDK on Linux.  
This repository contains code, configurations, and documentation to run and test haptic feedback applications in 3D space.

---

## Create Workspace

```bash
conda create -n haptic_3d_touch python=3.8 -y
conda activate haptic_3d_touch
pip install -r requirements.txt
sudo apt-get install freeglut3-dev
sudo apt-get install libncurses5 libtinfo5
```

---

## Install Driver

```bash
tar -xzf TouchDriver_2024_09_19.tgz
cd TouchDriver_2024_09_19
sudo ./install_haptic_driver
```

---

## Check Driver

```bash
cd TouchDriver_2024_09_19/bin
./Touch_HeadlessSetup
./Touch_AdvancedConfig
./TouchCheckup
```

---

### Build the Example

```bash
cd openhaptics_3.4-0-developer-edition-amd64
sudo ./install
cd /opt/OpenHaptics/Developer/3.4-0/QuickHaptics/examples/$example_project
sudo apt-get update
sudo apt-get install -y libtinfo5 libncurses5
make -j"$(nproc)"
```

Replace $example_project with the actual example name, e.g., SimpleSphereGLUT.

---
### Run the Example
Create a symbolic link to map /dev/ttyACM300 to /dev/ttyACM0
```bash
sudo ln -sf /dev/ttyACM0 /dev/ttyACM300
ls -l /dev/ttyACM*
```

---

### The Example of read and write in cpp
```bash
cd cpp_read_and_write_data_from_haptic_3d_touch
```

---

### The Example of read and write in python
```bash
cd python_read_and_write_data_from_haptic_3d_touch
mkdir build && cd build
cmake .. 
cmake --build .
```

---
