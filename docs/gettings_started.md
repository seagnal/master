# Getting Started with MASTER

This guide will walk you through setting up the MASTER environment, creating your first plugin, and running a simple example.

---

## Table of Contents
1. [Prerequisites](#prerequisites)
2. [Installation](#installation)
3. [Project Structure](#project-structure)
4. [Creating Your First Plugin](#creating-your-first-plugin)
5. [Configuring Your Plugin](#configuring-your-plugin)
6. [Building and Running](#building-and-running)
7. [Debugging](#debugging)
8. [Next Steps](#next-steps)

---

## Prerequisites

Before you begin, ensure you have the following installed:
- **C++ Compiler** (GCC, Clang, or MSVC) with C++11 support
- **Python 3.x**
- **[SCONS](https://scons.org/)** (build system)
- **Boost** (C++ libraries)
- **QT** (optional, for visualization)
- **Git** (for version control)

### Install Dependencies

#### On Ubuntu/Debian:
```bash
sudo apt update
sudo apt install build-essential python3 scons libboost-all-dev git
```

---

## Installation

### 1. Clone the Repository
```bash
git clone https://github.com/seagnal/master.git
cd master
```

### 2. Configure the Build System
MASTER uses SCONS for building. The default configuration is set in the `SConstruct` file.

### 3. Build MASTER
To build MASTER with the default configuration:
```bash
scons config=example master
```
This will compile the core system and the example plugins.

---

## Project Structure

Here’s an overview of the MASTER project structure:

```
master/
├── src/
│   ├── core/            # Core MASTER libraries
│   ├── plugins/         # Plugin directory
│   │   └── example/     # Example plugin
│   └── ...
├── config/              # Configuration files
│   └── example.xml      # Example configuration
├── docs/                # Documentation
├── SConstruct           # Main build script
└── README.md
```

---

## Creating Your First Plugin

### 1. Create a Plugin Directory
```bash
mkdir -p src/plugins/my_first_plugin
```

### 2. Create the Plugin Files
You need two files: a header file (`.hh`) and a source file (`.cc`).

#### `src/plugins/my_first_plugin/my_first_plugin.hh`
```cpp
/**
 * my_first_plugin.hh
 *
 * Copyright (c) SEAGNAL SAS
 * All rights reserved.
 */

#ifndef MY_FIRST_PLUGIN_HH_
#define MY_FIRST_PLUGIN_HH_

#include <core.hh>

/* Port identifiers */
enum {
    E_MY_FIRST_PORT_1,
    E_MY_FIRST_PORT_2,
};

/* Message identifiers */
enum {
    E_ID_MY_FIRST_MSG1 = M_ID_USER,
};

class CT_MY_FIRST_PLUGIN : public CT_CORE {
public:
    CT_MY_FIRST_PLUGIN();
    virtual ~CT_MY_FIRST_PLUGIN();

    /* Port event handler */
    int f_event(CT_GUARD<CT_PORT_NODE> const & in_pc_node);

    /* Plugin lifecycle events */
    int f_post_init(void);
    int f_pre_destroy(void);
    int f_settings(CT_NODE & in_c_node);
};

#endif /* MY_FIRST_PLUGIN_HH_ */
```

#### `src/plugins/my_first_plugin/my_first_plugin.cc`
```cpp
/**
 * my_first_plugin.cc
 *
 * Copyright (c) SEAGNAL SAS
 * All rights reserved.
 */

#include <my_first_plugin.hh>
#include <event.hh>

CT_MY_FIRST_PLUGIN::CT_MY_FIRST_PLUGIN() : CT_CORE() {
    D("Creating MY_FIRST_PLUGIN CORE [%p]", this);

    /* Create ports */
    f_port_create(E_MY_FIRST_PORT_1, "port1", M_PORT_BIND(CT_MY_FIRST_PLUGIN, f_event, this));
    f_port_create(E_MY_FIRST_PORT_2, "port2", M_PORT_BIND(CT_MY_FIRST_PLUGIN, f_event, this));
}

CT_MY_FIRST_PLUGIN::~CT_MY_FIRST_PLUGIN() {}

int CT_MY_FIRST_PLUGIN::f_event(CT_GUARD<CT_PORT_NODE> const & in_pc_node) {
    D("Received node ID: 0x%x (size: %d bytes)", in_pc_node->get_id(), in_pc_node->get_size());
    return EC_SUCCESS;
}

int CT_MY_FIRST_PLUGIN::f_post_init(void) {
    WARN("MY_FIRST_PLUGIN: All cores are ready!");
    return EC_SUCCESS;
}

int CT_MY_FIRST_PLUGIN::f_pre_destroy(void) {
    WARN("MY_FIRST_PLUGIN: Pre-destroy");
    return EC_SUCCESS;
}

int CT_MY_FIRST_PLUGIN::f_settings(CT_NODE & in_c_node) {
    WARN("MY_FIRST_PLUGIN: Settings received");
    return EC_SUCCESS;
}

/* Register the plugin */
M_PLUGIN_CORE(CT_MY_FIRST_PLUGIN, "MY_FIRST_PLUGIN", M_STR_VERSION);
```

### 3. Create the Build Script
#### `src/plugins/my_first_plugin/SConscript`
```python
#!/usr/bin/python
Import('bs')

sw_dict = {
    'name': 'plugin-my_first_plugin',
    'type': 'library',
    'sources': ['my_first_plugin.cc'],
    'includes': ['./', '#src/plugins'],
    'libs': {'shared': ['boost', 'core'], 'static': ['node', 'common-cpp']},
    'api': 'my_first_plugin.hh'
}

bs.AddSW(sw_dict)
```

---

## Configuring Your Plugin

### 1. Create a Configuration File
Create a new XML configuration file in the `config/` directory.

#### `config/my_first_plugin.xml`
```xml
<master>
    <node>
        <type>MY_FIRST_PLUGIN</type>
        <name>my_first_plugin_instance</name>
        <ports>
            <port>
                <name>port1</name>
                <url>
                    <rx>msg://my_queue</rx>
                </url>
            </port>
            <port>
                <name>port2</name>
                <url>
                    <tx>msg://my_queue</tx>
                </url>
            </port>
        </ports>
        <settings>
            <param1>value1</param1>
        </settings>
    </node>
</master>
```

### 2. Update the Build Configuration
Edit the `SConstruct` file to include your plugin and configuration.

#### Add to `SConstruct`:
```python
# Add your plugin to the build
if bs['config'] == 'my_first_plugin':
    libs['module'].append('plugin-my_first_plugin')

# Add your configuration
if bs['config'] == 'my_first_plugin':
    dirs.append('src/plugins/my_first_plugin')
    bs.env_run['ENV']['MASTER_CONFIG'] = os.path.abspath('config/my_first_plugin.xml')
```

---

## Building and Running

### 1. Build Your Plugin
```bash
scons config=my_first_plugin master
```

### 2. Run MASTER with Your Plugin
```bash
scons config=my_first_plugin master.run
```

### Expected Output
You should see log messages indicating that your plugin is loaded and running:
```
MY_FIRST_PLUGIN: All cores are ready!
```

---

## Debugging

### 1. Run with GDB
```bash
scons config=my_first_plugin master.gdb
```

### 2. Debugging Tips
- Use the `D()` or _DBG macros for debug messages.
- Check the log files for errors.
- Ensure all ports and messages are correctly configured in the XML file.

---

## Next Steps

- **Explore the Example Plugins**: Look at the provided examples in `src/plugins/example`.
- **Extend Functionality**: Add more ports, messages, or settings to your plugin.
- **Visualization**: Try using the M-VIS platform for real-time data visualization.
- **Advanced Topics**: Learn about distributed processing and FPGA integration.

---

## Troubleshooting

### Common Issues
- **Build Errors**: Ensure all dependencies are installed and paths are correct.
- **Plugin Not Found**: Verify the plugin name in the XML configuration matches the `M_PLUGIN_CORE` macro.
- **Port Communication Issues**: Double-check the port names and URLs in the XML file.

---

## Resources
- [MASTER Documentation](docs/)
- [BML Protocol](https://github/seagnal/libbml)
- [SEAGNAL Support](mailto:support@seagnal.fr)
```

---

### Key Features of This Guide:
- **Step-by-Step Instructions**: From installation to running your first plugin.
- **Code Examples**: Ready-to-use templates for plugins and configurations.
- **Debugging Tips**: Common issues and solutions.
- **Next Steps**: Encourages further exploration and learning.

