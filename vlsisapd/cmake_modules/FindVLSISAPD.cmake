# - Try to find the VLSISAPD libraries
# Once done this will define
#
#  VLSISAPD_FOUND       - system has the VLSISAPD library
#  VLSISAPD_INCLUDE_DIR - the VLSISAPD include directory
#  VLSISAPD_LIBRARIES   - The libraries needed to use VLSISAPD

SET(VLSISAPD_FOUND FALSE)

# Setup the DIR_SEARCH_PATH.
MACRO(SETUP_SEARCH_DIR project)
  IF( NOT("$ENV{${project}_TOP}" STREQUAL "") )
    MESSAGE("-- ${project}_TOP is set to $ENV{${project}_TOP}")
    LIST(INSERT ${project}_DIR_SEARCH 0 "$ENV{${project}_TOP}")
  ENDIF( NOT("$ENV{${project}_TOP}" STREQUAL "") )

  IF( NOT("$ENV{${project}_USER_TOP}" STREQUAL "") )
    MESSAGE("-- ${project}_USER_TOP is set to $ENV{${project}_USER_TOP}")
    LIST(INSERT ${project}_DIR_SEARCH 0 "$ENV{${project}_USER_TOP}")
  ENDIF( NOT("$ENV{${project}_USER_TOP}" STREQUAL "") )

  LIST(REMOVE_DUPLICATES ${project}_DIR_SEARCH)
ENDMACRO(SETUP_SEARCH_DIR project)

MACRO(SET_FOUND project)
    IF(${project}_INCLUDE_DIR AND ${project}_LIBRARY)
        SET(${project}_FOUND TRUE)
    ELSE(${project}_INCLUDE_DIR AND ${project}_LIBRARY)
        SET(${project}_FOUND FALSE)
    ENDIF(${project}_INCLUDE_DIR AND ${project}_LIBRARY)
ENDMACRO(SET_FOUND project)

SETUP_SEARCH_DIR(VLSISAPD)

IF(VLSISAPD_DIR_SEARCH)
    # AGDS
    FIND_PATH   (AGDS_INCLUDE_DIR NAMES vlsisapd/agds/GdsLibrary.h PATHS ${VLSISAPD_DIR_SEARCH} PATH_SUFFIXES include)
    FIND_LIBRARY(AGDS_LIBRARY     NAMES agds                       PATHS ${VLSISAPD_DIR_SEARCH} PATH_SUFFIXES lib)
    SET_FOUND   (AGDS)
    
    # CIF
    FIND_PATH   (CIF_INCLUDE_DIR NAMES vlsisapd/cif/CifCircuit.h PATHS ${VLSISAPD_DIR_SEARCH} PATH_SUFFIXES include)
    FIND_LIBRARY(CIF_LIBRARY     NAMES cif                       PATHS ${VLSISAPD_DIR_SEARCH} PATH_SUFFIXES lib)
    SET_FOUND   (CIF)
    
    # OPENCHAMS
    FIND_PATH   (OPENCHAMS_INCLUDE_DIR NAMES vlsisapd/openChams/Circuit.h PATHS ${VLSISAPD_DIR_SEARCH} PATH_SUFFIXES include)
    FIND_LIBRARY(OPENCHAMS_LIBRARY     NAMES openChams                    PATHS ${VLSISAPD_DIR_SEARCH} PATH_SUFFIXES lib)
    SET_FOUND   (OPENCHAMS)

    # DTR
    FIND_PATH   (DTR_INCLUDE_DIR NAMES vlsisapd/dtr/Techno.h PATHS ${VLSISAPD_DIR_SEARCH} PATH_SUFFIXES include)
    FIND_LIBRARY(DTR_LIBRARY     NAMES dtr                   PATHS ${VLSISAPD_DIR_SEARCH} PATH_SUFFIXES lib)
    SET_FOUND   (DTR)
    
    IF(AGDS_FOUND AND CIF_FOUND AND OPENCHAMS_FOUND AND DTR_FOUND)
        SET(VLSISAPD_FOUND TRUE)
    ELSE(AGDS_FOUND AND CIF_FOUND AND OPENCHAMS_FOUND AND DTR_FOUND)
        SET(VLSISAPD_FOUND FALSE)
    ENDIF(AGDS_FOUND AND CIF_FOUND AND OPENCHAMS_FOUND AND DTR_FOUND)
ELSE(VLSISAPD_DIR_SEARCH)
    MESSAGE("-- Cannot find VLSISAPD_LIBRARIES since VLSISAPD_DIR_SEARCH is not defined.")
ENDIF(VLSISAPD_DIR_SEARCH)

IF (NOT VLSISAPD_FOUND)
    SET(VLSISAPD_MESSAGE
    "VLSISAPD libraries were not found. Make sure VLSISAPD_TOP env variable is set.")
    IF (NOT VLSISAPD_FIND_QUIETLY)
        MESSAGE(STATUS "${VLSISAPD_MESSAGE}")
    ELSE(NOT VLSISAPD_FIND_QUIETLY)
        IF(VLSISAPD_FIND_REQUIRED)
            MESSAGE(FATAL_ERROR "${VLSISAPD_MESSAGE}")
        ENDIF(VLSISAPD_FIND_REQUIRED)
    ENDIF(NOT VLSISAPD_FIND_QUIETLY)
ELSE (NOT VLSISAPD_FOUND)
    MESSAGE(STATUS "VLSISAPD library was found in ${VLSISAPD_DIR_SEARCH}")
ENDIF (NOT VLSISAPD_FOUND)
