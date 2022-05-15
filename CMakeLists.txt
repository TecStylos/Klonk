cmake_minimum_required(VERSION 3.8)

project("klonk")

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED TRUE)

add_executable(
	klonk
	"src/klonk.cpp"
	"src/Image.cpp"
	"src/spotify.cpp"
	"src/Response.cpp"
	"src/framebuffer.cpp"
	"src/UserInterface.cpp"
	"src/GenTextImage.cpp"
	"src/stb_image.cpp"
	"src/stb_image_write.cpp"
)

target_include_directories(
	klonk PUBLIC
	"src"
)

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)
target_link_libraries(klonk PRIVATE Threads::Threads)
