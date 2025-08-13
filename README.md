# MASTER

[![License: LGPL v2.1](https://img.shields.io/badge/License-LGPL%20v2.1-blue.svg)](https://www.gnu.org/licenses/old-licenses/lgpl-2.1.html)
[![C++](https://img.shields.io/badge/language-C++-blue.svg)](https://isocpp.org/)
[![Build System](https://img.shields.io/badge/build-SCONS-green.svg)](https://scons.org/)

**MASTER** is a signal acquisition and processing system designed to accelerate the *Time to Market* for signal processing modules on heterogeneous architectures (FPGA/CPU/GPU). It provides a C++ abstraction layer, a unified communication protocol (BML), and libraries for acquisition, visualization, and data exchange with tools like MATLAB/Octave and Python.

---

## 📌 Overview

MASTER enables:
- **Reduced development time** by focusing on algorithms rather than data exchange.
- **Seamless communication** between software/hardware modules, debugging tools, and testing frameworks.
- **Optimized memory transfers** between modules, especially for parallel architectures (GPGPU).
- **Integration bridges** with MATLAB/Octave and Python for testing and validation.
- **Dynamic configuration** of modules and their interactions via XML files.

### 🔍 Use Cases
- Signal processing (Radar, Sonar, etc.)
- Real-time acquisition and visualization
- Unit testing and validation
- Distributed processing across multiple systems

---

## 🏗️ Architecture

### Environment
- **C++ Abstraction Layer**: Communication, execution context, and configuration.
- **Build System**: Based on SCONS and Python.
- **BML Protocol**: Binary format similar to XML, optimized for inter-module communication.

### Platforms
- **M-PROC**: Software platform for module instantiation and communication.
- **M-VIS**: Visualization platform (GUI) based on QT/OpenGL.
- **M-ACQ**: FPGA-based hardware platform for acquisition and processing.

### Modules
Each development results in a **module** (C++ or VHDL), characterized by:
- A name (node type)
- Input/output ports
- Hierarchy (child modules)
- XML configuration

---

## 🛠️ Installation

### Prerequisites
- C++11 or later
- Python 3.x (for the build system)
- SCONS
- Libraries: Boost, QT (optional), BML (LGPLv2.1)

### Steps
1. **Clone the repository**:
   ```bash
   git clone https://github.com/seagnal/master.git
   cd master
   ```

2. **Configure and compile**:
   ```bash
   scons config=<configuration_name> master
   ```
   Example for the `example` configuration:
   ```bash
   scons config=example master
   ```

3. **Run**:
   ```bash
   scons config=example master.run
   ```
   For debugging with GDB:
   ```bash
   scons config=example master.gdb
   ```

4. **Package**:
   ```bash
   scons config=example master.install
   ```
   
   For plugin packaging :
   ```bash
   scons config=example plugin-test1.modInstall
   ```
   
   For plugin bindings (Octave/Matlab/Python) :
   ```bash
   scons config=example plugin-test1.bindings
   ```
---

## 📦 Creating a Plugin

### Plugin Structure
A plugin is a shared library containing:
- A class inheriting from `CT_CORE`
- Communication ports
- An identification macro: `M_PLUGIN_CORE(MyClass, "MY_PLUGIN", M_STR_VERSION)`

### Minimal Example
1. **Create the directory**:
   ```bash
   mkdir -p src/plugins/my_plugin
   ```

2. **Source files**:
   - `my_plugin.hh`: Class declaration and ports.
   - `my_plugin.cc`: Implementation of callbacks (`f_event`, `f_post_init`, etc.).
   - `SConscript`: Compilation script.

3. **Example `SConscript`**:
   ```python
   Import('bs')
   sw_dict = {
       'name': 'plugin-my_plugin',
       'type': 'library',
       'sources': ['my_plugin.cc'],
       'includes': ['./', '#src/plugins'],
       'libs': {'shared': ['boost', 'core'], 'static': ['node', 'common-cpp']},
       'api': 'my_plugin.hh'
   }
   bs.AddSW(sw_dict)
   ```

4. **Configure the XML profile**:
   ```xml
   <master>
       <node>
           <type>my_plugin</type>
           <ports>
               <port>
                   <name>port1</name>
                   <url><rx>msg://my_queue</rx></url>
               </port>
           </ports>
       </node>
   </master>
   ```

5. **Add the plugin to the build**:
   Modify `SConstruct` to include the directory and dependency.

---

## 📂 Configuration
Modules are configured via an XML file:
```xml
<master>
    <node>
        <type>test1</type>
        <ports>
            <port>
                <name>port1</name>
                <url><tx>func://my_function</tx></url>
            </port>
        </ports>
        <settings>
            <param1>value1</param1>
        </settings>
    </node>
</master>
```

### Supported Protocols
- **Hardware**: Ethernet (UDP/TCP), Serial, PCIe
- **Software**: Message queues, pipes, function calls

---

## 🔧 Development

### Workflow
1. Develop the algorithm in a module.
2. Define ports and callbacks.
3. Configure the module via XML.
4. Compile and test with `master.run`.

### Best Practices
- Use `f_post_init()` for post-load initializations.
- Avoid communication calls in constructors.
- Use `CT_PORT_NODE_GUARD` to send data.

---

## 📄 License
- **LGPLv2.1**: Abstraction layer and BML libraries.
- **Proprietary**: Processing and visualization modules (SEAGNAL license).

---

## 🤝 Contribution
- **Report a bug**: Open an *issue* on GitHub.
- **Suggest a feature**: Open a *pull request*.
- **Support**: [support@seagnal.fr](mailto:support@seagnal.fr)

---

## 📚 Documentation
- [Getting Started Guide](docs/getting_started.md)
- [Environment](docs/environment.md)
- [Plugins](docs/plugins.md)
- [Examples](src/plugins/example/)
- [BML](https://github.com/seagnal/bml)

---

## 📬 Contact
- **Author**: Johann Baudy
- **Email**: johann.baudy@seagnal.fr
- **Website**: [www.seagnal.fr](https://www.seagnal.fr)
