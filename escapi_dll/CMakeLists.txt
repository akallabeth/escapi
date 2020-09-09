add_library(escapi SHARED
	capture.cpp
      	choosedeviceparam.h
      	conversion.cpp
      	escapi_dll.cpp
  	interface.cpp
    	videobufferlock.cpp
	capture.h
 	conversion.h
      	scopedrelease.h
      	videobufferlock.h
	)

find_library(MF Mf REQUIRED)
find_library(MFPLAT Mfplat REQUIRED)
find_library(MFREADWRITE Mfreadwrite REQUIRED)
find_library(SHLWAPI shlwapi REQUIRED)
target_link_libraries(escapi ${MF} ${MFPLAT} ${MFREADWRITE} ${SHLWAPI})

install(TARGETS escapi)
