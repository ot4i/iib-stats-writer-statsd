## Instructions for AIX

The conan-based configuration for the other platforms is unavailable on AIX, and so the build process is somewhat different. 
Makefile.aix provides the basis for building the LIL in two modes: GCC and xlC. The GCC build creates statsdsw-gcc630.lil and
builds with C++11 (as do the other platforms, while the xlC build does not use C++11 and instead uses a compatibility layer.

The build has several prerequisites:

1. An IIB installation

   Set IIB_INSTALL_LOCATION at the top of the file to point to a local installation of IIB 10.

2. Boost 1.60

   Set BOOST_LOCATION to point to the root of a downloaded and unzipped boost source tree.
   Downloads available from http://www.boost.org/users/history/version_1_60_0.html and no building is needed, as 
   the source files are pulled in by Makefile.aix.
   
3. IBM XL C++ 13 (for xlC builds)

   Set XLC_LOCATION to the base install path for the xlC installation.

4. GCC 6.3.0 (for GCC builds)

   Downloadable from various sources including http://www.oss4aix.org/download/RPMS/gcc/ but not supported official by IBM. 
   Choose the correct RPM for your AIX version, and install the compiler plus any prerequisite RPMs. Ensure that /opt/freeware/bin 
   in included in the PATH environment variable. 

The key operational difference between the two LILs is that the GCC-built LIL requires extra libraries to be installed for GCC
support; libstdc++-6.3.0 and libgcc-6.3.0 must be installed from RPMs on any system on which the LIL is to be used. The xlC-built 
LIL needs no extra libraries. See README.md in prebuilt/aix-7.1 for more details.
