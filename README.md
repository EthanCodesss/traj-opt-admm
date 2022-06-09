# Robust Multi-Robot Trajectory Optimization Using Alternating Direction Method of Multiplier

This repository is the official implementation of Robust Multi-Robot Trajectory Optimization Using Alternating Direction Method of Multiplier.

## Installation

To install our code:

- Clone the repository into your local machine:

```bash
git clone https://github.com/ruiqini/traj-opt-admm.git
```

- Compile the code using cmake (default in Release mode):

```bash
cd traj-opt-admm
mkdir build
cd build
cmake ..
make

## Data

You can download our dataset here:

- [Data](). Extract file in `build/` folder.

Extract compressed data file, there are four folders inside: `Config_file/`, `init/`, `model/`, `result`. 
`Config_file/3D.json` saves input parameters in paper.
`init/name_init.txt` saves initial trajectory of input environment mesh `name`.
`model/single_name` saves input environment point cloud `name`, for example `name = bridge.obj`.

## Usage

Run commands for single UAV:
```bash
./admmPathPlanning3D name
```
Run commands for multiple UAVs:
```bash
./multiPathPlanning3D name
```
Result information will be saved in `result/`.


