# MASTER – Plugin Configuration and Creation Guide

This guide explains how to create, configure, and integrate plugins within the MASTER framework.

---

## Table of Contents
1. [Introduction](#introduction)
2. [Plugin Overview](#plugin-overview)
3. [Creating a Plugin](#creating-a-plugin)
   - [Directory Structure](#directory-structure)
   - [Source Files](#source-files)
   - [Plugin Registration](#plugin-registration)
   - [Ports and Callbacks](#ports-and-callbacks)
4. [Compilation Script](#compilation-script)
5. [Configuration Profile](#configuration-profile)
6. [Integration into MASTER](#integration-into-master)
   - [Modifying the Main Build Script](#modifying-the-main-build-script)
   - [Adding External Libraries](#adding-external-libraries)
7. [Compilation and Execution](#compilation-and-execution)
8. [Best Practices](#best-practices)

---

## Introduction

MASTER is a modular framework for signal processing, designed to accelerate development and testing by abstracting communication, configuration, and resource management. Plugins are the building blocks of MASTER, allowing developers to add custom signal processing, acquisition, or visualization modules.

---

## Plugin Overview

A **plugin** in MASTER is a C++ module (or VHDL for hardware) that runs within the MASTER environment. Each plugin:
- Has a unique name and type.
- Exposes input/output ports for communication.
- Can be configured via XML profiles.
- Can have child modules (hierarchical structure).

Plugins communicate using the **BML (Binary Markup Language)** protocol, which is similar to XML but in binary format, ensuring flexibility and efficiency.

---

## Creating a Plugin

### Directory Structure

Create a dedicated directory for your plugin under `src/plugins/`, e.g.:
```
src/plugins/example/
```

### Source Files

Each plugin requires at least two files:
- A header file (`.hh`) declaring the plugin class and its methods.
- A source file (`.cc`) implementing the plugin logic.

#### Example: `test1.hh`
```cpp
class CT_TEST1: public CT_CORE {
public:
    CT_TEST1();
    virtual ~CT_TEST1();
    int f_event(CT_GUARD<CT_PORT_NODE> const & in_pc_node);
    int f_settings(CT_NODE & in_c_node);
    int f_post_init(void);
    int f_pre_destroy(void);
};
```

#### Example: `test1.cc`
```cpp
CT_TEST1::CT_TEST1(void) : CT_CORE() {
    f_port_create(E_TEST_PORT_1, "port1", M_PORT_BIND(CT_TEST1, f_event, this));
    f_port_create(E_TEST_PORT_2, "port2", M_PORT_BIND(CT_TEST1, f_event, this));
}

int CT_TEST1::f_event(CT_GUARD<CT_PORT_NODE> const & in_pc_node) {
    // Handle incoming BML nodes
    return EC_SUCCESS;
}
```

### Plugin Registration

Register your plugin using the `M_PLUGIN_CORE` macro:
```cpp
M_PLUGIN_CORE(CT_TEST1, "TEST1", M_STR_VERSION);
```
- `CT_TEST1`: The plugin class.
- `"TEST1"`: The plugin name (case-insensitive).
- `M_STR_VERSION`: The version string.

### Ports and Callbacks

- **Ports**: Define input/output ports in the constructor using `f_port_create()`.
- **Callbacks**: Implement the `f_event()` method to handle incoming data on each port.

---

## Compilation Script

Create a `SConscript` file in your plugin directory to define the build configuration:

```python
#!/usr/bin/python
Import('bs')

sw_dict = {
    'name': 'plugin-test1',
    'type': 'library',
    'sources': ['test1.cc'],
    'includes': ['./', '#src/plugins'],
    'libs': {
        'shared': ['boost', 'core'],
        'static': ['node', 'common-c', 'common-cpp']
    },
    'api': 'test1.hh',
    'install': []
}

bs.AddSW(sw_dict)
```

- **name**: Plugin name.
- **type**: Always `library` for plugins.
- **sources**: List of source files.
- **includes**: Directories for header files.
- **libs**: Shared and static library dependencies.

---

## Configuration Profile

Create an XML configuration file (e.g., `src/config/example.xml`) to define how your plugin is instantiated and connected:

```xml
<master>
    <node>
        <type>test1</type>
        <ports>
            <port>
                <name>port1</name>
                <url>
                    <rx>msg://msg1</rx>
                </url>
            </port>
        </ports>
        <settings>
            <config1>1</config1>
        </settings>
    </node>
</master>
```

- **type**: Plugin type (matches the name in `M_PLUGIN_CORE`).
- **ports**: Define communication ports and their URLs (e.g., message queues, TCP/UDP).
- **settings**: Plugin-specific configuration.

---

## Integration into MASTER

### Modifying the Main Build Script

Edit the main `SConstruct` file to:
1. Add your plugin directory to the source folders.
2. Add your plugin as a dependency for the desired configuration.

```python
if bs['config'] == 'example':
    libs['module'].append('plugin-test1')
    dirs.append('src/plugins/example')
    bs.env_run['ENV']['MASTER_CONFIG'] = os.path.abspath('src/config/example.xml')
```

### Adding External Libraries

If your plugin depends on external libraries, add them in the `EXTERNAL DEPENDENCY` section of `SConstruct`:

```python
bs.ConfigureAddLib(name='cryptopp', flags='-I/usr/include/cryptopp -lcrypto++')
```

---

## Compilation and Execution

Compile and run your plugin using the following commands:

```bash
# Compile MASTER with the 'example' configuration
scons config=example master

# Run MASTER with the 'example' configuration
scons config=example master.run

# Debug with GDB
scons config=example master.gdb
```

---

## Best Practices

- **Port Naming**: Use clear, unique names for ports and plugins.
- **Error Handling**: Always check return values and handle errors gracefully.
- **Documentation**: Document your plugin’s purpose, ports, and configuration options.
- **Testing**: Test your plugin in isolation before integrating it into larger configurations.

---

For more details, refer to the [MASTER Documentation](link-to-master-docs) or contact [SEAGNAL Support](mailto:support@seagnal.fr).

