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
	"src/stb_image.cpp"
	"src/stb_image_write.cpp"
)

target_include_directories(
	klonk PUBLIC
	"src"
)
