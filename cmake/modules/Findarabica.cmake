# - Try to find arabica
# Once done this will define
#  arabica_FOUND - system has arabica
#  arabica_INCLUDE_DIRS - include directories for arabica
#  arabica_LIBRARIES - libraries for arabica

FIND_PACKAGE(LibXml2 REQUIRED)
FIND_PACKAGE(EXPAT REQUIRED)

FIND_PATH(arabica_INCLUDE_DIR SAX/XMLReader.hpp PATH_SUFFIXES arabica)
FIND_LIBRARY(arabica_LIBRARY NAMES arabica)

INCLUDE(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(arabica DEFAULT_MSG
                                  arabica_LIBRARY arabica_INCLUDE_DIR LIBXML2_INCLUDE_DIR LIBXML2_LIBRARIES EXPAT_LIBRARY)
                                  
SET(arabica_INCLUDE_DIRS ${arabica_INCLUDE_DIR} ${LIBXML2_INCLUDE_DIR})
SET(arabica_LIBRARIES ${arabica_LIBRARY} ${LIBXML2_LIBRARIES} ${EXPAT_LIBRARY})

MARK_AS_ADVANCED(arabica_INCLUDE_DIRS arabica_LIBRARIES)
