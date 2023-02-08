<!-- TODO : clean up the whole code and edit the README.md @acorn421 [#2](https://github.com/acorn421/SVF-SRCSNK/issues/2)-->

# SVF-SRCSNK

## Codes
- tools/SRCSNKA/srcsnka.cpp
  - Code compiled into executable binary which contains LLVM main function.
- lib/SABER/PathChecker.cpp
  - Source code that finds all paths from Source to Sink.
- include/SABER/PathChecker.h
  - Declare the PathChecker class that inherits from SrcSnkDDA.

## Dependencies
- LLVM 13.0.0
- z3 4.8.8
- citr 10

## Install & Build
```bash
git clone https://github.com/SCVMON/SCVMON-SVF.git
sudo apt install cmake gcc g++ libtinfo-dev libz-dev zip wget
./build.sh
```

## Test
```bash
./test_srcsnka.sh

vim reachableNodes.dot
```

## Run
```bash
. ./setup.sh
srcsnka ${single_llvm_ir_file} --source-file=${source_file} --sink-file=${sink_file} --include-file=${include_file} --add-edge-file=${add_edge_file} -stat=false --debug-only=pathchecker
```

## Input files
- All files must conform to the indentation rules.

### Source file
1. Find LLVM IR of the Source variables and describe it in a regex format.
  - In the case of Source, it is often a member variable of a structure or class, so it cannot be expressed as a single IR.
  - Therefore, all IRs that refer to the member variable must be found.
  - In most cases, getelementptr command is used.
3. e.g.
    ```
    {} = getelementptr inbounds %class.AP_InertialSensor, %class.AP_InertialSensor* %this, i64 0, i32 11, i64 {}
    ```

### Sink file
1. Find the LLVM IR of the Sinks and describe it in regex format
2. In the case of sink, since only a specific line of the source code is considered a sink, the function name has to be provided.
   - Find the IR in a specific function and add it as a sink
3. e.g. 
    ```
    Function : _ZN20AP_MotorsMulticopter13output_to_pwmEf
      %conv15 = fptosi float %pwm_output.0 to i162
    ```

### Include file
1. Describe the nodes that must be included in the result for debugging.
2. After execution of source-sink analysis, debug logs are printed, whether the corresponding nodes are included in the result.
3. Description in the same format as the source file.
4. e.g.
    ```
      %conv15 = fptosi float %pwm_output.0 to i16
      ret i16 %conv15
    ```

### Add edge file
1. Additional edge information to be connected in DFG.
2. Support only new edges from Store nodes to Load nodes.
3. Store nodes are described in sink file format
4. Load nodes are described in source file format
5. Many-to-many connections are possible.
6. e.g.
    ```
    Store
    Function _ZN6AC_PID10update_allEffb
      store float %target, float* %_target, align 4
    Function _ZN6AC_PID10update_allEffb
      store float %add, float* %_target10, align 4
    Load
      {} = getelementptr inbounds %class.AC_PID, %class.AC_PID* %this, i64 0, i32 12
    ```
