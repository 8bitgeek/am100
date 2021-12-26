# am100

Alpha Microsystems AM-100 Emulator 

Derived from the work of Mike Noel http://www.kicksfortso.com/Am100/

# About

The aim of this project is to refactor the am100 emulator developed by 
Mike Noel, such that it becomes possible to run the emualtor on a 
microcontroller such as an STM32 or so.

In general, the goal is to separate the functional concerns of the various 
components of the system such that they can be individually included, or excluded from the build.

Release Notes:

- WD16 CPU emulation separated into a separate library project [libemuwd16](https://github.com/8bitgeek/libemuwd16)
- The 64K-word CPU Instruction-Type branch table converted to a decision tree to reduce memory footprint.
- Fixed a floating point format translation bug that prevented floating point numbers from working correctly on ARM 32/64 bit targets.
- The peripheral card source files have their own headers and are a little bit more indepeendent (there's more to go in that direction)
- The host O/S utility source files are present, but not included included this build, the intention is to migrate those to their own separate project.

# Fetch and Compile Steps
```
git clone https://github.com/8bitgeek/am100.git
cd am100
git submodule init
git submodule update
make
```

# Alpha Micro PDF Documentation at BitSavers

https://bitsavers.computerhistory.org/pdf/alphaMicrosystems/am-100/

# Other Sources of Information

http://s100computers.com/Hardware%20Folder/Alpha%20Micro/AM-100/AM-100.htm


http://www.obsoletecomputermuseum.org/am100/

