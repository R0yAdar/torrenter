install(
    TARGETS torrenter_exe
    RUNTIME COMPONENT torrenter_Runtime
)

if(PROJECT_IS_TOP_LEVEL)
  include(CPack)
endif()
