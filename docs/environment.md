# Environment

This chapter describes the MASTER environment: its components, how modules interact with it, and how to configure plugins.

---

## Overview

The MASTER environment is designed to accelerate the development, testing, and deployment of signal processing modules. It provides a unified execution and development environment, abstracting communication, context, configuration, visualization, and data recording/playback.

---

## Components

### Build System
- **Purpose**: Compiles libraries, programs, and modules; manages dependencies; and supports local/remote execution and debugging.
- **Features**:
  - Dependency management between libraries, programs, and modules.
  - Configuration management (plugin sets and profiles).
  - Remote execution via SSH/SCP.
  - Debugging via GDB or GDB-SERVER/GDB-CLIENT.

### Core Library
- **core.class**: A C++ abstraction layer for communication, execution context, and module configuration.
- **Utilities**: Debug, thread, smart pointers, etc.

---

## Module and Environment

### Module Characteristics
- **Name**: Unique type identifier.
- **Ports**: Input/output ports for data exchange.
- **Hierarchy**: Can contain sub-modules.

### Communication Abstraction
- Modules communicate via the **BML (Binary Markup Language)** protocol, similar to XML but in binary format.
- **Transports**: Raw Ethernet, TCP, UDP, Serial, Pipe, Message Queue, Pushed Call, etc.
- **Memory Optimization**: Data is only copied between modules if necessary (e.g., GPU to CPU).

### Module Communication
- Modules are agnostic to the underlying communication method.
- Multiple threads can be used within a module to handle events from different ports.

---

## Plugin Configuration

Plugins are deployed using an **XML configuration file**, which describes:
- **Instance name**
- **Type** (for instantiation)
- **Parameters** (module-specific)
- **Hierarchy** (sub-modules)
- **Communication methods** (ports and associated protocols)

### Example Configuration
```xml
<master>
  <node>
    <type>test1</type>
    <ports>
      <port>
        <name>port1</name>
        <url>
          <tx>func://func1</tx>
        </url>
      </port>
      <port>
        <name>port2</name>
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

- **`<node>`**: Defines a module instance.
- **`<type>`**: Plugin type (must match the macro in the plugin code).
- **`<ports>`**: Communication ports and their URLs (e.g., `udp://`, `tcp://`, `msg://`).
- **`<settings>`**: Initialization parameters.

---

## Software Layers

1. **Low-level Libraries**: e.g., `libbml` (BML_NODE, SAB).
2. **System Interface Libraries**: Qt or BOOST.
3. **MASTER CORE Libraries**: Core functionality.
4. **Plugins**:
   - **Module Plugins** (e.g., BeamForming, Visualization).
   - **Transport Plugins** (e.g., Msg Queue, TCP).
   - **Resource Plugins** (e.g., GPU Memory, CPU Memory).


