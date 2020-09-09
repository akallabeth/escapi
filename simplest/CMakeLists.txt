add_executable(simpetest main.cpp $<TARGET_OBJECTS:escapi_interface>)
install(TARGETS simpetest)
