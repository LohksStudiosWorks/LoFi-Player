include(FetchContent)

FetchContent_Declare(
    lsw-engine
    GIT_REPOSITORY https://github.com/LohksStudiosWorks/LSWEngine
    GIT_TAG        refactor
)

FetchContent_MakeAvailable(lsw-engine)