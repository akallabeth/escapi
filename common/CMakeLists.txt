add_library(escapi_interface OBJECT
	escapi.cpp
      	escapi.h
	)

install(FILES escapi.h DESTINATION include COMPONENT headers)
